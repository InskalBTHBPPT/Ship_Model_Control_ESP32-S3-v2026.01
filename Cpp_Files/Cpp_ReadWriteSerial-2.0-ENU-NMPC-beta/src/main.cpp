/**
 * @file main.cpp
 * @brief Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta
 *
 * Bridge USB seperti 1.2 (mentah) + NMPC C Sep 2026 → timestamp,result.
 * gyro_z diabaikan; r dari yaw_rate. v = 0. ψ = π/2 + yaw IMU (0=N, 270=E).
 */

#include "serial_port.hpp"
#include "telemetry_parser.hpp"
#include "wp_serial.hpp"

#include "geo_enu.h"
#include "nmpc_kapal_waypoint.h"
#include "waypoint_manager.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
volatile std::sig_atomic_t g_running = 1;

void handle_signal(int) { g_running = 0; }

constexpr double kShipLengthM = 1.0107;
constexpr double kU0Mps = 0.6114;
constexpr uint32_t kHorizonN = 20u;
constexpr double kTSimSec = 0.10;
constexpr double kGpsInvalidAbs = 1e-6;
constexpr double kDefaultRTranM = 3.0;
constexpr double kRudderLimitDeg = 45.0;

void print_usage(const char *program_name) {
  std::cerr
      << "Penggunaan:\n"
      << "  " << program_name << " [opsi]\n\n"
      << "Opsi:\n"
      << "  --port <nama_port>       Port serial (default: COM16 / /dev/ttyUSB0)\n"
      << "  --baud <rate>            Baud rate (default: 115200)\n"
      << "  --timeout <ms>           Timeout baca baris (default: 1000)\n"
      << "  --print <all|csv|wp|none> Filter stdout (default: all)\n"
      << "  --rudder-mode <nmpc|zero|yawrate2>\n"
      << "                         nmpc=NMPC C (default)\n"
      << "                         zero=rudder 0 deg\n"
      << "                         yawrate2=clamp(yaw_rate*2, -10, +10)\n"
      << "  --r-tran <m>             Radius ganti waypoint (default: 3.0)\n"
      << "  --help                   Tampilkan bantuan ini\n\n"
      << "Serial RX: CSV 8 kolom, [WP] Home / [WP] #n, $SHUTDOWN\n"
      << "Serial TX: $HB tiap 1 s, timestamp,result (rudder deg)\n"
      << "NMPC: v=0, r dari yaw_rate (bukan gyro_z), psi = pi/2 + yaw\n"
      << "      yaw IMU: 0=Utara, 270=Timur (bukan kompas 90=Timur)\n";
}

bool is_shutdown_line(const std::string &line) { return line == "$SHUTDOWN"; }

void request_os_shutdown() {
#ifdef _WIN32
  std::system(
      "shutdown /s /t 5 /c \"Ship Model: shutdown dari dashboard\"");
#else
  std::system("shutdown -h now");
#endif
}

bool parse_print_mode(const std::string &value, bool &print_csv, bool &print_wp) {
  if (value == "all") {
    print_csv = true;
    print_wp = true;
    return true;
  }
  if (value == "csv") {
    print_csv = true;
    print_wp = false;
    return true;
  }
  if (value == "wp") {
    print_csv = false;
    print_wp = true;
    return true;
  }
  if (value == "none") {
    print_csv = false;
    print_wp = false;
    return true;
  }
  return false;
}

std::string default_port() {
#ifdef _WIN32
  return "COM16";
#else
  return "/dev/ttyUSB0";
#endif
}

std::string format_result_line(double timestamp, double result) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3) << timestamp << ","
      << std::setprecision(2) << result;
  return oss.str();
}

double clamp_deg(double value, double min_deg, double max_deg) {
  return std::max(min_deg, std::min(max_deg, value));
}

bool gps_fix_ok(double lat, double lon) {
  return std::fabs(lat) > kGpsInvalidAbs || std::fabs(lon) > kGpsInvalidAbs;
}

double wrap_pi(double a) {
  return std::atan2(std::sin(a), std::cos(a));
}

// yaw IMU [deg]: 0=Utara, 270=Timur → psi NMPC [rad], 0=Timur CCW
double imu_yaw_to_nmpc_psi(double yaw_deg) {
  return wrap_pi((M_PI / 2.0) + (yaw_deg * (M_PI / 180.0)));
}

// yaw_rate IMU [deg/s] → r nondim (tanda sama: dψ/dt = +yaw_rate)
double yaw_rate_to_r_nd(double yaw_rate_dps, double L, double u0) {
  const double r_rad_s = yaw_rate_dps * (M_PI / 180.0);
  return r_rad_s * (L / u0);
}

struct PendingWaypoints {
  double lat[WP_MAX_WAYPOINTS]{};
  double lon[WP_MAX_WAYPOINTS]{};
  bool have[WP_MAX_WAYPOINTS]{};

  void clear() { std::memset(this, 0, sizeof(*this)); }

  bool store(uint32_t index_1based, double wp_lat, double wp_lon) {
    if (index_1based == 0 || index_1based > WP_MAX_WAYPOINTS) {
      return false;
    }
    const uint32_t i = index_1based - 1u;
    lat[i] = wp_lat;
    lon[i] = wp_lon;
    have[i] = true;
    return true;
  }

  uint32_t consecutive_count() const {
    uint32_t n = 0;
    while (n < WP_MAX_WAYPOINTS && have[n]) {
      ++n;
    }
    return n;
  }
};
} // namespace

int main(int argc, char **argv) {
  std::string port = default_port();
  uint32_t baud = 115200;
  uint32_t timeout_ms = 1000;
  std::string rudder_mode = "nmpc";
  std::string print_mode = "all";
  bool print_csv = true;
  bool print_wp = true;
  double r_tran = kDefaultRTranM;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    }
    if (arg == "--port" && i + 1 < argc) {
      port = argv[++i];
      continue;
    }
    if (arg == "--baud" && i + 1 < argc) {
      baud = static_cast<uint32_t>(std::stoul(argv[++i]));
      continue;
    }
    if (arg == "--timeout" && i + 1 < argc) {
      timeout_ms = static_cast<uint32_t>(std::stoul(argv[++i]));
      continue;
    }
    if (arg == "--print" && i + 1 < argc) {
      print_mode = argv[++i];
      if (!parse_print_mode(print_mode, print_csv, print_wp)) {
        std::cerr << "[ERROR] --print tidak dikenal. Gunakan: all, csv, wp, none\n";
        return 1;
      }
      continue;
    }
    if (arg == "--rudder-mode" && i + 1 < argc) {
      rudder_mode = argv[++i];
      if (rudder_mode != "nmpc" && rudder_mode != "zero" &&
          rudder_mode != "yawrate2") {
        std::cerr << "[ERROR] rudder-mode: nmpc, zero, yawrate2\n";
        return 1;
      }
      continue;
    }
    if (arg == "--r-tran" && i + 1 < argc) {
      r_tran = std::stod(argv[++i]);
      if (r_tran <= 0.0) {
        std::cerr << "[ERROR] --r-tran harus > 0\n";
        return 1;
      }
      continue;
    }

    std::cerr << "Argumen tidak dikenal: " << arg << "\n";
    print_usage(argv[0]);
    return 1;
  }

  std::signal(SIGINT, handle_signal);
#ifndef _WIN32
  std::signal(SIGTERM, handle_signal);
#endif

  SerialPort serial;
  if (!serial.open(port, baud)) {
    std::cerr << "[ERROR] " << serial.last_error() << "\n";
    return 1;
  }

  NMPC_Config_t cfg;
  NMPC_InitDefaultConfig(&cfg, kShipLengthM);
  cfg.r_limit_nd = (45.0 * (M_PI / 180.0)) * (kShipLengthM / kU0Mps);

  HomeOrigin origin = default_home_origin();
  PendingWaypoints pending_wps;
  WP_Manager_t wp_mgr{};
  bool wp_ready = false;
  bool mission_done = false;
  double u_prev = 0.0;

  std::cerr << "[INFO] Port " << port << " @ " << baud << " baud\n";
  std::cerr << "[INFO] Rudder mode: " << rudder_mode
            << " | $HB 1 Hz | timestamp,result TX\n";
  std::cerr << std::fixed << std::setprecision(4)
            << "[INFO] NMPC L=" << kShipLengthM << " m u0=" << kU0Mps
            << " m/s N=" << kHorizonN << " T_sim=" << kTSimSec
            << " s r_tran=" << r_tran << " m\n";
  std::cerr << std::setprecision(7)
            << "[INFO] Origin default: " << origin.lat << ", " << origin.lon
            << " (menunggu [WP] Home)\n";
  std::cerr << "[INFO] v=0 | r dari yaw_rate | psi = pi/2 + yaw (0=N 270=E) | gyro_z abaikan\n";
  std::cerr << "[INFO] Tekan Ctrl+C untuk berhenti\n";

  uint64_t valid_lines = 0;
  uint64_t waypoint_lines = 0;
  uint64_t skipped_lines = 0;
  uint64_t write_errors = 0;
  uint64_t nmpc_ok = 0;
  uint64_t nmpc_hold = 0;
  auto last_hb = std::chrono::steady_clock::now();
  auto last_nmpc_log = last_hb;
  int32_t last_status = 0;
  double last_result_deg = 0.0;
  double last_dist = 0.0;

  while (g_running) {
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hb)
            .count() >= 1000) {
      if (!serial.write_line("$HB")) {
        std::cerr << "[WARN] Gagal kirim heartbeat: " << serial.last_error()
                  << "\n";
      }
      last_hb = now;
    }

    std::string raw_line;
    if (!serial.read_line(raw_line, timeout_ms)) {
      continue;
    }

    const std::string line = trim_cr(raw_line);
    if (line.empty()) {
      continue;
    }

    if (is_header_line(line)) {
      if (print_csv) {
        std::cout << line << "\n" << std::flush;
      }
      continue;
    }

    if (is_waypoint_line(line)) {
      ++waypoint_lines;
      if (const auto parsed_home = try_parse_wp_home(line)) {
        const bool changed = origin_changed(origin, *parsed_home);
        origin = *parsed_home;
        pending_wps.clear();
        wp_ready = false;
        mission_done = false;
        std::memset(&wp_mgr, 0, sizeof(wp_mgr));
        if (changed) {
          std::cerr << std::fixed << std::setprecision(7)
                    << "[INFO] Origin ENU = " << origin.lat << ", " << origin.lon
                    << (origin.from_home ? " (Home)" : " (default, Home <none>)")
                    << " — reset WP\n";
        }
      } else if (const auto parsed_wp = try_parse_wp_numbered(line)) {
        if (pending_wps.store(parsed_wp->index_1based, parsed_wp->lat,
                              parsed_wp->lon)) {
          const uint32_t n = pending_wps.consecutive_count();
          if (n > 0 && (!wp_ready || n != wp_mgr.count)) {
            double east[WP_MAX_WAYPOINTS];
            double north[WP_MAX_WAYPOINTS];
            for (uint32_t i = 0; i < n; ++i) {
              GEO_LLA2ENU(origin.lat, origin.lon, pending_wps.lat[i],
                          pending_wps.lon[i], GEO_EARTH_RADIUS_M, &east[i],
                          &north[i]);
            }
            if (WP_Init(&wp_mgr, east, north, n, r_tran)) {
              wp_ready = true;
              mission_done = false;
              std::cerr << "[INFO] WP siap: " << n << " titik, r_tran="
                        << std::fixed << std::setprecision(1) << r_tran
                        << " m\n";
            }
          }
        }
      }
      if (print_wp) {
        std::cout << line << "\n" << std::flush;
      }
      continue;
    }

    if (is_shutdown_line(line)) {
      std::cerr << "[INFO] $SHUTDOWN — mematikan sistem dalam ~5 detik\n"
                << std::flush;
      if (print_csv || print_wp) {
        std::cout << line << "\n" << std::flush;
      }
      serial.close();
      request_os_shutdown();
      g_running = 0;
      break;
    }

    const auto row = parse_telemetry_line(line);
    if (!row) {
      ++skipped_lines;
      continue;
    }

    ++valid_lines;
    if (print_csv) {
      std::cout << line << "\n" << std::flush;
    }

    double rudder_deg = 0.0;

    if (rudder_mode == "zero") {
      rudder_deg = 0.0;
    } else if (rudder_mode == "yawrate2") {
      rudder_deg = clamp_deg(row->yaw_rate * 2.0, -10.0, 10.0);
    } else {
      if (!gps_fix_ok(row->lat, row->lon)) {
        rudder_deg = 0.0;
        last_status = 0;
        ++nmpc_hold;
      } else if (!wp_ready || wp_mgr.count == 0) {
        rudder_deg = 0.0;
        last_status = 0;
        ++nmpc_hold;
      } else {
        double east = 0.0;
        double north = 0.0;
        GEO_LLA2ENU(origin.lat, origin.lon, row->lat, row->lon,
                    GEO_EARTH_RADIUS_M, &east, &north);

        double dist = 0.0;
        if (WP_UpdateActive(&wp_mgr, east, north, &dist)) {
          std::cerr << "[INFO] Masuk r_tran → WP " << (wp_mgr.active_idx + 1u)
                    << "\n";
        }
        last_dist = dist;

        if (WP_IsMissionComplete(&wp_mgr, dist)) {
          if (!mission_done) {
            std::cerr << "[INFO] Misi selesai (WP terakhir dalam r_tran)\n";
            mission_done = true;
          }
          rudder_deg = 0.0;
          last_status = 0;
          ++nmpc_hold;
        } else {
          double wp_e = 0.0;
          double wp_n = 0.0;
          WP_GetActiveTarget(&wp_mgr, &wp_e, &wp_n);
          const double theta_target = std::atan2(wp_n - north, wp_e - east);

          double x_ref[NMPC_MAX_HORIZON];
          double y_ref[NMPC_MAX_HORIZON];
          double psi_ref[NMPC_MAX_HORIZON];
          for (uint32_t h = 0; h < kHorizonN; ++h) {
            const double dist_step = static_cast<double>(h + 1u) * kU0Mps * kTSimSec;
            x_ref[h] = (east + dist_step * std::cos(theta_target)) / kShipLengthM;
            y_ref[h] = (north + dist_step * std::sin(theta_target)) / kShipLengthM;
            psi_ref[h] = theta_target;
          }

          const double psi = imu_yaw_to_nmpc_psi(row->yaw);
          const double r_nd = yaw_rate_to_r_nd(row->yaw_rate, kShipLengthM, kU0Mps);
          const double s_nd[NMPC_NUM_STATES] = {
              0.0, r_nd, east / kShipLengthM, north / kShipLengthM, psi};

          double u_opt = u_prev;
          last_status = NMPC_Solve(&cfg, s_nd, u_prev, kU0Mps, x_ref, y_ref,
                                   psi_ref, kHorizonN, &u_opt);
          if (last_status < 0) {
            rudder_deg = 0.0;
            ++nmpc_hold;
          } else if (last_status == 0) {
            u_opt = u_prev;
            rudder_deg = clamp_deg(u_opt * (180.0 / M_PI), -kRudderLimitDeg,
                                   kRudderLimitDeg);
            ++nmpc_hold;
          } else {
            rudder_deg = clamp_deg(u_opt * (180.0 / M_PI), -kRudderLimitDeg,
                                   kRudderLimitDeg);
            u_prev = u_opt;
            ++nmpc_ok;
          }

          const auto log_now = std::chrono::steady_clock::now();
          if (std::chrono::duration_cast<std::chrono::milliseconds>(
                  log_now - last_nmpc_log)
                  .count() >= 1000) {
            std::cerr << std::fixed << std::setprecision(2)
                      << "[NMPC] WP" << (wp_mgr.active_idx + 1u)
                      << " d=" << dist << " m | E=" << east << " N=" << north
                      << " | psi=" << (psi * (180.0 / M_PI))
                      << " deg | d=" << rudder_deg << " deg | status="
                      << last_status << "\n";
            last_nmpc_log = log_now;
          }
        }
      }
    }

    last_result_deg = rudder_deg;
    const std::string result_line =
        format_result_line(row->timestamp, rudder_deg);
    if (!serial.write_line(result_line)) {
      ++write_errors;
      std::cerr << "[ERROR] Gagal tulis serial: " << serial.last_error()
                << " | line=" << result_line << "\n";
    }
  }

  std::cerr << "\n[INFO] Selesai. CSV valid: " << valid_lines
            << ", WP baris: " << waypoint_lines << ", skip: " << skipped_lines
            << ", TX gagal: " << write_errors << ", NMPC ok: " << nmpc_ok
            << ", hold: " << nmpc_hold << std::fixed << std::setprecision(2)
            << " | last result=" << last_result_deg << " deg"
            << " d_wp=" << last_dist << " m\n";
  return 0;
}

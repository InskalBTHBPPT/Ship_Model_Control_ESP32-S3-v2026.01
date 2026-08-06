#include "processor.hpp"
#include "serial_port.hpp"
#include "telemetry_parser.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
volatile std::sig_atomic_t g_running = 1;

void handle_signal(int) { g_running = 0; }

void print_usage(const char *program_name) {
  std::cerr
      << "Penggunaan:\n"
      << "  " << program_name << " [opsi]\n\n"
      << "Opsi:\n"
      << "  --port <nama_port>       Port serial (default: COM16 / /dev/ttyUSB0)\n"
      << "  --baud <rate>            Baud rate (default: 115200)\n"
      << "  --timeout <ms>           Timeout baca baris (default: 1000)\n"
      << "  --print <all|csv|wp>     Filter stdout (default: all)\n"
      << "                         all=CSV 8 kolom + [WP]\n"
      << "                         csv=hanya CSV / header\n"
      << "                         wp=hanya baris [WP]\n"
      << "  --op <add|sub|mul|div>   Operasi demo (default: sub)\n"
      << "  --field-a <nama_field>   Field pertama (default: calc_deg_servo_1)\n"
      << "  --field-b <nama_field>   Field kedua (default: calc_deg_servo_2)\n"
      << "  --rudder-mode <zero|yawrate2|demo>\n"
      << "                         zero=rudder 0 deg (default)\n"
      << "                         yawrate2=clamp(yaw_rate*2, -10, +10) deg\n"
      << "                         demo=--op math pada field\n"
      << "  --help                   Tampilkan bantuan ini\n\n"
      << "Field yang didukung:\n"
      << "  timestamp, lat, lon, calc_deg_servo_1, calc_deg_servo_2,\n"
      << "  yaw, gyro_z, yaw_rate\n\n"
      << "Perilaku:\n"
      << "  - Heartbeat $HB -> ESP32 setiap 1 detik (manual/auto)\n"
      << "  - Filter stdout via --print (hitung rudder + TX tetap jalan)\n"
      << "  - Baris timestamp,result (rudder deg) -> serial TX saja\n"
      << "  - Baris $SHUTDOWN dari Remote -> matikan OS (Windows/Linux)\n";
}

bool is_shutdown_line(const std::string &line) {
  return line == "$SHUTDOWN";
}

void request_os_shutdown() {
#ifdef _WIN32
  // Delay singkat agar pesan sempat tertulis ke console.
  std::system(
      "shutdown /s /t 5 /c \"Ship Model: shutdown dari dashboard\"");
#else
  std::system("shutdown -h now");
#endif
}

bool is_waypoint_line(const std::string &line) {
  // Remote-Side printWaypoints(): "[WP] ..." ke USB Serial yang sama.
  return line.size() >= 4 && line.compare(0, 4, "[WP]") == 0;
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

double clamp_rudder_deg(double value, double min_deg, double max_deg) {
  return std::max(min_deg, std::min(max_deg, value));
}
} // namespace

int main(int argc, char **argv) {
  std::string port = default_port();
  uint32_t baud = 115200;
  uint32_t timeout_ms = 1000;
  MathOp math_op = MathOp::Sub;
  TelemetryField field_a = TelemetryField::CalcDegServo1;
  TelemetryField field_b = TelemetryField::CalcDegServo2;
  std::string rudder_mode = "zero";
  std::string print_mode = "all";
  bool print_csv = true;
  bool print_wp = true;

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
        std::cerr << "[ERROR] --print tidak dikenal. Gunakan: all, csv, wp\n";
        return 1;
      }
      continue;
    }
    if (arg == "--rudder-mode" && i + 1 < argc) {
      rudder_mode = argv[++i];
      if (rudder_mode != "zero" && rudder_mode != "yawrate2" && rudder_mode != "demo") {
        std::cerr << "[ERROR] rudder-mode tidak dikenal. Gunakan: zero, yawrate2, demo\n";
        return 1;
      }
      continue;
    }
    if (arg == "--op" && i + 1 < argc) {
      const auto parsed = parse_math_op(argv[++i]);
      if (!parsed) {
        std::cerr << "[ERROR] Operasi tidak dikenal. Gunakan: add, sub, mul, div\n";
        return 1;
      }
      math_op = *parsed;
      continue;
    }
    if (arg == "--field-a" && i + 1 < argc) {
      const auto parsed = parse_field_name(argv[++i]);
      if (!parsed) {
        std::cerr << "[ERROR] field-a tidak dikenal\n";
        return 1;
      }
      field_a = *parsed;
      continue;
    }
    if (arg == "--field-b" && i + 1 < argc) {
      const auto parsed = parse_field_name(argv[++i]);
      if (!parsed) {
        std::cerr << "[ERROR] field-b tidak dikenal\n";
        return 1;
      }
      field_b = *parsed;
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

  std::cerr << "[INFO] Port " << port << " @ " << baud << " baud (read + write)\n";
  std::cerr << "[INFO] Heartbeat $HB -> ESP32 setiap 1 detik\n";
  std::cerr << "[INFO] Print mode: " << print_mode
            << " | timestamp,result -> serial TX\n";
  std::cerr << "[INFO] Rudder mode: " << rudder_mode << "\n";
  std::cerr << "[INFO] Tekan Ctrl+C untuk berhenti\n";

  uint64_t valid_lines = 0;
  uint64_t waypoint_lines = 0;
  uint64_t skipped_lines = 0;
  uint64_t write_errors = 0;
  auto last_hb = std::chrono::steady_clock::now();

  while (g_running) {
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hb).count() >= 1000) {
      if (!serial.write_line("$HB")) {
        std::cerr << "[WARN] Gagal kirim heartbeat: " << serial.last_error() << "\n";
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

    // Waypoint dari Remote (setelah dashboard Send Way Points).
    if (is_waypoint_line(line)) {
      ++waypoint_lines;
      if (print_wp) {
        std::cout << line << "\n" << std::flush;
      }
      continue;
    }

    // Shutdown dari dashboard → User → ESP-NOW → Remote → Serial.
    if (is_shutdown_line(line)) {
      std::cerr << "[INFO] Perintah $SHUTDOWN diterima — mematikan sistem dalam ~5 detik\n"
                << std::flush;
      std::cout << line << "\n" << std::flush;
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

    double rudder_deg = 0.0;
    if (rudder_mode == "yawrate2") {
      rudder_deg = clamp_rudder_deg(row->yaw_rate * 2.0, -10.0, 10.0);
    } else if (rudder_mode == "demo") {
      const auto result = compute_result(*row, math_op, field_a, field_b);
      if (!result) {
        ++skipped_lines;
        std::cerr << "[WARN] Hitung result gagal pada t=" << row->timestamp << "\n";
        if (print_csv) {
          std::cout << line << "\n" << std::flush;
        }
        continue;
      }
      rudder_deg = *result;
    }

    ++valid_lines;
    if (print_csv) {
      std::cout << line << "\n" << std::flush;
    }

    const std::string result_line = format_result_line(row->timestamp, rudder_deg);
    if (!serial.write_line(result_line)) {
      ++write_errors;
      std::cerr << "[ERROR] Gagal tulis ke serial: " << serial.last_error()
                << " | line=" << result_line << "\n";
    }
  }

  std::cerr << "\n[INFO] Selesai. Baris valid: " << valid_lines
            << ", waypoint: " << waypoint_lines
            << ", dilewati: " << skipped_lines
            << ", gagal tulis serial: " << write_errors << "\n";
  return 0;
}

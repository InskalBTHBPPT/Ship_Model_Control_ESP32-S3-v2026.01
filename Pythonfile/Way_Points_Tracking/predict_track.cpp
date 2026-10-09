/**
 * Prediksi lintasan tertutup: Home, yaw awal 90° (timur), model WyNDA + NMPC 2.3.
 * Argumen: file CSV No,Lat,Long. stdout = log tampilan untuk tab 3D.
 * Bangun dari root repo:
 *   g++ -std=c++17 -O2 -ICpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc
 *     "Pythonfile/Way_Points_Tracking/predict_track.cpp"
 *     Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc/nmpc_kapal_waypoint.c
 *     Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc/waypoint_manager.c
 *     Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/nmpc/geo_enu.c
 *     -o "Pythonfile/Way_Points_Tracking/predict_track.exe"
 */
#include "geo_enu.h"
#include "nmpc_kapal_waypoint.h"
#include "waypoint_manager.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr double kL = 1.0107;
constexpr double kU0 = 0.6114;
constexpr double kDt = 0.10;
constexpr uint32_t kN = 20u;
constexpr double kRTran = 3.0;
constexpr double kYaw0Deg = 90.0;
constexpr int kMaxSteps = 9000;

double wrap360(double deg) {
  while (deg < 0.0) deg += 360.0;
  while (deg >= 360.0) deg -= 360.0;
  return deg;
}

double wrap180(double deg) {
  while (deg > 180.0) deg -= 360.0;
  while (deg < -180.0) deg += 360.0;
  return deg;
}

double wrap_pi(double a) { return std::atan2(std::sin(a), std::cos(a)); }

double compass_to_psi(double yaw_deg) {
  return wrap_pi((M_PI / 2.0) - (yaw_deg * (M_PI / 180.0)));
}

double psi_to_compass(double psi) {
  return wrap360(90.0 - (psi * (180.0 / M_PI)));
}

bool load_wp_csv(const std::string &path, double &home_lat, double &home_lon,
                 std::vector<std::pair<double, double>> &wps) {
  std::ifstream in(path);
  if (!in) return false;
  std::string line;
  bool have_home = false;
  if (!std::getline(in, line)) return false;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::stringstream ss(line);
    std::string no, lat_s, lon_s;
    if (!std::getline(ss, no, ',')) continue;
    if (!std::getline(ss, lat_s, ',')) continue;
    if (!std::getline(ss, lon_s, ',')) continue;
    const double lat = std::stod(lat_s);
    const double lon = std::stod(lon_s);
    if (no == "Home" || no == "home") {
      home_lat = lat;
      home_lon = lon;
      have_home = true;
    } else {
      wps.emplace_back(lat, lon);
    }
  }
  return have_home && !wps.empty() && wps.size() <= WP_MAX_WAYPOINTS;
}
}  // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: predict_track <waypoints.csv>\n";
    return 1;
  }
  double home_lat = 0.0;
  double home_lon = 0.0;
  std::vector<std::pair<double, double>> wps;
  if (!load_wp_csv(argv[1], home_lat, home_lon, wps)) {
    std::cerr << "[ERROR] CSV butuh baris Home dan minimal satu waypoint\n";
    return 2;
  }

  std::vector<double> east(wps.size()), north(wps.size());
  for (size_t i = 0; i < wps.size(); ++i) {
    GEO_LLA2ENU(home_lat, home_lon, wps[i].first, wps[i].second, GEO_EARTH_RADIUS_M,
                &east[i], &north[i]);
  }

  WP_Manager_t mgr{};
  if (!WP_Init(&mgr, east.data(), north.data(), static_cast<uint32_t>(wps.size()), kRTran)) {
    std::cerr << "[ERROR] WP_Init gagal\n";
    return 3;
  }

  NMPC_Config_t cfg{};
  NMPC_InitDefaultConfig(&cfg, kL);
  cfg.r_limit_nd = (45.0 * (M_PI / 180.0)) * (kL / kU0);

  double s[NMPC_NUM_STATES] = {0.0, 0.0, 0.0, 0.0, compass_to_psi(kYaw0Deg)};
  double u_prev = 0.0;
  double delta_applied = 0.0;

  std::cout << "timestamp (s),latitude (°),longitude (°),speedMps (m/s),"
               "u (m/s),v (m/s),Calc_deg_servo_1 (°),Calc_deg_servo_2 (°),"
               "yaw (°),heading_setpoint (°),heading_error (°),rudder_cmd (°),"
               "track_wp_index,distance_to_wp (m),accel_x (g),accel_y (g),accel_z (g),"
               "gyro_x (deg/s),gyro_y (deg/s),gyro_z (deg/s),rpm_prop_1 (rpm),"
               "rpm_prop_2 (rpm),battery_1 (V),battery_2 (V),mode_auto,mini_pc_link\n";
  std::cout.setf(std::ios::fixed);
  std::cout.precision(6);

  for (int step = 0; step < kMaxSteps; ++step) {
    const double t = step * kDt;
    const double ship_e = s[2] * kL;
    const double ship_n = s[3] * kL;
    double lat = 0.0;
    double lon = 0.0;
    GEO_ENU2LLA(home_lat, home_lon, ship_e, ship_n, GEO_EARTH_RADIUS_M, &lat, &lon);
    const double yaw = psi_to_compass(s[4]);

    double dist = 0.0;
    WP_UpdateActive(&mgr, ship_e, ship_n, &dist);
    const bool done = WP_IsMissionComplete(&mgr, dist) != 0;

    double wp_e = 0.0;
    double wp_n = 0.0;
    WP_GetActiveTarget(&mgr, &wp_e, &wp_n);
    const double theta = std::atan2(wp_n - ship_n, wp_e - ship_e);
    const double bearing = psi_to_compass(theta);
    const double rudder_deg = done ? 0.0 : (delta_applied * (180.0 / M_PI));
    const int mode_auto = done ? 0 : 2;
    std::cout << t << "," << lat << "," << lon << "," << kU0 << "," << kU0 << ",0,"
              << "0,0," << yaw << "," << bearing << "," << wrap180(bearing - yaw) << ","
              << rudder_deg << "," << (mgr.active_idx + 1u) << "," << dist
              << ",0,0,0,0,0,0,0,0,0,0," << mode_auto << ",1\n";
    if (done) break;

    double delta = 0.0;
    double x_ref[NMPC_MAX_HORIZON];
    double y_ref[NMPC_MAX_HORIZON];
    double psi_ref[NMPC_MAX_HORIZON];
    for (uint32_t h = 0; h < kN; ++h) {
      const double dist_step = static_cast<double>(h + 1u) * kU0 * kDt;
      x_ref[h] = (ship_e + dist_step * std::cos(theta)) / kL;
      y_ref[h] = (ship_n + dist_step * std::sin(theta)) / kL;
      psi_ref[h] = theta;
    }
    double u_opt = u_prev;
    const int32_t status = NMPC_Solve(&cfg, s, u_prev, kU0, x_ref, y_ref, psi_ref, kN, &u_opt);
    if (status > 0) {
      delta = u_opt;
      u_prev = u_opt;
    } else if (status == 0) {
      delta = u_prev;
    }
    if (delta > 45.0 * M_PI / 180.0) delta = 45.0 * M_PI / 180.0;
    if (delta < -45.0 * M_PI / 180.0) delta = -45.0 * M_PI / 180.0;
    delta_applied = delta;

    double s_next[NMPC_NUM_STATES];
    NMPC_EulerStep(s, delta, cfg.theta, s_next);
    s_next[4] = wrap_pi(s_next[4]);
    for (int i = 0; i < NMPC_NUM_STATES; ++i) s[i] = s_next[i];
  }
  std::cerr << "[PREDICT] selesai, titik=" << wps.size() << " yaw0=" << kYaw0Deg << "\n";
  return 0;
}

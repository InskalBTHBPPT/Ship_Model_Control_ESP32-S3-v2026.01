/**
 * @file nmpc_tick.hpp
 * @brief Adaptor mentah → NMPC_Solve → result deg (sama aturan 2.0 live).
 *
 * v=0, r dari yaw_rate (bukan gyro_z), psi = pi/2 + yaw IMU (0=N, 270=E).
 */
#pragma once

#include "telemetry_parser.hpp"
#include "wp_serial.hpp"

#include "geo_enu.h"
#include "nmpc_kapal_waypoint.h"
#include "waypoint_manager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline constexpr double kShipLengthM = 1.0107;
inline constexpr double kU0Mps = 0.6114;
inline constexpr uint32_t kHorizonN = 20u;
inline constexpr double kTSimSec = 0.10;
inline constexpr double kGpsInvalidAbs = 1e-6;
inline constexpr double kDefaultRTranM = 3.0;
inline constexpr double kRudderLimitDeg = 45.0;

inline double clamp_deg(double value, double min_deg, double max_deg) {
  return std::max(min_deg, std::min(max_deg, value));
}

inline bool gps_fix_ok(double lat, double lon) {
  return std::fabs(lat) > kGpsInvalidAbs || std::fabs(lon) > kGpsInvalidAbs;
}

inline double wrap_pi(double a) {
  return std::atan2(std::sin(a), std::cos(a));
}

inline double imu_yaw_to_nmpc_psi(double yaw_deg) {
  return wrap_pi((M_PI / 2.0) + (yaw_deg * (M_PI / 180.0)));
}

inline double yaw_rate_to_r_nd(double yaw_rate_dps, double L, double u0) {
  const double r_rad_s = yaw_rate_dps * (M_PI / 180.0);
  return r_rad_s * (L / u0);
}

inline std::string format_result_line(double timestamp, double result) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3) << timestamp << ","
      << std::setprecision(2) << result;
  return oss.str();
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

struct NmpcTickInfo {
  int32_t status = 0;
  double result_deg = 0.0;
  double east_m = 0.0;
  double north_m = 0.0;
  double dist_m = 0.0;
  double psi_deg = 0.0;
  uint32_t active_wp_1based = 0;
  bool switched = false;
  bool mission_complete = false;
  bool hold = false;
};

struct NmpcSession {
  NMPC_Config_t cfg{};
  HomeOrigin origin = default_home_origin();
  PendingWaypoints pending{};
  WP_Manager_t wp_mgr{};
  bool wp_ready = false;
  bool mission_done = false;
  double u_prev = 0.0;
  double r_tran = kDefaultRTranM;

  void init(double switch_radius_m = kDefaultRTranM) {
    NMPC_InitDefaultConfig(&cfg, kShipLengthM);
    cfg.r_limit_nd = (45.0 * (M_PI / 180.0)) * (kShipLengthM / kU0Mps);
    origin = default_home_origin();
    pending.clear();
    std::memset(&wp_mgr, 0, sizeof(wp_mgr));
    wp_ready = false;
    mission_done = false;
    u_prev = 0.0;
    r_tran = switch_radius_m;
  }

  bool apply_wp_line(const std::string &line, std::string *info_out = nullptr) {
    if (const auto parsed_home = try_parse_wp_home(line)) {
      origin = *parsed_home;
      pending.clear();
      wp_ready = false;
      mission_done = false;
      std::memset(&wp_mgr, 0, sizeof(wp_mgr));
      if (info_out) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(7)
            << "Origin ENU = " << origin.lat << ", " << origin.lon
            << (origin.from_home ? " (Home)" : " (default)") << " — reset WP";
        *info_out = oss.str();
      }
      return true;
    }
    if (const auto parsed_wp = try_parse_wp_numbered(line)) {
      if (!pending.store(parsed_wp->index_1based, parsed_wp->lat,
                         parsed_wp->lon)) {
        return false;
      }
      const uint32_t n = pending.consecutive_count();
      if (n > 0 && (!wp_ready || n != wp_mgr.count)) {
        double east[WP_MAX_WAYPOINTS];
        double north[WP_MAX_WAYPOINTS];
        for (uint32_t i = 0; i < n; ++i) {
          GEO_LLA2ENU(origin.lat, origin.lon, pending.lat[i], pending.lon[i],
                      GEO_EARTH_RADIUS_M, &east[i], &north[i]);
        }
        if (WP_Init(&wp_mgr, east, north, n, r_tran)) {
          wp_ready = true;
          mission_done = false;
          if (info_out) {
            *info_out = "WP siap: " + std::to_string(n) + " titik";
          }
          return true;
        }
      }
      return true;
    }
    return false;
  }

  NmpcTickInfo tick(const TelemetryRow &row) {
    NmpcTickInfo info{};
    info.result_deg = 0.0;
    info.hold = true;

    if (!gps_fix_ok(row.lat, row.lon) || !wp_ready || wp_mgr.count == 0) {
      info.status = 0;
      return info;
    }

    GEO_LLA2ENU(origin.lat, origin.lon, row.lat, row.lon, GEO_EARTH_RADIUS_M,
                &info.east_m, &info.north_m);

    double dist = 0.0;
    info.switched = WP_UpdateActive(&wp_mgr, info.east_m, info.north_m, &dist) != 0;
    info.dist_m = dist;
    info.active_wp_1based = wp_mgr.active_idx + 1u;

    if (WP_IsMissionComplete(&wp_mgr, dist)) {
      mission_done = true;
      info.mission_complete = true;
      info.status = 0;
      return info;
    }

    double wp_e = 0.0;
    double wp_n = 0.0;
    WP_GetActiveTarget(&wp_mgr, &wp_e, &wp_n);
    const double theta_target =
        std::atan2(wp_n - info.north_m, wp_e - info.east_m);

    double x_ref[NMPC_MAX_HORIZON];
    double y_ref[NMPC_MAX_HORIZON];
    double psi_ref[NMPC_MAX_HORIZON];
    for (uint32_t h = 0; h < kHorizonN; ++h) {
      const double dist_step = static_cast<double>(h + 1u) * kU0Mps * kTSimSec;
      x_ref[h] = (info.east_m + dist_step * std::cos(theta_target)) / kShipLengthM;
      y_ref[h] = (info.north_m + dist_step * std::sin(theta_target)) / kShipLengthM;
      psi_ref[h] = theta_target;
    }

    const double psi = imu_yaw_to_nmpc_psi(row.yaw);
    info.psi_deg = psi * (180.0 / M_PI);
    const double r_nd = yaw_rate_to_r_nd(row.yaw_rate, kShipLengthM, kU0Mps);
    const double s_nd[NMPC_NUM_STATES] = {
        0.0, r_nd, info.east_m / kShipLengthM, info.north_m / kShipLengthM, psi};

    double u_opt = u_prev;
    info.status = NMPC_Solve(&cfg, s_nd, u_prev, kU0Mps, x_ref, y_ref, psi_ref,
                             kHorizonN, &u_opt);
    if (info.status < 0) {
      info.result_deg = 0.0;
      return info;
    }
    if (info.status == 0) {
      u_opt = u_prev;
      info.result_deg =
          clamp_deg(u_opt * (180.0 / M_PI), -kRudderLimitDeg, kRudderLimitDeg);
      return info;
    }

    info.result_deg =
        clamp_deg(u_opt * (180.0 / M_PI), -kRudderLimitDeg, kRudderLimitDeg);
    u_prev = u_opt;
    info.hold = false;
    return info;
  }
};

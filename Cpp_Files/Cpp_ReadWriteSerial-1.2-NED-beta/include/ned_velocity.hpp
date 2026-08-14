#pragma once

#include "local_frame.hpp"

#include <cmath>

// u, v = kecepatan badan (m/s), BUKAN perpindahan (m).
// X, Y = posisi NED (m) relatif origin.

inline constexpr double kDegToRad = kPi / 180.0;
inline constexpr double kVelLpfAlpha = 0.70;  // bobot nilai lama (0=mentah, ~1=lebih halus)
inline constexpr double kMinDtSec = 0.001;
inline constexpr double kMaxDtSec = 1.0;
inline constexpr double kGpsInvalidAbs = 1e-6;

struct NedKinematics {
  LocalXy xy{};        // X=North (m), Y=East (m) — posisi / perpindahan
  double x_dot = 0.0;  // Ẋ North (m/s)
  double y_dot = 0.0;  // Ẏ East  (m/s)
  double speed = 0.0;  // |V| = sqrt(Ẋ² + Ẏ²) (m/s)
  double psi = 0.0;    // heading (rad)
  double r = 0.0;      // yaw rate dari gyro_z (rad/s)
  double u = 0.0;      // surge (m/s) maju +
  double v = 0.0;      // sway  (m/s) kanan +
  bool gps_ok = false;
  bool vel_ok = false;
};

inline bool gps_fix_ok(double lat, double lon) {
  return std::fabs(lat) > kGpsInvalidAbs || std::fabs(lon) > kGpsInvalidAbs;
}

inline double yaw_deg_to_psi_rad(double yaw_deg) {
  return yaw_deg * kDegToRad;
}

inline double gyro_z_dps_to_r_rad(double gyro_z_dps) {
  return gyro_z_dps * kDegToRad;
}

// Ẋ,Ẏ peta NED → u,v badan. ψ dari Utara, naik ke timur (kompas).
inline void ned_vel_to_body(double x_dot, double y_dot, double psi, double &u,
                            double &v) {
  const double c = std::cos(psi);
  const double s = std::sin(psi);
  u = x_dot * c + y_dot * s;
  v = -x_dot * s + y_dot * c;
}

struct NedVelocityTracker {
  bool have_prev = false;
  LocalXy prev_xy{};
  double prev_t = 0.0;
  double x_dot_f = 0.0;
  double y_dot_f = 0.0;

  void reset() {
    have_prev = false;
    prev_xy = {};
    prev_t = 0.0;
    x_dot_f = 0.0;
    y_dot_f = 0.0;
  }

  NedKinematics update(double timestamp_s, double lat, double lon, double yaw_deg,
                       double gyro_z_dps, const Origin &origin) {
    NedKinematics out{};
    out.xy = ll_to_local(lat, lon, origin);
    out.psi = yaw_deg_to_psi_rad(yaw_deg);
    out.r = gyro_z_dps_to_r_rad(gyro_z_dps);
    out.gps_ok = gps_fix_ok(lat, lon);

    if (!out.gps_ok) {
      reset();
      ned_vel_to_body(0.0, 0.0, out.psi, out.u, out.v);
      return out;
    }

    if (!have_prev) {
      prev_xy = out.xy;
      prev_t = timestamp_s;
      have_prev = true;
      ned_vel_to_body(0.0, 0.0, out.psi, out.u, out.v);
      return out;
    }

    const double dt = timestamp_s - prev_t;
    const LocalXy last_xy = prev_xy;
    prev_xy = out.xy;
    prev_t = timestamp_s;

    if (dt < kMinDtSec || dt > kMaxDtSec) {
      x_dot_f = 0.0;
      y_dot_f = 0.0;
      ned_vel_to_body(0.0, 0.0, out.psi, out.u, out.v);
      return out;
    }

    const double x_dot_raw = (out.xy.x - last_xy.x) / dt;
    const double y_dot_raw = (out.xy.y - last_xy.y) / dt;
    x_dot_f = kVelLpfAlpha * x_dot_f + (1.0 - kVelLpfAlpha) * x_dot_raw;
    y_dot_f = kVelLpfAlpha * y_dot_f + (1.0 - kVelLpfAlpha) * y_dot_raw;

    out.x_dot = x_dot_f;
    out.y_dot = y_dot_f;
    out.speed = std::sqrt(out.x_dot * out.x_dot + out.y_dot * out.y_dot);
    ned_vel_to_body(out.x_dot, out.y_dot, out.psi, out.u, out.v);
    out.vel_ok = true;
    return out;
  }
};

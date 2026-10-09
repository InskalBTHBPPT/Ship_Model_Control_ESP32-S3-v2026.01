/**
 * @file body_velocity.hpp
 * @brief ẋ,ẏ ENU → surge u / sway v (m/s). Bukan konstanta.
 *
 * ψ = yaw kompas CW (rad): 0 = Utara, 90° = Timur.
 * Low-pass α = 0.70 pada ẋ,ẏ. Sampel pertama dan Δt di luar 1 ms…1 s → u,v = 0.
 */
#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline void enu_vel_to_body(double x_dot, double y_dot, double psi_compass,
                            double &u, double &v) {
  const double c = std::cos(psi_compass);
  const double s = std::sin(psi_compass);
  u = x_dot * s + y_dot * c;
  v = x_dot * c - y_dot * s;
}

struct BodyVelocityTracker {
  static constexpr double kAlpha = 0.70;
  static constexpr double kMinDtSec = 0.001;
  static constexpr double kMaxDtSec = 1.0;

  bool have_prev = false;
  double prev_east = 0.0;
  double prev_north = 0.0;
  double prev_t = 0.0;
  double x_dot_f = 0.0;
  double y_dot_f = 0.0;

  void reset() {
    have_prev = false;
    prev_east = 0.0;
    prev_north = 0.0;
    prev_t = 0.0;
    x_dot_f = 0.0;
    y_dot_f = 0.0;
  }

  struct Sample {
    double u = 0.0;
    double v = 0.0;
    double x_dot = 0.0;
    double y_dot = 0.0;
    bool vel_ok = false;
  };

  Sample update(double timestamp_s, double east_m, double north_m, double yaw_deg) {
    Sample out{};
    const double psi = yaw_deg * (M_PI / 180.0);

    if (!have_prev) {
      prev_east = east_m;
      prev_north = north_m;
      prev_t = timestamp_s;
      have_prev = true;
      return out;
    }

    const double dt = timestamp_s - prev_t;
    const double east0 = prev_east;
    const double north0 = prev_north;
    prev_east = east_m;
    prev_north = north_m;
    prev_t = timestamp_s;

    if (dt < kMinDtSec || dt > kMaxDtSec) {
      x_dot_f = 0.0;
      y_dot_f = 0.0;
      return out;
    }

    const double x_raw = (east_m - east0) / dt;
    const double y_raw = (north_m - north0) / dt;
    x_dot_f = kAlpha * x_dot_f + (1.0 - kAlpha) * x_raw;
    y_dot_f = kAlpha * y_dot_f + (1.0 - kAlpha) * y_raw;

    out.x_dot = x_dot_f;
    out.y_dot = y_dot_f;
    enu_vel_to_body(out.x_dot, out.y_dot, psi, out.u, out.v);
    out.vel_ok = true;
    return out;
  }
};

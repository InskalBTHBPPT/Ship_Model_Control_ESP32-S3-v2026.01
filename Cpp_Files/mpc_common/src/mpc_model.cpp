#include "mpc_model.hpp"

#include <algorithm>
#include <cstring>

namespace mpc {

static double mat2x2_det(const double m[4]) {
  return m[0] * m[3] - m[1] * m[2];
}

static void build_a_sys_b_sys(const ShipParams &p, double a_sys[4],
                              double b_sys[2]) {
  const double L = p.L, B = p.B, T = p.T;
  const double C_B = p.C_B, x_G = p.x_G;
  const double rho = p.rho;
  const double u_0_nd = p.u_0_nd;

  const double Y_v_dot =
      -(1 + 0.16 * C_B * B / T - 5.1 * (B / L) * (B / L)) * kPi * (T / L) * (T / L);
  const double Y_r_dot =
      -(0.67 * (B / L) - 0.0033 * (B / T) * (B / T)) * kPi * (T / L) * (T / L);
  const double N_v_dot =
      -(1.1 * B / L - 0.041 * B / T) * kPi * (T / L) * (T / L);
  const double N_r_dot =
      -((1 / 12.0) + 0.017 * C_B * B / T - 0.33 * B / L) * kPi * (T / L) * (T / L);
  const double Y_v = -(1 + 0.4 * C_B * B / T) * kPi * (T / L) * (T / L);
  const double Y_r = -(-0.5 + 2.2 * B / L - 0.08 * B / T) * kPi * (T / L) * (T / L);
  const double N_v = -(0.5 + 2.4 * T / L) * kPi * (T / L) * (T / L);
  const double N_r =
      -(0.25 + 0.039 * B / T - 0.56 * B / L) * kPi * (T / L) * (T / L);

  const double m_nd = 2 * p.m / (rho * L * L * L);
  const double x_G_nd = x_G / L;
  const double I_z_nd = p.I_z_nd;

  const double m00 = m_nd - Y_v_dot;
  const double m01 = m_nd * x_G_nd - Y_r_dot;
  const double m10 = m_nd * x_G_nd - N_v_dot;
  const double m11 = I_z_nd - N_r_dot;
  const double det_m = m00 * m11 - m01 * m10;

  const double a11 =
      ((I_z_nd - N_r_dot) * Y_v - (m_nd * x_G_nd - Y_r_dot) * N_v) / det_m;
  const double a12 =
      ((I_z_nd - N_r_dot) * (Y_r - m_nd * u_0_nd) -
       (m_nd * x_G_nd - Y_r_dot) * (N_r - m_nd * x_G_nd * u_0_nd)) /
      det_m;
  const double a21 =
      ((m_nd - Y_v_dot) * N_v - (m_nd * x_G_nd - N_v_dot) * Y_v) / det_m;
  const double a22 =
      ((m_nd - Y_v_dot) * (N_r - m_nd * x_G_nd * u_0_nd) -
       (m_nd * x_G_nd - N_v_dot) * (Y_r - m_nd * u_0_nd)) /
      det_m;

  a_sys[0] = a11;
  a_sys[1] = a12;
  a_sys[2] = a21;
  a_sys[3] = a22;
  b_sys[0] = 0.01;
  b_sys[1] = 1.0;
}

LinearShipModel build_linear_ship_model(const ShipParams &ship,
                                        const MPCConfig &cfg) {
  LinearShipModel model;
  double a_sys[4], b_sys[2];
  build_a_sys_b_sys(ship, a_sys, b_sys);

  model.a_lin.assign(25, 0.0);
  model.a_lin[0] = a_sys[0];
  model.a_lin[1] = a_sys[1];
  model.a_lin[5] = a_sys[2];
  model.a_lin[6] = a_sys[3];
  model.a_lin[13] = 1.0;
  model.a_lin[19] = ship.u_0_nd;
  model.a_lin[24] = 1.0;

  model.b_lin.assign(5, 0.0);
  model.b_lin[0] = b_sys[0];
  model.b_lin[1] = b_sys[1];

  model.d_affine.assign(5, 0.0);
  model.d_affine[2] = ship.u_0_nd;

  const double dt_nd = cfg.dt_pred() * (ship.u_0 / ship.L);
  model.ad.assign(25, 0.0);
  model.bd.assign(5, 0.0);
  model.dd.assign(5, 0.0);
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < 5; ++j) {
      model.ad[static_cast<size_t>(i) * 5 + j] =
          (i == j ? 1.0 : 0.0) + model.a_lin[static_cast<size_t>(i) * 5 + j] * dt_nd;
    }
    model.bd[i] = model.b_lin[i] * dt_nd;
    model.dd[i] = model.d_affine[i] * dt_nd;
  }

  model.q_full.assign(25, 0.0);
  model.q_full[0] = 0.0;
  model.q_full[6] = 0.0;
  model.q_full[12] = cfg.q_diag[0];
  model.q_full[18] = cfg.q_diag[1];
  model.q_full[24] = cfg.q_diag[2];

  model.r_weight = cfg.r_weight;
  model.r_limit_nd = cfg.r_limit * (ship.L / ship.u_0);
  model.u_limit = cfg.u_limit_deg * kPi / 180.0;
  model.u_rate_limit = cfg.u_rate_limit_deg * kPi / 180.0 * cfg.dt_pred();

  return model;
}

void dimensional_to_nondimensional(const double x_dim[5], double x_nd[5],
                                   double L, double u0) {
  x_nd[0] = x_dim[0] / u0;
  x_nd[1] = x_dim[1] * L / u0;
  x_nd[2] = x_dim[2] / L;
  x_nd[3] = x_dim[3] / L;
  x_nd[4] = x_dim[4];
}

void nondimensional_to_dimensional(const double x_nd[5], double x_dim[5],
                                   double L, double u0) {
  x_dim[0] = x_nd[0] * u0;
  x_dim[1] = x_nd[1] * u0 / L;
  x_dim[2] = x_nd[2] * L;
  x_dim[3] = x_nd[3] * L;
  x_dim[4] = x_nd[4];
}

static void linear_dynamics(const double s[5], double u,
                            const LinearShipModel &model, double s_dot[5]) {
  for (int i = 0; i < 5; ++i) {
    s_dot[i] = model.d_affine[i];
    for (int j = 0; j < 5; ++j) {
      s_dot[i] += model.a_lin[i * 5 + j] * s[j];
    }
    s_dot[i] += model.b_lin[i] * u;
  }
}

void rk4_step_linear(const double s[5], double u, double dt_nd,
                     const LinearShipModel &model, double s_next[5]) {
  double k1[5], k2[5], k3[5], k4[5], tmp[5];
  linear_dynamics(s, u, model, k1);
  for (int i = 0; i < 5; ++i) {
    tmp[i] = s[i] + 0.5 * dt_nd * k1[i];
  }
  linear_dynamics(tmp, u, model, k2);
  for (int i = 0; i < 5; ++i) {
    tmp[i] = s[i] + 0.5 * dt_nd * k2[i];
  }
  linear_dynamics(tmp, u, model, k3);
  for (int i = 0; i < 5; ++i) {
    tmp[i] = s[i] + dt_nd * k3[i];
  }
  linear_dynamics(tmp, u, model, k4);
  for (int i = 0; i < 5; ++i) {
    s_next[i] =
        s[i] + (dt_nd / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
  }
}

}  // namespace mpc

#pragma once

#include <array>
#include <cmath>
#include <vector>

namespace mpc {

constexpr double kPi = 3.14159265358979323846;
constexpr double kRadToDeg = 57.29577951308232;

struct ShipParams {
  double L = 101.07;
  double B = 14.0;
  double T = 3.7;
  double m = 2423e3;
  double u_0 = 15.4;
  double C_B = 0.65;
  double x_G = 5.25;
  double rho = 1024.0;
  double I_z_nd = 1.2392e-4;
  double u_0_nd = 1.0;
};

struct MPCConfig {
  double tp = 60.0;
  int np_steps = 60;
  double t_sim = 1.0;
  double t_sim_total = 150.0;  // 150 langkah simulasi (Cpp_MPC_150)
  std::array<double, 3> q_diag = {50.0, 50.0, 50.0};
  double r_weight = 100.0;
  double r_limit = 0.0932;
  double u_limit_deg = 35.0;
  double u_rate_limit_deg = 5.0;

  double dt_pred() const { return tp / static_cast<double>(np_steps); }
};

struct LinearShipModel {
  int n_state = 5;
  std::vector<double> a_lin;  // 5x5 row-major
  std::vector<double> b_lin;  // 5
  std::vector<double> d_affine;  // 5
  std::vector<double> ad;  // 5x5
  std::vector<double> bd;  // 5
  std::vector<double> dd;  // 5
  std::vector<double> q_full;  // 5x5
  double r_weight = 100.0;
  double r_limit_nd = 0.0;
  double u_limit = 0.0;
  double u_rate_limit = 0.0;
};

LinearShipModel build_linear_ship_model(const ShipParams &ship, const MPCConfig &cfg);

void dimensional_to_nondimensional(const double x_dim[5], double x_nd[5],
                                   double L, double u0);
void nondimensional_to_dimensional(const double x_nd[5], double x_dim[5],
                                   double L, double u0);

void rk4_step_linear(const double s[5], double u, double dt_nd,
                     const LinearShipModel &model, double s_next[5]);

}  // namespace mpc

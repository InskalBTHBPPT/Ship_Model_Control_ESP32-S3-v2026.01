// LMPC konvensional — simulasi 150 langkah + waktu komputasi
#include "mpc_model.hpp"
#include "mpc_qp.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr double kRadToDeg = mpc::kRadToDeg;

double rmse(const std::vector<double> &a, const std::vector<double> &b) {
  double sum = 0.0;
  const size_t n = std::min(a.size(), b.size());
  for (size_t i = 0; i < n; ++i) {
    const double d = a[i] - b[i];
    sum += d * d;
  }
  return n > 0 ? std::sqrt(sum / static_cast<double>(n)) : 0.0;
}

}  // namespace

int main() {
  mpc::ShipParams ship;
  mpc::MPCConfig cfg;
  cfg.t_sim_total = 150.0;

  mpc::LinearShipModel model =
      mpc::build_linear_ship_model(ship, cfg);
  mpc::MPCQPProblem qp =
      mpc::build_mpc_qp_problem(model, cfg.np_steps, cfg.dt_pred());

  double x0_dim[5] = {0.0, 0.0, 0.0, 500.0, 0.0};
  double x0_nd[5];
  mpc::dimensional_to_nondimensional(x0_dim, x0_nd, ship.L, ship.u_0);
  const double h_ref[3] = {0.0, 0.0, 0.0};

  const int num_steps = static_cast<int>(cfg.t_sim_total / cfg.t_sim) + 1;
  const double dt_nd_sim = cfg.t_sim * ship.u_0 / ship.L;

  std::vector<std::vector<double>> history_state_nd;
  std::vector<double> history_input;
  std::vector<double> x_nd(5);
  for (int i = 0; i < 5; ++i) {
    x_nd[i] = x0_nd[i];
  }

  double u_prev = 0.0;
  std::vector<double> s_ref, f, b_eq, b_ineq, z_opt;
  std::string status;

  std::vector<double> z_warm;

  std::cout << "Memulai simulasi Linear MPC (" << num_steps << " langkah, Np="
            << cfg.np_steps << ")...\n";

  const auto t0 = std::chrono::high_resolution_clock::now();

  for (int step = 0; step < num_steps; ++step) {
    const double t = step * cfg.t_sim;
    mpc::build_reference_vector(qp, t, h_ref, ship.u_0, ship.L, s_ref);
    mpc::gradient(qp, s_ref, f);
    mpc::equality_rhs(qp, x_nd.data(), b_eq);
    mpc::inequality_rhs(qp, u_prev, b_ineq);

    if (!mpc::solve_mpc_step(qp, f, b_eq, b_ineq, z_opt, status,
                             z_warm.empty() ? nullptr : &z_warm, &z_warm)) {
      std::cerr << "QP gagal pada t=" << t << " (" << status << ")\n";
      z_opt.assign(qp.n_var, 0.0);
    }

    const double u_apply = z_opt[static_cast<size_t>(qp.idx_u0)];
    u_prev = u_apply;
    history_state_nd.push_back(x_nd);
    history_input.push_back(u_apply);

    double x_next[5];
    mpc::rk4_step_linear(x_nd.data(), u_apply, dt_nd_sim, model, x_next);
    for (int i = 0; i < 5; ++i) {
      x_nd[i] = x_next[i];
    }
  }

  const auto t1 = std::chrono::high_resolution_clock::now();
  const double elapsed_s =
      std::chrono::duration<double>(t1 - t0).count();
  const double elapsed_ms = elapsed_s * 1000.0;
  const double avg_ms = elapsed_ms / static_cast<double>(num_steps);

  std::vector<double> x_ref_full(num_steps);
  std::vector<double> y_ref_full(num_steps);
  std::vector<double> psi_ref_full(num_steps);
  std::vector<double> x_traj(num_steps);
  std::vector<double> y_traj(num_steps);
  std::vector<double> psi_traj(num_steps);

  for (int step = 0; step < num_steps; ++step) {
    const double t = step * cfg.t_sim;
    x_ref_full[step] = h_ref[0] + t * ship.u_0;
    y_ref_full[step] = h_ref[1];
    psi_ref_full[step] = h_ref[2];
    double x_dim[5];
    mpc::nondimensional_to_dimensional(history_state_nd[step].data(), x_dim,
                                     ship.L, ship.u_0);
    x_traj[step] = x_dim[2];
    y_traj[step] = x_dim[3];
    psi_traj[step] = x_dim[4];
  }

  std::cout << "\nSIMULASI SELESAI\n";
  std::cout << "Total Waktu Komputasi : " << elapsed_s << " detik\n";
  std::cout << "Total Waktu (ms)      : " << elapsed_ms << " ms\n";
  std::cout << "Rata-rata per step    : " << avg_ms << " ms\n";

  std::cout << "\n--- HASIL PERHITUNGAN RMSE ---\n";
  std::cout << "RMSE X   : " << rmse(x_traj, x_ref_full) << " meter\n";
  std::cout << "RMSE Y   : " << rmse(y_traj, y_ref_full) << " meter\n";
  std::cout << "RMSE Psi : " << rmse(psi_traj, psi_ref_full) << " rad ("
            << rmse(psi_traj, psi_ref_full) * kRadToDeg << " derajat)\n";

  if (!history_input.empty()) {
    std::cout << "\nRudder pertama (deg): "
              << history_input.front() * kRadToDeg << "\n";
    std::cout << "Rudder akhir (deg)  : "
              << history_input.back() * kRadToDeg << "\n";
  }

  return 0;
}

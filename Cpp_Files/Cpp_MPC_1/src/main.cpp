// LMPC konvensional — satu solve QP + waktu komputasi
#include "mpc_model.hpp"
#include "mpc_qp.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr double kRadToDeg = mpc::kRadToDeg;

void printState(const char *label, const double s[5]) {
  std::cout << label << " v=" << s[0] << " r=" << s[1]
            << " x=" << s[2] << " y=" << s[3]
            << " psi=" << (s[4] * kRadToDeg) << " deg\n";
}

}  // namespace

int main() {
  mpc::ShipParams ship;
  mpc::MPCConfig cfg;

  mpc::LinearShipModel model = mpc::build_linear_ship_model(ship, cfg);
  mpc::MPCQPProblem qp =
      mpc::build_mpc_qp_problem(model, cfg.np_steps, cfg.dt_pred());

  double x0_dim[5] = {0.0, 0.0, 0.0, 500.0, 0.0};
  double x0_nd[5];
  mpc::dimensional_to_nondimensional(x0_dim, x0_nd, ship.L, ship.u_0);
  const double h_ref[3] = {0.0, 0.0, 0.0};

  std::vector<double> s_ref, f, b_eq, b_ineq, z_opt;
  std::string status;

  mpc::build_reference_vector(qp, 0.0, h_ref, ship.u_0, ship.L, s_ref);
  mpc::gradient(qp, s_ref, f);
  mpc::equality_rhs(qp, x0_nd, b_eq);
  mpc::inequality_rhs(qp, 0.0, b_ineq);

  std::cout << "Memulai satu solve Linear MPC (Np=" << cfg.np_steps << ")...\n";

  const auto t0 = std::chrono::high_resolution_clock::now();
  const bool ok = mpc::solve_mpc_step(qp, f, b_eq, b_ineq, z_opt, status, nullptr,
                                      nullptr);
  const auto t1 = std::chrono::high_resolution_clock::now();

  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();

  std::cout << "\nSOLVE SELESAI\n";
  std::cout << "Status              : " << status << "\n";
  std::cout << "Waktu Komputasi     : " << elapsed_ms << " ms\n";
  if (elapsed_ms > 0.0) {
    std::cout << "Estimasi rate       : " << (1000.0 / elapsed_ms) << " Hz\n";
  }

  if (ok && static_cast<int>(z_opt.size()) > qp.idx_u0) {
    const double u = z_opt[static_cast<size_t>(qp.idx_u0)];
    std::cout << "Rudder (rad)        : " << u << "\n";
    std::cout << "Rudder (deg)        : " << (u * kRadToDeg) << "\n";

    const double dt_nd_sim = cfg.t_sim * ship.u_0 / ship.L;
    double x_next[5];
    mpc::rk4_step_linear(x0_nd, u, dt_nd_sim, model, x_next);

    double before[5], after[5];
    mpc::nondimensional_to_dimensional(x0_nd, before, ship.L, ship.u_0);
    mpc::nondimensional_to_dimensional(x_next, after, ship.L, ship.u_0);
    std::cout << "\n--- State ---\n";
    printState("Sebelum", before);
    printState("Sesudah (dt=1s)", after);
  }

  return ok ? 0 : 1;
}

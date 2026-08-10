#pragma once

#include "mpc_model.hpp"
#include <string>
#include <vector>

namespace mpc {

struct MPCQPProblem {
  int np_steps = 0;
  int n_state = 5;
  int n_var = 0;
  int idx_u0 = 0;
  int n_red = 0;
  double dt_pred = 1.0;
  double u_rate_limit = 0.0;

  std::vector<double> H;
  std::vector<double> H_s;
  std::vector<double> a_eq;
  std::vector<double> a_ineq;
  std::vector<double> b_eq_base;
  std::vector<double> lb;
  std::vector<double> ub;

  std::vector<double> M_chol;
  std::vector<double> Y;
  std::vector<double> H_red;

  MPCQPProblem() = default;
  MPCQPProblem(MPCQPProblem &&) = default;
  MPCQPProblem &operator=(MPCQPProblem &&) = default;
  MPCQPProblem(const MPCQPProblem &) = delete;
  MPCQPProblem &operator=(const MPCQPProblem &) = delete;
};

MPCQPProblem build_mpc_qp_problem(const LinearShipModel &model, int np_steps,
                                  double dt_pred);

void build_reference_vector(const MPCQPProblem &problem, double t,
                            const double h_ref[3], double u_0, double L,
                            std::vector<double> &s_ref_flat);

void gradient(const MPCQPProblem &problem, const std::vector<double> &s_ref,
              std::vector<double> &f);

void equality_rhs(const MPCQPProblem &problem, const double x0_nd[5],
                  std::vector<double> &b_eq);

void inequality_rhs(const MPCQPProblem &problem, double u_prev,
                    std::vector<double> &b_ineq);

bool solve_mpc_step(const MPCQPProblem &problem, const std::vector<double> &f,
                    const std::vector<double> &b_eq,
                    const std::vector<double> &b_ineq, std::vector<double> &z_opt,
                    std::string &status, const std::vector<double> *z_warm,
                    std::vector<double> *z_warm_out);

}  // namespace mpc

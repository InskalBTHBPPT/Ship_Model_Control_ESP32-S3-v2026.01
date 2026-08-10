#include "mpc_qp.hpp"

#include "qp_solver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mpc {

static void kron_eye_block(const std::vector<double> &block, int n_block,
                           int n_rep, std::vector<double> &out) {
  const int dim = n_block * n_rep;
  out.assign(static_cast<size_t>(dim) * dim, 0.0);
  for (int k = 0; k < n_rep; ++k) {
    for (int i = 0; i < n_block; ++i) {
      for (int j = 0; j < n_block; ++j) {
        out[static_cast<size_t>(k * n_block + i) * dim + (k * n_block + j)] =
            block[static_cast<size_t>(i) * n_block + j];
      }
    }
  }
}

static void project_onto_null(const MPCQPProblem &problem,
                              std::vector<double> &v) {
  const int meq = static_cast<int>(problem.b_eq_base.size());
  const int n = problem.n_var;
  std::vector<double> Av(static_cast<size_t>(meq), 0.0);
  for (int r = 0; r < meq; ++r) {
    double sum = 0.0;
    for (int c = 0; c < n; ++c) {
      sum += problem.a_eq[static_cast<size_t>(r) * n + c] * v[c];
    }
    Av[r] = sum;
  }
  std::vector<double> w;
  chol_solve_lower(problem.M_chol, meq, Av, w);
  for (int c = 0; c < n; ++c) {
    double corr = 0.0;
    for (int r = 0; r < meq; ++r) {
      corr += problem.a_eq[static_cast<size_t>(r) * n + c] * w[r];
    }
    v[c] -= corr;
  }
}

static void build_null_basis(MPCQPProblem &problem) {
  const int n = problem.n_var;
  const int meq = static_cast<int>(problem.b_eq_base.size());
  problem.n_red = n - meq;
  problem.Y.assign(static_cast<size_t>(n) * problem.n_red, 0.0);

  std::vector<std::vector<double>> basis;
  for (int k = 0; k < problem.n_red; ++k) {
    std::vector<double> v(n, 0.0);
    v[static_cast<size_t>(problem.idx_u0 + k)] = 1.0;
    project_onto_null(problem, v);
    for (const auto &prev : basis) {
      double dot = 0.0;
      for (int i = 0; i < n; ++i) {
        dot += v[i] * prev[i];
      }
      for (int i = 0; i < n; ++i) {
        v[i] -= dot * prev[i];
      }
    }
    double norm = 0.0;
    for (double c : v) {
      norm += c * c;
    }
    norm = std::sqrt(norm);
    if (norm < 1e-10) {
      continue;
    }
    for (int i = 0; i < n; ++i) {
      v[i] /= norm;
    }
    basis.push_back(v);
  }

  for (int col = 0; col < static_cast<int>(basis.size()); ++col) {
    for (int row = 0; row < n; ++row) {
      problem.Y[static_cast<size_t>(row) * problem.n_red + col] = basis[col][row];
    }
  }
  problem.n_red = static_cast<int>(basis.size());
}

MPCQPProblem build_mpc_qp_problem(const LinearShipModel &model, int np_steps,
                                  double dt_pred) {
  MPCQPProblem problem;
  const int n_state = model.n_state;
  const int n_var = n_state * (np_steps + 1) + np_steps;

  problem.np_steps = np_steps;
  problem.n_state = n_state;
  problem.n_var = n_var;
  problem.idx_u0 = n_state * (np_steps + 1);
  problem.dt_pred = dt_pred;
  problem.u_rate_limit = model.u_rate_limit;

  std::vector<double> h_s;
  kron_eye_block(model.q_full, n_state, np_steps + 1, h_s);
  std::vector<double> h_u(static_cast<size_t>(np_steps) * np_steps, 0.0);
  for (int i = 0; i < np_steps; ++i) {
    h_u[static_cast<size_t>(i) * np_steps + i] = model.r_weight;
  }

  problem.H.assign(static_cast<size_t>(n_var) * n_var, 0.0);
  for (int i = 0; i < static_cast<int>(h_s.size()); ++i) {
    problem.H[i] = h_s[i];
  }
  for (int i = 0; i < np_steps; ++i) {
    for (int j = 0; j < np_steps; ++j) {
      problem.H[static_cast<size_t>(problem.idx_u0 + i) * n_var +
                (problem.idx_u0 + j)] = h_u[static_cast<size_t>(i) * np_steps + j];
    }
  }
  for (int i = 0; i < n_var; ++i) {
    problem.H[static_cast<size_t>(i) * n_var + i] += 1e-6;
  }
  problem.H_s = h_s;

  const int meq = n_state * (np_steps + 1);
  problem.a_eq.assign(static_cast<size_t>(meq) * n_var, 0.0);
  for (int i = 0; i < np_steps + 1; ++i) {
    for (int d = 0; d < n_state; ++d) {
      problem.a_eq[static_cast<size_t>(i * n_state + d) * n_var +
                   (i * n_state + d)] = 1.0;
    }
  }
  for (int i = 1; i <= np_steps; ++i) {
  for (int r = 0; r < n_state; ++r) {
    for (int c = 0; c < n_state; ++c) {
      problem.a_eq[static_cast<size_t>(i * n_state + r) * n_var +
                   ((i - 1) * n_state + c)] -= model.ad[static_cast<size_t>(r) * n_state + c];
    }
  }
  for (int r = 0; r < n_state; ++r) {
    problem.a_eq[static_cast<size_t>(i * n_state + r) * n_var +
                 (problem.idx_u0 + (i - 1))] -= model.bd[r];
  }
  }

  problem.b_eq_base.assign(meq, 0.0);
  for (int i = 0; i < np_steps; ++i) {
    for (int r = 0; r < n_state; ++r) {
      problem.b_eq_base[static_cast<size_t>((i + 1) * n_state + r)] = model.dd[r];
    }
  }

  problem.a_ineq.assign(static_cast<size_t>(2 * np_steps) * n_var, 0.0);
  for (int i = 0; i < np_steps; ++i) {
    problem.a_ineq[static_cast<size_t>(i) * n_var + (problem.idx_u0 + i)] = 1.0;
    if (i > 0) {
      problem.a_ineq[static_cast<size_t>(i) * n_var + (problem.idx_u0 + i - 1)] =
          -1.0;
    }
    problem.a_ineq[static_cast<size_t>(np_steps + i) * n_var +
                   (problem.idx_u0 + i)] = -1.0;
    if (i > 0) {
      problem.a_ineq[static_cast<size_t>(np_steps + i) * n_var +
                   (problem.idx_u0 + i - 1)] = 1.0;
    }
  }

  problem.lb.assign(n_var, 0.0);
  problem.ub.assign(n_var, 0.0);
  const double lb_r = -model.r_limit_nd;
  const double ub_r = model.r_limit_nd;
  for (int i = 0; i <= np_steps; ++i) {
    problem.lb[static_cast<size_t>(i * n_state + 0)] =
        -std::numeric_limits<double>::infinity();
    problem.lb[static_cast<size_t>(i * n_state + 1)] = lb_r;
    problem.lb[static_cast<size_t>(i * n_state + 2)] =
        -std::numeric_limits<double>::infinity();
    problem.lb[static_cast<size_t>(i * n_state + 3)] =
        -std::numeric_limits<double>::infinity();
    problem.lb[static_cast<size_t>(i * n_state + 4)] =
        -std::numeric_limits<double>::infinity();

    problem.ub[static_cast<size_t>(i * n_state + 0)] =
        std::numeric_limits<double>::infinity();
    problem.ub[static_cast<size_t>(i * n_state + 1)] = ub_r;
    problem.ub[static_cast<size_t>(i * n_state + 2)] =
        std::numeric_limits<double>::infinity();
    problem.ub[static_cast<size_t>(i * n_state + 3)] =
        std::numeric_limits<double>::infinity();
    problem.ub[static_cast<size_t>(i * n_state + 4)] =
        std::numeric_limits<double>::infinity();
  }
  for (int i = 0; i < np_steps; ++i) {
    problem.lb[static_cast<size_t>(problem.idx_u0 + i)] = -model.u_limit;
    problem.ub[static_cast<size_t>(problem.idx_u0 + i)] = model.u_limit;
  }

  std::vector<double> M(static_cast<size_t>(meq) * meq, 0.0);
  for (int i = 0; i < meq; ++i) {
    for (int j = 0; j < meq; ++j) {
      double sum = 0.0;
      for (int c = 0; c < n_var; ++c) {
        sum +=
            problem.a_eq[static_cast<size_t>(i) * n_var + c] *
            problem.a_eq[static_cast<size_t>(j) * n_var + c];
      }
      M[static_cast<size_t>(i) * meq + j] = sum;
    }
  }
  problem.M_chol = M;
  if (!chol_decompose(problem.M_chol, meq)) {
    for (int i = 0; i < meq; ++i) {
      problem.M_chol[static_cast<size_t>(i) * meq + i] += 1e-8;
    }
    chol_decompose(problem.M_chol, meq);
  }

  build_null_basis(problem);

  std::vector<double> YtH;
  mat_transpose_mul(problem.Y, n_var, problem.n_red, problem.H, n_var, YtH);
  mat_mul(YtH, problem.n_red, n_var, problem.Y, problem.n_red, problem.H_red);

  return problem;
}

void build_reference_vector(const MPCQPProblem &problem, double t,
                            const double h_ref[3], double u_0, double L,
                            std::vector<double> &s_ref_flat) {
  const int n_state = problem.n_state;
  const int blocks = problem.np_steps + 1;
  s_ref_flat.assign(static_cast<size_t>(n_state) * blocks, 0.0);
  for (int k = 0; k < blocks; ++k) {
    const double t_pred = t + k * problem.dt_pred;
    s_ref_flat[static_cast<size_t>(k * n_state + 2)] =
        (h_ref[0] + t_pred * u_0) / L;
    s_ref_flat[static_cast<size_t>(k * n_state + 3)] = h_ref[1] / L;
    s_ref_flat[static_cast<size_t>(k * n_state + 4)] = h_ref[2];
  }
}

void gradient(const MPCQPProblem &problem, const std::vector<double> &s_ref,
              std::vector<double> &f) {
  const int n_s = static_cast<int>(s_ref.size());
  std::vector<double> f_s;
  mat_mul(problem.H_s, n_s, n_s, s_ref, 1, f_s);
  f.assign(problem.n_var, 0.0);
  for (int i = 0; i < n_s; ++i) {
    f[i] = -f_s[i];
  }
}

void equality_rhs(const MPCQPProblem &problem, const double x0_nd[5],
                  std::vector<double> &b_eq) {
  b_eq = problem.b_eq_base;
  for (int i = 0; i < problem.n_state; ++i) {
    b_eq[i] = x0_nd[i];
  }
}

void inequality_rhs(const MPCQPProblem &problem, double u_prev,
                    std::vector<double> &b_ineq) {
  const int np = problem.np_steps;
  b_ineq.assign(2 * np, problem.u_rate_limit);
  b_ineq[0] = problem.u_rate_limit + u_prev;
  b_ineq[np] = problem.u_rate_limit - u_prev;
}

bool solve_mpc_step(const MPCQPProblem &problem, const std::vector<double> &f,
                    const std::vector<double> &b_eq,
                    const std::vector<double> &b_ineq, std::vector<double> &z_opt,
                    std::string &status, const std::vector<double> *z_warm,
                    std::vector<double> *z_red_out) {
  const int n = problem.n_var;
  const int meq = static_cast<int>(b_eq.size());
  const int m_ineq = static_cast<int>(problem.a_ineq.size()) / n;
  const int n_red = problem.n_red;
  const int n_state = problem.n_state;

  std::vector<double> w;
  chol_solve_lower(problem.M_chol, meq, b_eq, w);
  std::vector<double> x_part(n, 0.0);
  for (int c = 0; c < n; ++c) {
    double sum = 0.0;
    for (int r = 0; r < meq; ++r) {
      sum += problem.a_eq[static_cast<size_t>(r) * n + c] * w[r];
    }
    x_part[c] = sum;
  }

  std::vector<double> Hx;
  mat_mul(problem.H, n, n, x_part, 1, Hx);
  std::vector<double> f_full(n, 0.0);
  for (int i = 0; i < n; ++i) {
    f_full[i] = f[i] + Hx[i];
  }

  std::vector<double> f_red;
  mat_transpose_mul(problem.Y, n, n_red, f_full, 1, f_red);

  std::vector<std::vector<double>> G_rows;
  std::vector<double> h;

  for (int i = 0; i < m_ineq; ++i) {
    std::vector<double> row(n_red, 0.0);
    for (int j = 0; j < n_red; ++j) {
      double sum = 0.0;
      for (int c = 0; c < n; ++c) {
        sum += problem.a_ineq[static_cast<size_t>(i) * n + c] *
               problem.Y[static_cast<size_t>(c) * n_red + j];
      }
      row[j] = sum;
    }
    G_rows.push_back(row);
    double val = 0.0;
    for (int c = 0; c < n; ++c) {
      val += problem.a_ineq[static_cast<size_t>(i) * n + c] * x_part[c];
    }
    h.push_back(b_ineq[i] - val);
  }

  for (int block = 0; block <= problem.np_steps; ++block) {
    const int r_idx = block * n_state + 1;
    std::vector<double> row_lb(n_red, 0.0);
    std::vector<double> row_ub(n_red, 0.0);
    for (int j = 0; j < n_red; ++j) {
      row_lb[j] = -problem.Y[static_cast<size_t>(r_idx) * n_red + j];
      row_ub[j] = problem.Y[static_cast<size_t>(r_idx) * n_red + j];
    }
    G_rows.push_back(row_lb);
    h.push_back(-problem.lb[r_idx] + x_part[r_idx]);
    G_rows.push_back(row_ub);
    h.push_back(problem.ub[r_idx] - x_part[r_idx]);
  }

  for (int i = 0; i < problem.np_steps; ++i) {
    const int u_idx = problem.idx_u0 + i;
    std::vector<double> row_lb(n_red, 0.0);
    std::vector<double> row_ub(n_red, 0.0);
    for (int j = 0; j < n_red; ++j) {
      row_lb[j] = -problem.Y[static_cast<size_t>(u_idx) * n_red + j];
      row_ub[j] = problem.Y[static_cast<size_t>(u_idx) * n_red + j];
    }
    G_rows.push_back(row_lb);
    h.push_back(-problem.lb[u_idx] + x_part[u_idx]);
    G_rows.push_back(row_ub);
    h.push_back(problem.ub[u_idx] - x_part[u_idx]);
  }

  const int m_total = static_cast<int>(G_rows.size());
  std::vector<double> G_flat(static_cast<size_t>(m_total) * n_red, 0.0);
  for (int i = 0; i < m_total; ++i) {
    for (int j = 0; j < n_red; ++j) {
      G_flat[static_cast<size_t>(i) * n_red + j] = G_rows[i][j];
    }
  }

  std::vector<double> z(n_red, 0.0);
  if (z_warm && static_cast<int>(z_warm->size()) == n_red) {
    z = *z_warm;
  }

  if (!solve_qp_active_set(problem.H_red, n_red, f_red, G_flat, m_total, h, z,
                           80)) {
    status = "fail:active_set";
    z_opt.assign(n, 0.0);
    return false;
  }

  z_opt = x_part;
  for (int c = 0; c < n; ++c) {
    for (int j = 0; j < n_red; ++j) {
      z_opt[c] += problem.Y[static_cast<size_t>(c) * n_red + j] * z[j];
    }
  }

  status = "ok";
  if (z_red_out) {
    *z_red_out = z;
  }
  return true;
}

}  // namespace mpc

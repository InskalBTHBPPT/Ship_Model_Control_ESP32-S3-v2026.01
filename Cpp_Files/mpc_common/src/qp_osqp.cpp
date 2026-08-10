#include "qp_osqp.hpp"

#include <osqp.h>

#include <cmath>

namespace mpc {

static void dense_to_csc_upper(const std::vector<double> &dense, int n,
                               std::vector<c_float> &x, std::vector<c_int> &i,
                               std::vector<c_int> &p) {
  p.assign(static_cast<size_t>(n) + 1, 0);
  i.clear();
  x.clear();
  for (int col = 0; col < n; ++col) {
    for (int row = 0; row <= col; ++row) {
      const double val = dense[static_cast<size_t>(row) * n + col];
      if (std::fabs(val) > 1e-16) {
        i.push_back(row);
        x.push_back(static_cast<c_float>(val));
      }
    }
    p[static_cast<size_t>(col) + 1] = static_cast<c_int>(i.size());
  }
}

static void dense_to_csc(const std::vector<double> &dense, int rows, int cols,
                         std::vector<c_float> &x, std::vector<c_int> &i,
                         std::vector<c_int> &p) {
  p.assign(static_cast<size_t>(cols) + 1, 0);
  i.clear();
  x.clear();
  for (int col = 0; col < cols; ++col) {
    for (int row = 0; row < rows; ++row) {
      const double val = dense[static_cast<size_t>(row) * cols + col];
      if (std::fabs(val) > 1e-16) {
        i.push_back(row);
        x.push_back(static_cast<c_float>(val));
      }
    }
    p[static_cast<size_t>(col) + 1] = static_cast<c_int>(i.size());
  }
}

bool solve_qp_osqp_reduced(const std::vector<double> &H, int n,
                           const std::vector<double> &f,
                           const std::vector<double> &G, int m,
                           const std::vector<double> &h, std::vector<double> &x,
                           std::string &status) {
  std::vector<c_float> P_x, A_x;
  std::vector<c_int> P_i, P_p, A_i, A_p;
  dense_to_csc_upper(H, n, P_x, P_i, P_p);
  dense_to_csc(G, m, n, A_x, A_i, A_p);

  csc *P_csc =
      csc_matrix(n, n, static_cast<c_int>(P_x.size()), P_x.data(), P_i.data(),
                 P_p.data());
  csc *A_csc =
      csc_matrix(m, n, static_cast<c_int>(A_x.size()), A_x.data(), A_i.data(),
                 A_p.data());

  std::vector<c_float> q(n);
  for (int i = 0; i < n; ++i) {
    q[i] = static_cast<c_float>(f[i]);
  }

  const c_float neg_inf = static_cast<c_float>(-OSQP_INFTY);
  std::vector<c_float> l(m, neg_inf);
  std::vector<c_float> u(m);
  for (int i = 0; i < m; ++i) {
    u[i] = static_cast<c_float>(h[i]);
  }

  OSQPData data{};
  data.n = n;
  data.m = m;
  data.P = P_csc;
  data.A = A_csc;
  data.q = q.data();
  data.l = l.data();
  data.u = u.data();

  OSQPSettings settings;
  osqp_set_default_settings(&settings);
  settings.verbose = 0;
  settings.polish = 1;
  settings.max_iter = 4000;
  settings.eps_abs = 1e-6;
  settings.eps_rel = 1e-6;

  OSQPWorkspace *work = nullptr;
  const c_int setup_status = osqp_setup(&work, &data, &settings);
  if (setup_status != 0 || work == nullptr) {
    c_free(P_csc);
    c_free(A_csc);
    status = "fail:osqp_setup";
    x.assign(n, 0.0);
    return false;
  }

  const c_int solve_status = osqp_solve(work);
  bool ok = false;
  if (solve_status == 0 &&
      (work->info->status_val == OSQP_SOLVED ||
       work->info->status_val == OSQP_SOLVED_INACCURATE)) {
    x.assign(n, 0.0);
    for (int i = 0; i < n; ++i) {
      x[i] = work->solution->x[i];
    }
    status = "ok";
    ok = true;
  } else {
    status = std::string("fail:") + work->info->status;
    x.assign(n, 0.0);
  }

  osqp_cleanup(work);
  c_free(P_csc);
  c_free(A_csc);
  return ok;
}

}  // namespace mpc

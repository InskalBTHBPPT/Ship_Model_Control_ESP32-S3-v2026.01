#include "qp_solver.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace mpc {

void mat_mul(const std::vector<double> &A, int rows, int cols,
             const std::vector<double> &B, int bcols,
             std::vector<double> &C) {
  C.assign(static_cast<size_t>(rows) * bcols, 0.0);
  for (int i = 0; i < rows; ++i) {
    for (int k = 0; k < cols; ++k) {
      const double a = A[static_cast<size_t>(i) * cols + k];
      for (int j = 0; j < bcols; ++j) {
        C[static_cast<size_t>(i) * bcols + j] +=
            a * B[static_cast<size_t>(k) * bcols + j];
      }
    }
  }
}

void mat_transpose_mul(const std::vector<double> &A, int rows, int cols,
                       const std::vector<double> &B, int brows,
                       std::vector<double> &C) {
  C.assign(static_cast<size_t>(cols) * brows, 0.0);
  for (int i = 0; i < rows; ++i) {
    for (int k = 0; k < cols; ++k) {
      const double a = A[static_cast<size_t>(i) * cols + k];
      for (int j = 0; j < brows; ++j) {
        C[static_cast<size_t>(k) * brows + j] +=
            a * B[static_cast<size_t>(i) * brows + j];
      }
    }
  }
}

bool chol_decompose(std::vector<double> &A, int n) {
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j <= i; ++j) {
      double sum = A[static_cast<size_t>(i) * n + j];
      for (int k = 0; k < j; ++k) {
        sum -= A[static_cast<size_t>(i) * n + k] *
               A[static_cast<size_t>(j) * n + k];
      }
      if (i == j) {
        if (sum <= 1e-14) {
          return false;
        }
        A[static_cast<size_t>(i) * n + j] = std::sqrt(sum);
      } else {
        A[static_cast<size_t>(i) * n + j] =
            sum / A[static_cast<size_t>(j) * n + j];
      }
    }
    for (int j = i + 1; j < n; ++j) {
      A[static_cast<size_t>(i) * n + j] = 0.0;
    }
  }
  return true;
}

void chol_solve_lower(const std::vector<double> &L, int n,
                      const std::vector<double> &b, std::vector<double> &x) {
  x.assign(n, 0.0);
  for (int i = 0; i < n; ++i) {
    double sum = b[i];
    for (int k = 0; k < i; ++k) {
      sum -= L[static_cast<size_t>(i) * n + k] * x[k];
    }
    x[i] = sum / L[static_cast<size_t>(i) * n + i];
  }
  for (int i = n - 1; i >= 0; --i) {
    double sum = x[i];
    for (int k = i + 1; k < n; ++k) {
      sum -= L[static_cast<size_t>(k) * n + i] * x[k];
    }
    x[i] = sum / L[static_cast<size_t>(i) * n + i];
  }
}

static double dot_row(const std::vector<double> &G, int row, int n,
                      const std::vector<double> &x) {
  double val = 0.0;
  for (int j = 0; j < n; ++j) {
    val += G[static_cast<size_t>(row) * n + j] * x[j];
  }
  return val;
}

static void project_inequalities(const std::vector<double> &G, int m, int n,
                                 const std::vector<double> &h,
                                 std::vector<double> &x, int passes) {
  for (int pass = 0; pass < passes; ++pass) {
    for (int i = 0; i < m; ++i) {
      const double val = dot_row(G, i, n, x);
      const double viol = val - h[i];
      if (viol <= 1e-12) {
        continue;
      }
      double norm2 = 0.0;
      for (int j = 0; j < n; ++j) {
        const double c = G[static_cast<size_t>(i) * n + j];
        norm2 += c * c;
      }
      if (norm2 < 1e-14) {
        continue;
      }
      const double scale = viol / norm2;
      for (int j = 0; j < n; ++j) {
        x[j] -= scale * G[static_cast<size_t>(i) * n + j];
      }
    }
  }
}

static double objective(const std::vector<double> &H, int n,
                        const std::vector<double> &f,
                        const std::vector<double> &x) {
  double quad = 0.0;
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      quad += x[i] * H[static_cast<size_t>(i) * n + j] * x[j];
    }
  }
  double lin = 0.0;
  for (int i = 0; i < n; ++i) {
    lin += f[i] * x[i];
  }
  return 0.5 * quad + lin;
}

bool solve_qp_active_set(const std::vector<double> &H, int n,
                         const std::vector<double> &f,
                         const std::vector<double> &G, int m,
                         const std::vector<double> &h, std::vector<double> &x,
                         int max_iter) {
  x.assign(n, 0.0);
  std::vector<double> L = H;
  std::vector<double> rhs(n, 0.0);
  for (int i = 0; i < n; ++i) {
    rhs[i] = -f[i];
  }
  if (chol_decompose(L, n)) {
    chol_solve_lower(L, n, rhs, x);
  }
  project_inequalities(G, m, n, h, x, 15);

  std::vector<double> g(n, 0.0);
  std::vector<double> x_try(n, 0.0);

  for (int iter = 0; iter < max_iter; ++iter) {
    project_inequalities(G, m, n, h, x, 2);

    double max_viol = 0.0;
    for (int i = 0; i < m; ++i) {
      max_viol = std::max(max_viol, dot_row(G, i, n, x) - h[i]);
    }

    for (int i = 0; i < n; ++i) {
      g[i] = f[i];
      for (int j = 0; j < n; ++j) {
        g[i] += H[static_cast<size_t>(i) * n + j] * x[j];
      }
    }
    double gnorm = 0.0;
    for (int i = 0; i < n; ++i) {
      gnorm += g[i] * g[i];
    }
    gnorm = std::sqrt(gnorm);
    if (max_viol < 1e-7 && gnorm < 1e-5) {
      return true;
    }

    double alpha = 1.0;
    const double f0 = objective(H, n, f, x);
    for (int ls = 0; ls < 20; ++ls) {
      for (int i = 0; i < n; ++i) {
        x_try[i] = x[i] - alpha * g[i];
      }
      project_inequalities(G, m, n, h, x_try, 8);
      const double f1 = objective(H, n, f, x_try);
      if (f1 < f0 - 1e-9 * alpha * gnorm * gnorm) {
        x = x_try;
        break;
      }
      alpha *= 0.5;
    }
    if (alpha < 1e-6) {
      for (int i = 0; i < n; ++i) {
        x[i] -= 1e-4 * g[i];
      }
    }
  }

  project_inequalities(G, m, n, h, x, 20);
  return true;
}

}  // namespace mpc

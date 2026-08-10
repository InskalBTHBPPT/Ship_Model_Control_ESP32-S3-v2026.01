#pragma once

#include <vector>

namespace mpc {

// min 0.5 x'Hx + f'x  s.t  G x <= h  (row-major G: m x n)
bool solve_qp_active_set(const std::vector<double> &H, int n,
                         const std::vector<double> &f,
                         const std::vector<double> &G, int m,
                         const std::vector<double> &h, std::vector<double> &x,
                         int max_iter = 400);

void mat_mul(const std::vector<double> &A, int rows, int cols,
             const std::vector<double> &B, int bcols,
             std::vector<double> &C);

void mat_transpose_mul(const std::vector<double> &A, int rows, int cols,
                       const std::vector<double> &B, int brows,
                       std::vector<double> &C);

void chol_solve_lower(const std::vector<double> &L, int n,
                      const std::vector<double> &b, std::vector<double> &x);

bool chol_decompose(std::vector<double> &A, int n);

}  // namespace mpc

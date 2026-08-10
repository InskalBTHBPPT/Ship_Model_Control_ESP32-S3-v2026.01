#pragma once

#include <string>
#include <vector>

namespace mpc {

// min 0.5 x'Hx + f'x  s.t.  G x <= h  (row-major G: m x n)
bool solve_qp_osqp_reduced(const std::vector<double> &H, int n,
                           const std::vector<double> &f,
                           const std::vector<double> &G, int m,
                           const std::vector<double> &h, std::vector<double> &x,
                           std::string &status);

}  // namespace mpc

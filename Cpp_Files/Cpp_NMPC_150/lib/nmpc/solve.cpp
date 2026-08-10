//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// solve.cpp
//
// Code generation for function 'solve'
//

// Include files
#include "solve.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace CholManager {
void solve(const i_struct_T &obj, double rhs[151])
{
  int jA;
  int n;
  n = obj.ndims;
  if (obj.ndims != 0) {
    for (int j{0}; j < n; j++) {
      double temp;
      jA = j * 301;
      temp = rhs[j];
      for (int i{0}; i < j; i++) {
        temp -= obj.FMat[jA + i] * rhs[i];
      }
      rhs[j] = temp / obj.FMat[jA + j];
    }
  }
  if (obj.ndims != 0) {
    for (int j{n}; j >= 1; j--) {
      jA = (j + (j - 1) * 301) - 1;
      rhs[j - 1] /= obj.FMat[jA];
      for (int i{0}; i <= j - 2; i++) {
        int ix;
        ix = (j - i) - 2;
        rhs[ix] -= rhs[j - 1] * obj.FMat[(jA - i) - 1];
      }
    }
  }
}

} // namespace CholManager
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (solve.cpp)

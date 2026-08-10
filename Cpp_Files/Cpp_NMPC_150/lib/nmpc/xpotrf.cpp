//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xpotrf.cpp
//
// Code generation for function 'xpotrf'
//

// Include files
#include "xpotrf.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace internal {
namespace lapack {
int xpotrf(int n, double A[90601])
{
  int info;
  int j;
  boolean_T exitg1;
  info = 0;
  j = 0;
  exitg1 = false;
  while ((!exitg1) && (j <= n - 1)) {
    double c;
    double ssq;
    int idxA1j;
    int idxAjj;
    idxA1j = j * 301;
    idxAjj = idxA1j + j;
    ssq = 0.0;
    if (j >= 1) {
      for (int k{0}; k < j; k++) {
        c = A[idxA1j + k];
        ssq += c * c;
      }
    }
    ssq = A[idxAjj] - ssq;
    if (ssq > 0.0) {
      ssq = std::sqrt(ssq);
      A[idxAjj] = ssq;
      if (j + 1 < n) {
        int i1;
        int ia0;
        int idxAjjp1;
        int nmj;
        nmj = (n - j) - 2;
        ia0 = idxA1j + 302;
        idxAjjp1 = idxAjj + 302;
        if ((j != 0) && (nmj + 1 != 0)) {
          int i;
          i = (idxA1j + 301 * nmj) + 302;
          for (int iac{ia0}; iac <= i; iac += 301) {
            c = 0.0;
            i1 = iac + j;
            for (int k{iac}; k < i1; k++) {
              c += A[k - 1] * A[(idxA1j + k) - iac];
            }
            i1 = (idxAjj + div_nde_s32_floor((iac - idxA1j) - 302, 301) * 301) +
                 301;
            A[i1] -= c;
          }
        }
        ssq = 1.0 / ssq;
        i1 = (idxAjj + 301 * nmj) + 302;
        for (int k{idxAjjp1}; k <= i1; k += 301) {
          A[k - 1] *= ssq;
        }
      }
      j++;
    } else {
      A[idxAjj] = ssq;
      info = j + 1;
      exitg1 = true;
    }
  }
  return info;
}

} // namespace lapack
} // namespace internal
} // namespace coder

// End of code generation (xpotrf.cpp)

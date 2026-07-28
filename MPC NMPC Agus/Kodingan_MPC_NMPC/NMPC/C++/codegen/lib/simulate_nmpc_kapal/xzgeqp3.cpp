//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xzgeqp3.cpp
//
// Code generation for function 'xzgeqp3'
//

// Include files
#include "xzgeqp3.h"
#include "rt_nonfinite.h"
#include "xzlarf.h"
#include "xzlarfg.h"
#include <cstring>

// Function Definitions
namespace coder {
namespace internal {
namespace reflapack {
void qrf(double A[90601], int m, int n, int nfxd, double tau[301])
{
  double work[301];
  int i;
  std::memset(&tau[0], 0, 301U * sizeof(double));
  std::memset(&work[0], 0, 301U * sizeof(double));
  i = static_cast<unsigned short>(nfxd);
  for (int b_i{0}; b_i < i; b_i++) {
    double atmp;
    double d;
    int ii;
    int mmi;
    ii = b_i * 301 + b_i;
    mmi = m - b_i;
    if (b_i + 1 < m) {
      atmp = A[ii];
      d = xzlarfg(mmi, atmp, A, ii + 2);
      tau[b_i] = d;
      A[ii] = atmp;
    } else {
      d = 0.0;
      tau[b_i] = 0.0;
    }
    if (b_i + 1 < n) {
      atmp = A[ii];
      A[ii] = 1.0;
      xzlarf(mmi, (n - b_i) - 1, ii + 1, d, A, ii + 302, work);
      A[ii] = atmp;
    }
  }
}

} // namespace reflapack
} // namespace internal
} // namespace coder

// End of code generation (xzgeqp3.cpp)

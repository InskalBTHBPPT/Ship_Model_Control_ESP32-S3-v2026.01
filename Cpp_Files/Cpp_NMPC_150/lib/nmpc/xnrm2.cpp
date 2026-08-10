//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xnrm2.cpp
//
// Code generation for function 'xnrm2'
//

// Include files
#include "xnrm2.h"
#include "rt_nonfinite.h"
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace internal {
namespace blas {
double xnrm2(int n, const double x[151])
{
  double y;
  y = 0.0;
  if (n >= 1) {
    if (n == 1) {
      y = std::abs(x[0]);
    } else {
      double scale;
      scale = 3.3121686421112381E-170;
      for (int k{0}; k < n; k++) {
        double absxk;
        absxk = std::abs(x[k]);
        if (absxk > scale) {
          double t;
          t = scale / absxk;
          y = y * t * t + 1.0;
          scale = absxk;
        } else {
          double t;
          t = absxk / scale;
          y += t * t;
        }
      }
      y = scale * std::sqrt(y);
      if (std::isnan(y)) {
        int b_k;
        b_k = 0;
        int exitg1;
        do {
          exitg1 = 0;
          if (b_k <= n - 1) {
            if (std::isnan(x[b_k])) {
              exitg1 = 1;
            } else {
              b_k++;
            }
          } else {
            y = rtInf;
            exitg1 = 1;
          }
        } while (exitg1 == 0);
      }
    }
  }
  return y;
}

double xnrm2(int n, const double x[90601], int ix0)
{
  double y;
  y = 0.0;
  if (n >= 1) {
    if (n == 1) {
      y = std::abs(x[ix0 - 1]);
    } else {
      double scale;
      int kend;
      scale = 3.3121686421112381E-170;
      kend = (ix0 + n) - 1;
      for (int k{ix0}; k <= kend; k++) {
        double absxk;
        absxk = std::abs(x[k - 1]);
        if (absxk > scale) {
          double t;
          t = scale / absxk;
          y = y * t * t + 1.0;
          scale = absxk;
        } else {
          double t;
          t = absxk / scale;
          y += t * t;
        }
      }
      y = scale * std::sqrt(y);
      if (std::isnan(y)) {
        int b_k;
        b_k = ix0;
        int exitg1;
        do {
          exitg1 = 0;
          if (b_k <= kend) {
            if (std::isnan(x[b_k - 1])) {
              exitg1 = 1;
            } else {
              b_k++;
            }
          } else {
            y = rtInf;
            exitg1 = 1;
          }
        } while (exitg1 == 0);
      }
    }
  }
  return y;
}

} // namespace blas
} // namespace internal
} // namespace coder

// End of code generation (xnrm2.cpp)

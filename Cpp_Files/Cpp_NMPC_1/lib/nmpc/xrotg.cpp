//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xrotg.cpp
//
// Code generation for function 'xrotg'
//

// Include files
#include "xrotg.h"
#include "rt_nonfinite.h"
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace internal {
namespace blas {
double xrotg(double &a, double &b, double &s)
{
  double absa;
  double absb;
  double c;
  double roe;
  double scale;
  roe = b;
  absa = std::abs(a);
  absb = std::abs(b);
  if (absa > absb) {
    roe = a;
  }
  scale = absa + absb;
  if (scale == 0.0) {
    scale = 0.0;
    c = 1.0;
    a = 0.0;
    b = 0.0;
  } else {
    double bds;
    c = absa / scale;
    bds = absb / scale;
    bds = scale * std::sqrt(c * c + bds * bds);
    if (roe < 0.0) {
      bds = -bds;
    }
    c = a / bds;
    scale = b / bds;
    if (absa > absb) {
      b = scale;
    } else if (c != 0.0) {
      b = 1.0 / c;
    } else {
      b = 1.0;
    }
    a = bds;
  }
  s = scale;
  return c;
}

} // namespace blas
} // namespace internal
} // namespace coder

// End of code generation (xrotg.cpp)

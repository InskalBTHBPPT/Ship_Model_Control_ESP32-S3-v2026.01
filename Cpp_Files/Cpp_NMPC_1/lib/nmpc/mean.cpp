//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// mean.cpp
//
// Code generation for function 'mean'
//

// Include files
#include "mean.h"
#include "rt_nonfinite.h"
#include <cstring>

// Function Definitions
namespace coder {
double mean(const double x[151])
{
  double y;
  y = x[0];
  for (int k{0}; k < 150; k++) {
    y += x[k + 1];
  }
  y /= 151.0;
  return y;
}

} // namespace coder

// End of code generation (mean.cpp)

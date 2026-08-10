//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeFval_ReuseHx.cpp
//
// Code generation for function 'computeFval_ReuseHx'
//

// Include files
#include "computeFval_ReuseHx.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <algorithm>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace Objective {
double computeFval_ReuseHx(const struct_T &obj, double workspace[45451],
                           const double f[151], const double x[151])
{
  double val;
  val = 0.0;
  switch (obj.objtype) {
  case 5:
    val = obj.gammaScalar * x[obj.nvar - 1];
    break;
  case 3: {
    if (obj.hasLinear) {
      int i;
      int ixlast;
      int vectorUB;
      i = obj.nvar;
      ixlast = (i / 2) << 1;
      vectorUB = ixlast - 2;
      for (int b_i{0}; b_i <= vectorUB; b_i += 2) {
        __m128d r;
        r = _mm_loadu_pd(&obj.Hx[b_i]);
        _mm_storeu_pd(
            &workspace[b_i],
            _mm_add_pd(_mm_mul_pd(_mm_set1_pd(0.5), r), _mm_loadu_pd(&f[b_i])));
      }
      for (int b_i{ixlast}; b_i < i; b_i++) {
        workspace[b_i] = 0.5 * obj.Hx[b_i] + f[b_i];
      }
      if (obj.nvar >= 1) {
        for (int b_i{0}; b_i < i; b_i++) {
          val += x[b_i] * workspace[b_i];
        }
      }
    } else {
      if (obj.nvar >= 1) {
        int ixlast;
        ixlast = obj.nvar;
        for (int b_i{0}; b_i < ixlast; b_i++) {
          val += x[b_i] * obj.Hx[b_i];
        }
      }
      val *= 0.5;
    }
  } break;
  case 4: {
    if (obj.hasLinear) {
      int ixlast;
      ixlast = obj.nvar;
      if (ixlast - 1 >= 0) {
        std::copy(&f[0], &f[ixlast], &workspace[0]);
      }
      ixlast = 150 - obj.nvar;
      for (int b_i{0}; b_i < ixlast; b_i++) {
        workspace[obj.nvar + b_i] = obj.rho;
      }
      for (int b_i{0}; b_i < 150; b_i++) {
        double d;
        d = workspace[b_i] + 0.5 * obj.Hx[b_i];
        workspace[b_i] = d;
        val += x[b_i] * d;
      }
    } else {
      int ixlast;
      for (int b_i{0}; b_i < 150; b_i++) {
        val += x[b_i] * obj.Hx[b_i];
      }
      val *= 0.5;
      ixlast = obj.nvar + 1;
      for (int b_i{ixlast}; b_i < 151; b_i++) {
        val += x[b_i - 1] * obj.rho;
      }
    }
  } break;
  }
  return val;
}

} // namespace Objective
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (computeFval_ReuseHx.cpp)

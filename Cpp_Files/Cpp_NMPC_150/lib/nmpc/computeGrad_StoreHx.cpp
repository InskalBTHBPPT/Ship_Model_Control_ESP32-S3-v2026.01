//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeGrad_StoreHx.cpp
//
// Code generation for function 'computeGrad_StoreHx'
//

// Include files
#include "computeGrad_StoreHx.h"
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
void computeGrad_StoreHx(struct_T &obj, const double H[900],
                         const double f[151], const double x[151])
{
  switch (obj.objtype) {
  case 5: {
    int ixlast;
    ixlast = obj.nvar;
    if (ixlast - 2 >= 0) {
      std::memset(&obj.grad[0], 0,
                  static_cast<unsigned int>(ixlast - 1) * sizeof(double));
    }
    obj.grad[obj.nvar - 1] = obj.gammaScalar;
  } break;
  case 3: {
    int ixlast;
    int iy;
    int m;
    ixlast = obj.nvar - 1;
    iy = obj.nvar;
    if (obj.nvar != 0) {
      int ix;
      int scalarLB;
      if (ixlast >= 0) {
        std::memset(&obj.Hx[0], 0,
                    static_cast<unsigned int>(ixlast + 1) * sizeof(double));
      }
      ix = 0;
      scalarLB = obj.nvar * (obj.nvar - 1) + 1;
      for (int idx{1}; iy < 0 ? idx >= scalarLB : idx <= scalarLB; idx += iy) {
        m = idx + ixlast;
        for (int ia{idx}; ia <= m; ia++) {
          int i;
          i = ia - idx;
          obj.Hx[i] += H[ia - 1] * x[ix];
        }
        ix++;
      }
    }
    ixlast = obj.nvar;
    if (ixlast - 1 >= 0) {
      std::copy(&obj.Hx[0], &obj.Hx[ixlast], &obj.grad[0]);
    }
    if (obj.hasLinear && (obj.nvar >= 1)) {
      m = (iy / 2) << 1;
      ixlast = m - 2;
      for (int idx{0}; idx <= ixlast; idx += 2) {
        __m128d r;
        r = _mm_loadu_pd(&obj.grad[idx]);
        _mm_storeu_pd(&obj.grad[idx], _mm_add_pd(r, _mm_loadu_pd(&f[idx])));
      }
      for (int idx{m}; idx < iy; idx++) {
        obj.grad[idx] += f[idx];
      }
    }
  } break;
  case 4: {
    __m128d r;
    int ix;
    int ixlast;
    int iy;
    int m;
    int scalarLB;
    m = obj.nvar - 1;
    ixlast = obj.nvar;
    if (obj.nvar != 0) {
      if (m >= 0) {
        std::memset(&obj.Hx[0], 0,
                    static_cast<unsigned int>(m + 1) * sizeof(double));
      }
      iy = 0;
      ix = obj.nvar * (obj.nvar - 1) + 1;
      for (int idx{1}; ixlast < 0 ? idx >= ix : idx <= ix; idx += ixlast) {
        scalarLB = idx + m;
        for (int ia{idx}; ia <= scalarLB; ia++) {
          int i;
          i = ia - idx;
          obj.Hx[i] += H[ia - 1] * x[iy];
        }
        iy++;
      }
    }
    ixlast = obj.nvar + 1;
    iy = (((151 - ixlast) / 2) << 1) + ixlast;
    m = iy - 2;
    for (int idx{ixlast}; idx <= m; idx += 2) {
      _mm_storeu_pd(&obj.Hx[idx - 1], _mm_mul_pd(_mm_set1_pd(obj.beta),
                                                 _mm_loadu_pd(&x[idx - 1])));
    }
    for (int idx{iy}; idx < 151; idx++) {
      obj.Hx[idx - 1] = obj.beta * x[idx - 1];
    }
    std::copy(&obj.Hx[0], &obj.Hx[150], &obj.grad[0]);
    if (obj.hasLinear && (obj.nvar >= 1)) {
      ixlast = obj.nvar;
      iy = (ixlast / 2) << 1;
      m = iy - 2;
      for (int idx{0}; idx <= m; idx += 2) {
        r = _mm_loadu_pd(&obj.grad[idx]);
        _mm_storeu_pd(&obj.grad[idx], _mm_add_pd(r, _mm_loadu_pd(&f[idx])));
      }
      for (int idx{iy}; idx < ixlast; idx++) {
        obj.grad[idx] += f[idx];
      }
    }
    if (150 - obj.nvar >= 1) {
      iy = obj.nvar;
      ix = 149 - obj.nvar;
      scalarLB = ((ix + 1) / 2) << 1;
      m = scalarLB - 2;
      for (int idx{0}; idx <= m; idx += 2) {
        ixlast = iy + idx;
        r = _mm_loadu_pd(&obj.grad[ixlast]);
        _mm_storeu_pd(&obj.grad[ixlast], _mm_add_pd(r, _mm_set1_pd(obj.rho)));
      }
      for (int idx{scalarLB}; idx <= ix; idx++) {
        m = iy + idx;
        obj.grad[m] += obj.rho;
      }
    }
  } break;
  }
}

} // namespace Objective
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (computeGrad_StoreHx.cpp)

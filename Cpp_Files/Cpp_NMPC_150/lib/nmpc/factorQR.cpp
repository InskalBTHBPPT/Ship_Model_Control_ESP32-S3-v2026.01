//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// factorQR.cpp
//
// Code generation for function 'factorQR'
//

// Include files
#include "factorQR.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_data.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "xzgeqp3.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace QRManager {
void factorQR(c_struct_T &obj, const double A[45451], int mrows, int ncols)
{
  int ix0;
  int iy0;
  boolean_T guard1;
  ix0 = mrows * ncols;
  guard1 = false;
  if (ix0 > 0) {
    for (int idx{0}; idx < ncols; idx++) {
      int i;
      ix0 = 151 * idx;
      iy0 = 301 * idx;
      i = static_cast<unsigned char>(mrows);
      for (int k{0}; k < i; k++) {
        obj.QR[iy0 + k] = A[ix0 + k];
      }
    }
    guard1 = true;
  } else if (ix0 == 0) {
    obj.mrows = mrows;
    obj.ncols = ncols;
    obj.minRowCol = 0;
  } else {
    guard1 = true;
  }
  if (guard1) {
    obj.usedPivoting = false;
    obj.mrows = mrows;
    obj.ncols = ncols;
    ix0 = (ncols / 4) << 2;
    iy0 = ix0 - 4;
    for (int idx{0}; idx <= iy0; idx += 4) {
      _mm_storeu_si128(
          (__m128i *)&obj.jpvt[idx],
          _mm_add_epi32(_mm_add_epi32(_mm_set1_epi32(idx),
                                      _mm_loadu_si128((const __m128i *)&iv[0])),
                        _mm_set1_epi32(1)));
    }
    for (int idx{ix0}; idx < ncols; idx++) {
      obj.jpvt[idx] = idx + 1;
    }
    if (mrows <= ncols) {
      ix0 = mrows;
    } else {
      ix0 = ncols;
    }
    obj.minRowCol = ix0;
    std::memset(&obj.tau[0], 0, 301U * sizeof(double));
    if (ix0 >= 1) {
      internal::reflapack::qrf(obj.QR, mrows, ncols, ix0, obj.tau);
    }
  }
}

} // namespace QRManager
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (factorQR.cpp)

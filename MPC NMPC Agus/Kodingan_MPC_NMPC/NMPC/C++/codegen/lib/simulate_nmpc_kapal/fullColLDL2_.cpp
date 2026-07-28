//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// fullColLDL2_.cpp
//
// Code generation for function 'fullColLDL2_'
//

// Include files
#include "fullColLDL2_.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace DynamicRegCholManager {
void fullColLDL2_(i_struct_T &obj, int LD_offset, int NColsRemain)
{
  for (int k{0}; k < NColsRemain; k++) {
    __m128d r;
    double alpha1;
    double y;
    int LD_diagOffset;
    int jA;
    int offset1;
    int scalarLB;
    int subMatrixDim;
    int vectorUB;
    LD_diagOffset = LD_offset + 302 * k;
    alpha1 = -1.0 / obj.FMat[LD_diagOffset - 1];
    subMatrixDim = (NColsRemain - k) - 2;
    offset1 = LD_diagOffset + 1;
    y = obj.workspace_;
    for (int b_k{0}; b_k <= subMatrixDim; b_k++) {
      y = obj.FMat[LD_diagOffset + b_k];
    }
    obj.workspace_ = y;
    if (!(alpha1 == 0.0)) {
      jA = LD_diagOffset;
      for (int b_k{0}; b_k <= subMatrixDim; b_k++) {
        if (y != 0.0) {
          double temp;
          int b_scalarLB;
          int b_vectorUB;
          temp = y * alpha1;
          scalarLB = jA + 302;
          vectorUB = subMatrixDim + jA;
          b_scalarLB = ((vectorUB - scalarLB) + 303) / 2 * 2 + scalarLB;
          b_vectorUB = b_scalarLB - 2;
          for (int ijA{scalarLB}; ijA <= b_vectorUB; ijA += 2) {
            r = _mm_loadu_pd(&obj.FMat[ijA - 1]);
            r = _mm_add_pd(r, _mm_set1_pd(y * temp));
            _mm_storeu_pd(&obj.FMat[ijA - 1], r);
          }
          for (int ijA{b_scalarLB}; ijA <= vectorUB + 302; ijA++) {
            obj.FMat[ijA - 1] += y * temp;
          }
        }
        jA += 301;
      }
    }
    alpha1 = 1.0 / obj.FMat[LD_diagOffset - 1];
    jA = (LD_diagOffset + subMatrixDim) + 1;
    scalarLB = ((jA - offset1) + 1) / 2 * 2 + offset1;
    vectorUB = scalarLB - 2;
    for (int b_k{offset1}; b_k <= vectorUB; b_k += 2) {
      r = _mm_loadu_pd(&obj.FMat[b_k - 1]);
      r = _mm_mul_pd(_mm_set1_pd(alpha1), r);
      _mm_storeu_pd(&obj.FMat[b_k - 1], r);
    }
    for (int b_k{scalarLB}; b_k <= jA; b_k++) {
      obj.FMat[b_k - 1] *= alpha1;
    }
  }
}

} // namespace DynamicRegCholManager
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (fullColLDL2_.cpp)

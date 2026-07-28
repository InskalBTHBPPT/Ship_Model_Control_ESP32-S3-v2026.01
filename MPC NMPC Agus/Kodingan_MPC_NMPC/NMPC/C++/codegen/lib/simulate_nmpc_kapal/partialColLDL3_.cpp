//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// partialColLDL3_.cpp
//
// Code generation for function 'partialColLDL3_'
//

// Include files
#include "partialColLDL3_.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace DynamicRegCholManager {
void partialColLDL3_(i_struct_T &obj, int LD_offset, int NColsRemain)
{
  __m128d r;
  double d;
  int FMat_offset;
  int LD_diagOffset;
  int i;
  int ix;
  int lastColC;
  int subRows;
  for (int k{0}; k < 48; k++) {
    double y;
    subRows = (NColsRemain - k) - 1;
    LD_diagOffset = (LD_offset + 302 * k) - 1;
    for (int idx{0}; idx <= subRows; idx++) {
      obj.workspace_ = obj.FMat[LD_diagOffset + idx];
    }
    for (int idx{0}; idx < NColsRemain; idx++) {
      obj.workspace2_ = obj.workspace_;
    }
    y = obj.workspace2_;
    if ((NColsRemain != 0) && (k != 0)) {
      ix = LD_offset + k;
      FMat_offset = 301 * (k - 1) + 1;
      for (int idx{1}; idx <= FMat_offset; idx += 301) {
        lastColC = idx + NColsRemain;
        for (int ia{idx}; ia < lastColC; ia++) {
          y += obj.workspace_ * -obj.FMat[ix - 1];
        }
        ix += 301;
      }
    }
    obj.workspace2_ = y;
    for (int idx{0}; idx < NColsRemain; idx++) {
      obj.workspace_ = y;
    }
    for (int idx{0}; idx <= subRows; idx++) {
      obj.FMat[LD_diagOffset + idx] = obj.workspace_;
    }
    lastColC = subRows / 2 * 2;
    ix = lastColC - 2;
    for (int idx{0}; idx <= ix; idx += 2) {
      FMat_offset = (LD_diagOffset + idx) + 1;
      r = _mm_loadu_pd(&obj.FMat[FMat_offset]);
      r = _mm_div_pd(r, _mm_set1_pd(obj.FMat[LD_diagOffset]));
      _mm_storeu_pd(&obj.FMat[FMat_offset], r);
    }
    for (int idx{lastColC}; idx < subRows; idx++) {
      ix = (LD_diagOffset + idx) + 1;
      obj.FMat[ix] /= obj.FMat[LD_diagOffset];
    }
  }
  i = NColsRemain - 1;
  for (int j{48}; j <= i; j += 48) {
    int m;
    int subBlockSize;
    subRows = NColsRemain - j;
    if (subRows >= 48) {
      subBlockSize = 48;
    } else {
      subBlockSize = subRows;
    }
    LD_diagOffset = j + subBlockSize;
    for (int k{j}; k < LD_diagOffset; k++) {
      m = LD_diagOffset - k;
      for (int idx{0}; idx < 48; idx++) {
        d = obj.FMat[((LD_offset + k) + idx * 301) - 1];
      }
      obj.workspace2_ = d;
      lastColC = k + 1;
      if (m != 0) {
        ix = k + 14148;
        for (int idx{lastColC}; idx <= ix; idx += 301) {
          FMat_offset = idx + m;
          for (int ia{idx}; ia < FMat_offset; ia++) {
            // Check node always fails. would cause program termination and was
            // eliminated
          }
        }
      }
    }
    if (LD_diagOffset < NColsRemain) {
      int b_m;
      int ic0;
      b_m = subRows - subBlockSize;
      ic0 = ((LD_offset + subBlockSize) + 302 * j) - 1;
      for (int idx{0}; idx < 48; idx++) {
        FMat_offset = (LD_offset + j) + idx * 301;
        for (int ia{0}; ia < subBlockSize; ia++) {
          obj.workspace2_ = obj.FMat[(FMat_offset + ia) - 1];
        }
      }
      if ((b_m != 0) && (subBlockSize != 0)) {
        lastColC = ic0 + 301 * (subBlockSize - 1);
        ix = 0;
        for (int idx{ic0}; idx <= lastColC; idx += 301) {
          ix++;
          FMat_offset = ix + 14147;
          for (int ia{ix}; ia <= FMat_offset; ia += 301) {
            subRows = idx + 1;
            LD_diagOffset = idx + b_m;
            m = ((LD_diagOffset - subRows) + 1) / 2 * 2 + subRows;
            subBlockSize = m - 2;
            for (int k{subRows}; k <= subBlockSize; k += 2) {
              r = _mm_loadu_pd(&obj.FMat[k - 1]);
              r = _mm_add_pd(r, _mm_set1_pd(-obj.workspace2_ * obj.workspace_));
              _mm_storeu_pd(&obj.FMat[k - 1], r);
            }
            for (int k{m}; k <= LD_diagOffset; k++) {
              obj.FMat[k - 1] += -obj.workspace2_ * obj.workspace_;
            }
          }
        }
      }
    }
  }
}

} // namespace DynamicRegCholManager
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (partialColLDL3_.cpp)

//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// compute_lambda.cpp
//
// Code generation for function 'compute_lambda'
//

// Include files
#include "compute_lambda.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
void compute_lambda(double workspace[45451], e_struct_T &solution,
                    const struct_T &objective, const c_struct_T &qrmanager)
{
  int nActiveConstr;
  nActiveConstr = qrmanager.ncols;
  if (qrmanager.ncols > 0) {
    double c;
    int ix;
    int jjA;
    boolean_T guard1;
    guard1 = false;
    if (objective.objtype != 4) {
      boolean_T nonDegenerate;
      jjA = qrmanager.mrows;
      ix = qrmanager.ncols;
      if (jjA >= ix) {
        ix = jjA;
      }
      c = std::fmin(1.4901161193847656E-8,
                    2.2204460492503131E-15 * static_cast<double>(ix));
      if ((qrmanager.mrows > 0) && (qrmanager.ncols > 0)) {
        nonDegenerate = true;
      } else {
        nonDegenerate = false;
      }
      if (nonDegenerate) {
        boolean_T guard2;
        jjA = nActiveConstr;
        guard2 = false;
        if (qrmanager.mrows < qrmanager.ncols) {
          ix = qrmanager.mrows + 301 * (qrmanager.ncols - 1);
          while ((jjA > qrmanager.mrows) &&
                 (std::abs(qrmanager.QR[ix - 1]) >= c)) {
            jjA--;
            ix -= 301;
          }
          nonDegenerate = (jjA == qrmanager.mrows);
          if (nonDegenerate) {
            guard2 = true;
          }
        } else {
          guard2 = true;
        }
        if (guard2) {
          ix = jjA + 301 * (jjA - 1);
          while ((jjA >= 1) && (std::abs(qrmanager.QR[ix - 1]) >= c)) {
            jjA--;
            ix -= 302;
          }
          nonDegenerate = (jjA == 0);
        }
      }
      if (!nonDegenerate) {
        solution.state = -7;
      } else {
        guard1 = true;
      }
    } else {
      guard1 = true;
    }
    if (guard1) {
      if (qrmanager.mrows != 0) {
        if (nActiveConstr - 1 >= 0) {
          std::memset(&workspace[0], 0,
                      static_cast<unsigned int>(nActiveConstr) *
                          sizeof(double));
        }
        ix = 301 * (qrmanager.ncols - 1) + 1;
        for (int iac{1}; iac <= ix; iac += 301) {
          c = 0.0;
          jjA = iac + qrmanager.mrows;
          for (int ia{iac}; ia < jjA; ia++) {
            c += qrmanager.Q[ia - 1] * objective.grad[ia - iac];
          }
          jjA = div_nde_s32_floor(iac - 1, 301);
          workspace[jjA] += c;
        }
      }
      for (int ia{nActiveConstr}; ia >= 1; ia--) {
        jjA = (ia + (ia - 1) * 301) - 1;
        workspace[ia - 1] /= qrmanager.QR[jjA];
        for (int iac{0}; iac <= ia - 2; iac++) {
          ix = (ia - iac) - 2;
          workspace[ix] -= workspace[ia - 1] * qrmanager.QR[(jjA - iac) - 1];
        }
      }
      jjA = nActiveConstr / 2 * 2;
      ix = jjA - 2;
      for (int ia{0}; ia <= ix; ia += 2) {
        __m128d r;
        r = _mm_loadu_pd(&workspace[ia]);
        r = _mm_mul_pd(r, _mm_set1_pd(-1.0));
        _mm_storeu_pd(&solution.lambda[ia], r);
      }
      for (int ia{jjA}; ia < nActiveConstr; ia++) {
        solution.lambda[ia] = -workspace[ia];
      }
    }
  }
}

} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (compute_lambda.cpp)

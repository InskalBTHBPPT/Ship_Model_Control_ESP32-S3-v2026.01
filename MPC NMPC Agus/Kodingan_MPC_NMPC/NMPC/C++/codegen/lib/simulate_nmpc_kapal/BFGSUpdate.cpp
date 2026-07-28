//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// BFGSUpdate.cpp
//
// Code generation for function 'BFGSUpdate'
//

// Include files
#include "BFGSUpdate.h"
#include "rt_nonfinite.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
boolean_T BFGSUpdate(int nvar, double Bk[900], const double sk[151],
                     double yk[151], double workspace[45451])
{
  __m128d r;
  __m128d r1;
  double curvatureS;
  double dotSY;
  double theta;
  int i;
  int ix;
  int jA;
  int scalarLB;
  int vectorUB;
  boolean_T success;
  dotSY = 0.0;
  i = static_cast<unsigned char>(nvar);
  for (int k{0}; k < i; k++) {
    dotSY += sk[k] * yk[k];
    workspace[k] = 0.0;
  }
  ix = 0;
  jA = 30 * (nvar - 1) + 1;
  for (int k{1}; k <= jA; k += 30) {
    scalarLB = k + nvar;
    for (int ia{k}; ia < scalarLB; ia++) {
      vectorUB = ia - k;
      workspace[vectorUB] += Bk[ia - 1] * sk[ix];
    }
    ix++;
  }
  curvatureS = 0.0;
  if (nvar >= 1) {
    for (int k{0}; k < nvar; k++) {
      curvatureS += sk[k] * workspace[k];
    }
  }
  if (dotSY < 0.2 * curvatureS) {
    theta = 0.8 * curvatureS / (curvatureS - dotSY);
    ix = (static_cast<unsigned char>(nvar) >> 1) << 1;
    jA = ix - 2;
    for (int k{0}; k <= jA; k += 2) {
      r = _mm_loadu_pd(&yk[k]);
      _mm_storeu_pd(&yk[k], _mm_mul_pd(_mm_set1_pd(theta), r));
    }
    for (int k{ix}; k < i; k++) {
      yk[k] *= theta;
    }
    if (!(1.0 - theta == 0.0)) {
      ix = nvar / 2 * 2;
      jA = ix - 2;
      for (int k{0}; k <= jA; k += 2) {
        r = _mm_loadu_pd(&workspace[k]);
        r = _mm_mul_pd(_mm_set1_pd(1.0 - theta), r);
        r1 = _mm_loadu_pd(&yk[k]);
        r = _mm_add_pd(r1, r);
        _mm_storeu_pd(&yk[k], r);
      }
      for (int k{ix}; k < nvar; k++) {
        yk[k] += (1.0 - theta) * workspace[k];
      }
    }
    dotSY = 0.0;
    for (int k{0}; k < i; k++) {
      dotSY += sk[k] * yk[k];
    }
  }
  if ((curvatureS > 2.2204460492503131E-16) &&
      (dotSY > 2.2204460492503131E-16)) {
    success = true;
  } else {
    success = false;
  }
  if (success) {
    curvatureS = -1.0 / curvatureS;
    if (!(curvatureS == 0.0)) {
      jA = 1;
      for (int k{0}; k < i; k++) {
        theta = workspace[k];
        if (theta != 0.0) {
          theta *= curvatureS;
          ix = nvar + jA;
          scalarLB = (ix - jA) / 2 * 2 + jA;
          vectorUB = scalarLB - 2;
          for (int ia{jA}; ia <= vectorUB; ia += 2) {
            r = _mm_loadu_pd(&workspace[ia - jA]);
            r = _mm_mul_pd(r, _mm_set1_pd(theta));
            r1 = _mm_loadu_pd(&Bk[ia - 1]);
            r = _mm_add_pd(r1, r);
            _mm_storeu_pd(&Bk[ia - 1], r);
          }
          for (int ia{scalarLB}; ia < ix; ia++) {
            Bk[ia - 1] += workspace[ia - jA] * theta;
          }
        }
        jA += 30;
      }
    }
    curvatureS = 1.0 / dotSY;
    if (!(curvatureS == 0.0)) {
      ix = 1;
      for (int k{0}; k < i; k++) {
        theta = yk[k];
        if (theta != 0.0) {
          theta *= curvatureS;
          jA = nvar + ix;
          scalarLB = (jA - ix) / 2 * 2 + ix;
          vectorUB = scalarLB - 2;
          for (int ia{ix}; ia <= vectorUB; ia += 2) {
            r = _mm_loadu_pd(&yk[ia - ix]);
            r = _mm_mul_pd(r, _mm_set1_pd(theta));
            r1 = _mm_loadu_pd(&Bk[ia - 1]);
            r = _mm_add_pd(r1, r);
            _mm_storeu_pd(&Bk[ia - 1], r);
          }
          for (int ia{scalarLB}; ia < jA; ia++) {
            Bk[ia - 1] += yk[ia - ix] * theta;
          }
        }
        ix += 30;
      }
    }
  }
  return success;
}

} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (BFGSUpdate.cpp)

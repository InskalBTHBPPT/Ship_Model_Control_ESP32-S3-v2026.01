//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xgemm.cpp
//
// Code generation for function 'xgemm'
//

// Include files
#include "xgemm.h"
#include "rt_nonfinite.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace blas {
void xgemm(int m, int n, int k, const double A[900], int lda,
           const double B[90601], int ib0, double C[45451])
{
  if ((m != 0) && (n != 0)) {
    int ar;
    int br;
    int i;
    int lastColC;
    br = ib0;
    lastColC = 301 * (n - 1);
    for (int cr{0}; cr <= lastColC; cr += 301) {
      ar = cr + 1;
      i = cr + m;
      if (ar <= i) {
        std::memset(&C[ar + -1], 0,
                    static_cast<unsigned int>((i - ar) + 1) * sizeof(double));
      }
    }
    for (int cr{0}; cr <= lastColC; cr += 301) {
      ar = -1;
      i = br + k;
      for (int ib{br}; ib < i; ib++) {
        int i1;
        int i2;
        int scalarLB;
        int vectorUB;
        i1 = cr + 1;
        i2 = cr + m;
        scalarLB = ((i2 - i1) + 1) / 2 * 2 + i1;
        vectorUB = scalarLB - 2;
        for (int ic{i1}; ic <= vectorUB; ic += 2) {
          __m128d r;
          __m128d r1;
          r = _mm_loadu_pd(&A[(ar + ic) - cr]);
          r = _mm_mul_pd(_mm_set1_pd(B[ib - 1]), r);
          r1 = _mm_loadu_pd(&C[ic - 1]);
          r = _mm_add_pd(r1, r);
          _mm_storeu_pd(&C[ic - 1], r);
        }
        for (int ic{scalarLB}; ic <= i2; ic++) {
          C[ic - 1] += B[ib - 1] * A[(ar + ic) - cr];
        }
        ar += lda;
      }
      br += 301;
    }
  }
}

void xgemm(int m, int n, int k, const double A[90601], int ia0,
           const double B[45451], double C[90601])
{
  if ((m != 0) && (n != 0)) {
    int ar;
    int br;
    int lastColC;
    lastColC = 301 * (n - 1);
    for (int cr{0}; cr <= lastColC; cr += 301) {
      br = cr + 1;
      ar = cr + m;
      if (br <= ar) {
        std::memset(&C[br + -1], 0,
                    static_cast<unsigned int>((ar - br) + 1) * sizeof(double));
      }
    }
    br = -1;
    for (int cr{0}; cr <= lastColC; cr += 301) {
      int i;
      int i1;
      ar = ia0;
      i = cr + 1;
      i1 = cr + m;
      for (int ic{i}; ic <= i1; ic++) {
        double temp;
        temp = 0.0;
        for (int w{0}; w < k; w++) {
          temp += A[(w + ar) - 1] * B[(w + br) + 1];
        }
        C[ic - 1] += temp;
        ar += 301;
      }
      br += 301;
    }
  }
}

} // namespace blas
} // namespace internal
} // namespace coder

// End of code generation (xgemm.cpp)

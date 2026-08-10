//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// xgeqp3.cpp
//
// Code generation for function 'xgeqp3'
//

// Include files
#include "xgeqp3.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_data.h"
#include "xnrm2.h"
#include "xzgeqp3.h"
#include "xzlarf.h"
#include "xzlarfg.h"
#include "omp.h"
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace internal {
namespace lapack {
void xgeqp3(double A[90601], int m, int n, int jpvt[301], double tau[301])
{
  double vn1[301];
  double vn2[301];
  double work[301];
  double d;
  double temp;
  int minmn;
  if (m <= n) {
    minmn = m;
  } else {
    minmn = n;
  }
  std::memset(&tau[0], 0, 301U * sizeof(double));
  if (minmn < 1) {
    int ix;
    int iy;
    iy = (n / 4) << 2;
    ix = iy - 4;
    for (int j{0}; j <= ix; j += 4) {
      _mm_storeu_si128(
          (__m128i *)&jpvt[j],
          _mm_add_epi32(_mm_add_epi32(_mm_set1_epi32(j),
                                      _mm_loadu_si128((const __m128i *)&iv[0])),
                        _mm_set1_epi32(1)));
    }
    for (int j{iy}; j < n; j++) {
      jpvt[j] = j + 1;
    }
  } else {
    int ix;
    int iy;
    int nfxd;
    int pvt;
    int temp_tmp;
    nfxd = 0;
    for (int j{0}; j < n; j++) {
      if (jpvt[j] != 0) {
        nfxd++;
        if (j + 1 != nfxd) {
          ix = j * 301;
          iy = (nfxd - 1) * 301;
          for (int k{0}; k < m; k++) {
            temp_tmp = ix + k;
            temp = A[temp_tmp];
            pvt = iy + k;
            A[temp_tmp] = A[pvt];
            A[pvt] = temp;
          }
          jpvt[j] = jpvt[nfxd - 1];
          jpvt[nfxd - 1] = j + 1;
        } else {
          jpvt[j] = j + 1;
        }
      } else {
        jpvt[j] = j + 1;
      }
    }
    if (nfxd > minmn) {
      nfxd = minmn;
    }
    reflapack::qrf(A, m, n, nfxd, tau);
    if (nfxd < minmn) {
      int i;
      std::memset(&work[0], 0, 301U * sizeof(double));
      std::memset(&vn1[0], 0, 301U * sizeof(double));
      std::memset(&vn2[0], 0, 301U * sizeof(double));
      iy = nfxd + 1;
      if (static_cast<int>(n - nfxd < 1600)) {
        for (int b_j{iy}; b_j <= n; b_j++) {
          d = blas::xnrm2(m - nfxd, A, (nfxd + (b_j - 1) * 301) + 1);
          vn1[b_j - 1] = d;
          vn2[b_j - 1] = d;
        }
      } else {
#pragma omp parallel for num_threads(omp_get_max_threads()) private(d)

        for (int b_j = iy; b_j <= n; b_j++) {
          d = blas::xnrm2(m - nfxd, A, (nfxd + (b_j - 1) * 301) + 1);
          vn1[b_j - 1] = d;
          vn2[b_j - 1] = d;
        }
      }
      i = nfxd + 1;
      for (int k{i}; k <= minmn; k++) {
        double temp1;
        int ii;
        int ip1;
        int mmi;
        int nmi;
        ip1 = k + 1;
        nfxd = (k - 1) * 301;
        ii = (nfxd + k) - 1;
        nmi = (n - k) + 1;
        mmi = m - k;
        if (nmi < 1) {
          ix = -2;
        } else {
          ix = -1;
          if (nmi > 1) {
            temp = std::abs(vn1[k - 1]);
            for (int j{2}; j <= nmi; j++) {
              temp1 = std::abs(vn1[(k + j) - 2]);
              if (temp1 > temp) {
                ix = j - 2;
                temp = temp1;
              }
            }
          }
        }
        pvt = k + ix;
        if (pvt + 1 != k) {
          ix = pvt * 301;
          for (int j{0}; j < m; j++) {
            iy = ix + j;
            temp = A[iy];
            temp_tmp = nfxd + j;
            A[iy] = A[temp_tmp];
            A[temp_tmp] = temp;
          }
          ix = jpvt[pvt];
          jpvt[pvt] = jpvt[k - 1];
          jpvt[k - 1] = ix;
          vn1[pvt] = vn1[k - 1];
          vn2[pvt] = vn2[k - 1];
        }
        if (k < m) {
          temp = A[ii];
          temp1 = reflapack::xzlarfg(mmi + 1, temp, A, ii + 2);
          tau[k - 1] = temp1;
          A[ii] = temp;
        } else {
          temp1 = 0.0;
          tau[k - 1] = 0.0;
        }
        if (k < n) {
          temp = A[ii];
          A[ii] = 1.0;
          reflapack::xzlarf(mmi + 1, nmi - 1, ii + 1, temp1, A, ii + 302, work);
          A[ii] = temp;
        }
        for (int j{ip1}; j <= n; j++) {
          iy = k + (j - 1) * 301;
          temp = vn1[j - 1];
          if (temp != 0.0) {
            double temp2;
            temp1 = std::abs(A[iy - 1]) / temp;
            temp1 = 1.0 - temp1 * temp1;
            if (temp1 < 0.0) {
              temp1 = 0.0;
            }
            temp2 = temp / vn2[j - 1];
            temp2 = temp1 * (temp2 * temp2);
            if (temp2 <= 1.4901161193847656E-8) {
              if (k < m) {
                temp = blas::xnrm2(mmi, A, iy + 1);
                vn1[j - 1] = temp;
                vn2[j - 1] = temp;
              } else {
                vn1[j - 1] = 0.0;
                vn2[j - 1] = 0.0;
              }
            } else {
              vn1[j - 1] = temp * std::sqrt(temp1);
            }
          }
        }
      }
    }
  }
}

} // namespace lapack
} // namespace internal
} // namespace coder

// End of code generation (xgeqp3.cpp)

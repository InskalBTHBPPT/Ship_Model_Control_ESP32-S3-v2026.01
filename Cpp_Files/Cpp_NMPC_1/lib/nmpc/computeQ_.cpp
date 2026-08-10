//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeQ_.cpp
//
// Code generation for function 'computeQ_'
//

// Include files
#include "computeQ_.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include <algorithm>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace QRManager {
void computeQ_(c_struct_T &obj, int nrows)
{
  double work[301];
  int iQR0;
  int k;
  int m;
  int n;
  k = obj.minRowCol;
  for (int idx{0}; idx < k; idx++) {
    iQR0 = 301 * idx + idx;
    n = obj.mrows - idx;
    if (n - 2 >= 0) {
      std::copy(&obj.QR[iQR0 + 1], &obj.QR[iQR0 + n], &obj.Q[iQR0 + 1]);
    }
  }
  m = obj.mrows;
  if (nrows >= 1) {
    int itau;
    for (int idx{k}; idx < nrows; idx++) {
      iQR0 = idx * 301;
      std::memset(&obj.Q[iQR0], 0,
                  static_cast<unsigned int>(m) * sizeof(double));
      obj.Q[iQR0 + idx] = 1.0;
    }
    itau = obj.minRowCol - 1;
    std::memset(&work[0], 0, 301U * sizeof(double));
    for (int i{obj.minRowCol}; i >= 1; i--) {
      int iaii;
      int lastc;
      iaii = i + (i - 1) * 301;
      if (i < nrows) {
        int lastv;
        obj.Q[iaii - 1] = 1.0;
        iQR0 = (m - i) - 1;
        k = iaii + 301;
        if (obj.tau[itau] != 0.0) {
          boolean_T exitg2;
          lastv = iQR0 + 2;
          iQR0 += iaii;
          while ((lastv > 0) && (obj.Q[iQR0] == 0.0)) {
            lastv--;
            iQR0--;
          }
          lastc = (nrows - i) - 1;
          exitg2 = false;
          while ((!exitg2) && (lastc + 1 > 0)) {
            int exitg1;
            iQR0 = (iaii + lastc * 301) + 300;
            n = iQR0;
            do {
              exitg1 = 0;
              if (n + 1 <= iQR0 + lastv) {
                if (obj.Q[n] != 0.0) {
                  exitg1 = 1;
                } else {
                  n++;
                }
              } else {
                lastc--;
                exitg1 = 2;
              }
            } while (exitg1 == 0);
            if (exitg1 == 1) {
              exitg2 = true;
            }
          }
        } else {
          lastv = 0;
          lastc = -1;
        }
        if (lastv > 0) {
          double c;
          if (lastc + 1 != 0) {
            if (lastc >= 0) {
              std::memset(&work[0], 0,
                          static_cast<unsigned int>(lastc + 1) *
                              sizeof(double));
            }
            n = (iaii + 301 * lastc) + 301;
            for (int iac{k}; iac <= n; iac += 301) {
              c = 0.0;
              iQR0 = iac + lastv;
              for (int idx{iac}; idx < iQR0; idx++) {
                c += obj.Q[idx - 1] * obj.Q[((iaii + idx) - iac) - 1];
              }
              iQR0 = div_nde_s32_floor((iac - iaii) - 301, 301);
              work[iQR0] += c;
            }
          }
          if (!(-obj.tau[itau] == 0.0)) {
            iQR0 = iaii;
            for (int idx{0}; idx <= lastc; idx++) {
              c = work[idx];
              if (c != 0.0) {
                c *= -obj.tau[itau];
                n = iQR0 + 301;
                k = lastv + iQR0;
                for (int iac{n}; iac <= k + 300; iac++) {
                  obj.Q[iac - 1] += obj.Q[((iaii + iac) - iQR0) - 302] * c;
                }
              }
              iQR0 += 301;
            }
          }
        }
      }
      if (i < m) {
        iQR0 = iaii + 1;
        n = (iaii + m) - i;
        k = ((n - iQR0) + 1) / 2 * 2 + iQR0;
        lastc = k - 2;
        for (int idx{iQR0}; idx <= lastc; idx += 2) {
          __m128d r;
          r = _mm_loadu_pd(&obj.Q[idx - 1]);
          r = _mm_mul_pd(_mm_set1_pd(-obj.tau[itau]), r);
          _mm_storeu_pd(&obj.Q[idx - 1], r);
        }
        for (int idx{k}; idx <= n; idx++) {
          obj.Q[idx - 1] *= -obj.tau[itau];
        }
      }
      obj.Q[iaii - 1] = 1.0 - obj.tau[itau];
      for (int idx{0}; idx <= i - 2; idx++) {
        obj.Q[(iaii - idx) - 2] = 0.0;
      }
      itau--;
    }
  }
}

} // namespace QRManager
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (computeQ_.cpp)

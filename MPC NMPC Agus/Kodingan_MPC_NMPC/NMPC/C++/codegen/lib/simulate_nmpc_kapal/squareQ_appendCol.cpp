//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// squareQ_appendCol.cpp
//
// Code generation for function 'squareQ_appendCol'
//

// Include files
#include "squareQ_appendCol.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "xrotg.h"
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace QRManager {
void squareQ_appendCol(c_struct_T &obj, const double vec[45451], int iv0)
{
  double c;
  double s;
  int Qk0;
  int idx;
  int iyend;
  int temp_tmp;
  iyend = obj.mrows;
  Qk0 = obj.ncols + 1;
  if (iyend <= Qk0) {
    Qk0 = iyend;
  }
  obj.minRowCol = Qk0;
  idx = 301 * obj.ncols;
  if (obj.mrows != 0) {
    iyend = idx + obj.mrows;
    if (idx + 1 <= iyend) {
      std::memset(&obj.QR[idx], 0,
                  static_cast<unsigned int>(iyend - idx) * sizeof(double));
    }
    temp_tmp = 301 * (obj.mrows - 1) + 1;
    for (int iac{1}; iac <= temp_tmp; iac += 301) {
      c = 0.0;
      iyend = iac + obj.mrows;
      for (int k{iac}; k < iyend; k++) {
        c += obj.Q[k - 1] * vec[((iv0 + k) - iac) - 1];
      }
      Qk0 = idx + div_nde_s32_floor(iac - 1, 301);
      obj.QR[Qk0] += c;
    }
  }
  obj.ncols++;
  obj.jpvt[obj.ncols - 1] = obj.ncols;
  for (idx = obj.mrows - 2; idx + 2 > obj.ncols; idx--) {
    double b_c;
    iyend = idx + 301 * (obj.ncols - 1);
    c = obj.QR[iyend + 1];
    b_c = internal::blas::xrotg(obj.QR[iyend], c, s);
    obj.QR[iyend + 1] = c;
    Qk0 = 301 * idx;
    iyend = obj.mrows;
    if (obj.mrows >= 1) {
      for (int k{0}; k < iyend; k++) {
        double b_temp_tmp;
        temp_tmp = Qk0 + k;
        c = obj.Q[temp_tmp + 301];
        b_temp_tmp = obj.Q[temp_tmp];
        obj.Q[temp_tmp + 301] = b_c * c - s * b_temp_tmp;
        obj.Q[temp_tmp] = b_c * b_temp_tmp + s * c;
      }
    }
  }
}

} // namespace QRManager
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (squareQ_appendCol.cpp)

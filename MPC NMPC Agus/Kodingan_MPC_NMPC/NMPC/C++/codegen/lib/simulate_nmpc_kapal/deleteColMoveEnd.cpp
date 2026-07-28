//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// deleteColMoveEnd.cpp
//
// Code generation for function 'deleteColMoveEnd'
//

// Include files
#include "deleteColMoveEnd.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "xrotg.h"
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace QRManager {
void deleteColMoveEnd(c_struct_T &obj, int idx)
{
  int i;
  if (obj.usedPivoting) {
    i = 1;
    while ((i <= obj.ncols) && (obj.jpvt[i - 1] != idx)) {
      i++;
    }
    idx = i;
  }
  if (idx >= obj.ncols) {
    obj.ncols--;
  } else {
    int u1;
    obj.jpvt[idx - 1] = obj.jpvt[obj.ncols - 1];
    i = obj.minRowCol;
    for (int k{0}; k < i; k++) {
      obj.QR[k + 301 * (idx - 1)] = obj.QR[k + 301 * (obj.ncols - 1)];
    }
    obj.ncols--;
    i = obj.mrows;
    u1 = obj.ncols;
    if (i <= u1) {
      u1 = i;
    }
    obj.minRowCol = u1;
    if (idx < obj.mrows) {
      double c;
      double s;
      double temp;
      int QRk0;
      int b_i;
      int b_k;
      int endIdx;
      int idxRotGCol;
      i = obj.mrows - 1;
      endIdx = obj.ncols;
      if (i <= endIdx) {
        endIdx = i;
      }
      b_k = endIdx;
      idxRotGCol = 301 * (idx - 1);
      while (b_k >= idx) {
        i = b_k + idxRotGCol;
        temp = obj.QR[i];
        c = internal::blas::xrotg(obj.QR[i - 1], temp, s);
        obj.QR[i] = temp;
        b_i = 301 * (b_k - 1);
        obj.QR[b_k + b_i] = 0.0;
        QRk0 = b_k + 301 * idx;
        i = obj.ncols - idx;
        if (i >= 1) {
          for (int k{0}; k < i; k++) {
            u1 = QRk0 + k * 301;
            temp = c * obj.QR[u1 - 1] + s * obj.QR[u1];
            obj.QR[u1] = c * obj.QR[u1] - s * obj.QR[u1 - 1];
            obj.QR[u1 - 1] = temp;
          }
        }
        i = obj.mrows;
        for (int k{0}; k < i; k++) {
          u1 = b_i + k;
          temp = c * obj.Q[u1] + s * obj.Q[u1 + 301];
          obj.Q[u1 + 301] = c * obj.Q[u1 + 301] - s * obj.Q[u1];
          obj.Q[u1] = temp;
        }
        b_k--;
      }
      b_i = idx + 1;
      for (int c_k{b_i}; c_k <= endIdx; c_k++) {
        idxRotGCol = 301 * (c_k - 1);
        i = c_k + idxRotGCol;
        temp = obj.QR[i];
        c = internal::blas::xrotg(obj.QR[i - 1], temp, s);
        obj.QR[i] = temp;
        QRk0 = c_k * 302;
        i = obj.ncols - c_k;
        if (i >= 1) {
          for (int k{0}; k < i; k++) {
            u1 = QRk0 + k * 301;
            temp = c * obj.QR[u1 - 1] + s * obj.QR[u1];
            obj.QR[u1] = c * obj.QR[u1] - s * obj.QR[u1 - 1];
            obj.QR[u1 - 1] = temp;
          }
        }
        i = obj.mrows;
        for (int k{0}; k < i; k++) {
          u1 = idxRotGCol + k;
          temp = c * obj.Q[u1] + s * obj.Q[u1 + 301];
          obj.Q[u1 + 301] = c * obj.Q[u1 + 301] - s * obj.Q[u1];
          obj.Q[u1] = temp;
        }
      }
    }
  }
}

} // namespace QRManager
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (deleteColMoveEnd.cpp)

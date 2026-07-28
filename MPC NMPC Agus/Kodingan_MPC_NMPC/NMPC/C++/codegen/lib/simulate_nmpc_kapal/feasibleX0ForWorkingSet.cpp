//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// feasibleX0ForWorkingSet.cpp
//
// Code generation for function 'feasibleX0ForWorkingSet'
//

// Include files
#include "feasibleX0ForWorkingSet.h"
#include "computeQ_.h"
#include "factorQRE.h"
#include "maxConstraintViolation.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "xgeqp3.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace initialize {
boolean_T feasibleX0ForWorkingSet(double workspace[45451], double xCurrent[151],
                                  g_struct_T &workingset, c_struct_T &qrmanager)
{
  static double B[45451];
  int mWConstr;
  int nVar;
  boolean_T nonDegenerateWset;
  mWConstr = workingset.nActiveConstr;
  nVar = workingset.nVar;
  nonDegenerateWset = true;
  if (mWConstr != 0) {
    __m128d r;
    __m128d r1;
    double tol;
    int br;
    int offsetQR;
    if (mWConstr >= nVar) {
      int minmn;
      int rankQR;
      int scalarLB;
      int temp_tmp;
      scalarLB = static_cast<unsigned char>(nVar);
      for (int idx_col{0}; idx_col < scalarLB; idx_col++) {
        offsetQR = 301 * idx_col;
        for (int idx_row{0}; idx_row < mWConstr; idx_row++) {
          qrmanager.QR[idx_row + offsetQR] =
              workingset.ATwset[idx_col + 151 * idx_row];
        }
        qrmanager.jpvt[idx_col] = 0;
      }
      if (mWConstr * nVar == 0) {
        qrmanager.mrows = mWConstr;
        qrmanager.ncols = nVar;
        qrmanager.minRowCol = 0;
      } else {
        qrmanager.usedPivoting = true;
        qrmanager.mrows = mWConstr;
        qrmanager.ncols = nVar;
        if (mWConstr <= nVar) {
          offsetQR = mWConstr;
        } else {
          offsetQR = nVar;
        }
        qrmanager.minRowCol = offsetQR;
        internal::lapack::xgeqp3(qrmanager.QR, mWConstr, nVar, qrmanager.jpvt,
                                 qrmanager.tau);
      }
      QRManager::computeQ_(qrmanager, qrmanager.mrows);
      rankQR = 0;
      br = qrmanager.mrows;
      minmn = qrmanager.ncols;
      if (br <= minmn) {
        minmn = br;
      }
      if (minmn > 0) {
        offsetQR = qrmanager.ncols;
        if (br >= offsetQR) {
          offsetQR = br;
        }
        tol =
            std::fmin(1.4901161193847656E-8,
                      2.2204460492503131E-15 * static_cast<double>(offsetQR)) *
            std::abs(qrmanager.QR[0]);
        while ((rankQR < minmn) &&
               (!(std::abs(qrmanager.QR[rankQR + 301 * rankQR]) <= tol))) {
          rankQR++;
        }
      }
      for (int idx_col{0}; idx_col < mWConstr; idx_col++) {
        tol = workingset.bwset[idx_col];
        workspace[idx_col] = tol;
        workspace[idx_col + 301] = tol;
      }
      offsetQR = 151 * (mWConstr - 1) + 1;
      for (int idx_row{1}; idx_row <= offsetQR; idx_row += 151) {
        tol = 0.0;
        br = idx_row + nVar;
        for (int idx_col{idx_row}; idx_col < br; idx_col++) {
          tol += workingset.ATwset[idx_col - 1] * xCurrent[idx_col - idx_row];
        }
        br = div_nde_s32_floor(idx_row - 1, 151);
        workspace[br] -= tol;
      }
      std::copy(&workspace[0], &workspace[45451], &B[0]);
      for (int idx_col{0}; idx_col <= 301; idx_col += 301) {
        br = idx_col + 1;
        offsetQR = idx_col + nVar;
        if (br <= offsetQR) {
          std::memset(&workspace[br + -1], 0,
                      static_cast<unsigned int>((offsetQR - br) + 1) *
                          sizeof(double));
        }
      }
      br = -1;
      for (int idx_col{0}; idx_col <= 301; idx_col += 301) {
        offsetQR = -1;
        minmn = idx_col + 1;
        temp_tmp = idx_col + nVar;
        for (int idx_row{minmn}; idx_row <= temp_tmp; idx_row++) {
          tol = 0.0;
          for (int w{0}; w < mWConstr; w++) {
            tol += qrmanager.Q[(w + offsetQR) + 1] * B[(w + br) + 1];
          }
          workspace[idx_row - 1] += tol;
          offsetQR += 301;
        }
        br += 301;
      }
      for (int idx_col{0}; idx_col < 2; idx_col++) {
        offsetQR = 301 * idx_col - 1;
        for (int idx_row{rankQR}; idx_row >= 1; idx_row--) {
          minmn = 301 * (idx_row - 1) - 1;
          temp_tmp = idx_row + offsetQR;
          tol = workspace[temp_tmp];
          if (tol != 0.0) {
            workspace[temp_tmp] = tol / qrmanager.QR[idx_row + minmn];
            br = static_cast<unsigned char>(idx_row - 1);
            for (int w{0}; w < br; w++) {
              mWConstr = (w + offsetQR) + 1;
              workspace[mWConstr] -=
                  workspace[temp_tmp] * qrmanager.QR[(w + minmn) + 1];
            }
          }
        }
      }
      offsetQR = rankQR + 1;
      for (int idx_col{offsetQR}; idx_col <= nVar; idx_col++) {
        workspace[idx_col - 1] = 0.0;
        workspace[idx_col + 300] = 0.0;
      }
      for (int idx_col{0}; idx_col < scalarLB; idx_col++) {
        workspace[qrmanager.jpvt[idx_col] + 601] = workspace[idx_col];
      }
      for (int idx_col{0}; idx_col < scalarLB; idx_col++) {
        workspace[idx_col] = workspace[idx_col + 602];
      }
      for (int idx_col{0}; idx_col < scalarLB; idx_col++) {
        workspace[qrmanager.jpvt[idx_col] + 601] = workspace[idx_col + 301];
      }
      for (int idx_col{0}; idx_col < scalarLB; idx_col++) {
        workspace[idx_col + 301] = workspace[idx_col + 602];
      }
    } else {
      int minmn;
      int rankQR;
      int temp_tmp;
      if (mWConstr - 1 >= 0) {
        std::memset(&qrmanager.jpvt[0], 0,
                    static_cast<unsigned int>(mWConstr) * sizeof(int));
      }
      QRManager::factorQRE(qrmanager, workingset.ATwset, nVar, mWConstr);
      QRManager::computeQ_(qrmanager, qrmanager.minRowCol);
      rankQR = 0;
      offsetQR = qrmanager.mrows;
      minmn = qrmanager.ncols;
      if (offsetQR <= minmn) {
        minmn = offsetQR;
      }
      if (minmn > 0) {
        br = qrmanager.mrows;
        offsetQR = qrmanager.ncols;
        if (br >= offsetQR) {
          offsetQR = br;
        }
        tol =
            std::fmin(1.4901161193847656E-8,
                      2.2204460492503131E-15 * static_cast<double>(offsetQR)) *
            std::abs(qrmanager.QR[0]);
        while ((rankQR < minmn) &&
               (!(std::abs(qrmanager.QR[rankQR + 301 * rankQR]) <= tol))) {
          rankQR++;
        }
      }
      for (int idx_col{0}; idx_col < mWConstr; idx_col++) {
        br = (qrmanager.jpvt[idx_col] - 1) * 151;
        tol = 0.0;
        offsetQR = static_cast<unsigned char>(nVar);
        for (int idx_row{0}; idx_row < offsetQR; idx_row++) {
          tol += workingset.ATwset[br + idx_row] * xCurrent[idx_row];
        }
        workspace[idx_col] =
            workingset.bwset[qrmanager.jpvt[idx_col] - 1] - tol;
        workspace[idx_col + 301] =
            workingset.bwset[qrmanager.jpvt[idx_col] - 1];
      }
      br = static_cast<unsigned char>(rankQR);
      for (int idx_col{0}; idx_col < 2; idx_col++) {
        offsetQR = 301 * idx_col;
        for (int idx_row{0}; idx_row < br; idx_row++) {
          minmn = 301 * idx_row;
          temp_tmp = idx_row + offsetQR;
          tol = workspace[temp_tmp];
          for (int w{0}; w < idx_row; w++) {
            tol -= qrmanager.QR[w + minmn] * workspace[w + offsetQR];
          }
          workspace[temp_tmp] = tol / qrmanager.QR[idx_row + minmn];
        }
      }
      std::copy(&workspace[0], &workspace[45451], &B[0]);
      for (int idx_col{0}; idx_col <= 301; idx_col += 301) {
        offsetQR = idx_col + 1;
        br = idx_col + nVar;
        if (offsetQR <= br) {
          std::memset(&workspace[offsetQR + -1], 0,
                      static_cast<unsigned int>((br - offsetQR) + 1) *
                          sizeof(double));
        }
      }
      offsetQR = 1;
      for (int idx_col{0}; idx_col <= 301; idx_col += 301) {
        minmn = -1;
        temp_tmp = offsetQR + rankQR;
        for (int idx_row{offsetQR}; idx_row < temp_tmp; idx_row++) {
          int scalarLB;
          int vectorUB;
          br = idx_col + 1;
          mWConstr = idx_col + nVar;
          scalarLB = ((mWConstr - br) + 1) / 2 * 2 + br;
          vectorUB = scalarLB - 2;
          for (int w{br}; w <= vectorUB; w += 2) {
            r = _mm_loadu_pd(&qrmanager.Q[(minmn + w) - idx_col]);
            r = _mm_mul_pd(_mm_set1_pd(B[idx_row - 1]), r);
            r1 = _mm_loadu_pd(&workspace[w - 1]);
            r = _mm_add_pd(r1, r);
            _mm_storeu_pd(&workspace[w - 1], r);
          }
          for (int w{scalarLB}; w <= mWConstr; w++) {
            workspace[w - 1] +=
                B[idx_row - 1] * qrmanager.Q[(minmn + w) - idx_col];
          }
          minmn += 301;
        }
        offsetQR += 301;
      }
    }
    offsetQR = 0;
    int exitg1;
    do {
      exitg1 = 0;
      if (offsetQR <= static_cast<unsigned char>(nVar) - 1) {
        if (std::isinf(workspace[offsetQR]) ||
            std::isnan(workspace[offsetQR]) ||
            (std::isinf(workspace[offsetQR + 301]) ||
             std::isnan(workspace[offsetQR + 301]))) {
          nonDegenerateWset = false;
          exitg1 = 1;
        } else {
          offsetQR++;
        }
      } else {
        double v;
        offsetQR = nVar / 2 * 2;
        br = offsetQR - 2;
        for (int idx_col{0}; idx_col <= br; idx_col += 2) {
          r = _mm_loadu_pd(&workspace[idx_col]);
          r1 = _mm_loadu_pd(&xCurrent[idx_col]);
          r = _mm_add_pd(r, r1);
          _mm_storeu_pd(&workspace[idx_col], r);
        }
        for (int idx_col{offsetQR}; idx_col < nVar; idx_col++) {
          workspace[idx_col] += xCurrent[idx_col];
        }
        if (workingset.probType == 2) {
          v = 0.0;
          for (int idx_col{0}; idx_col <= 118; idx_col += 2) {
            r = _mm_loadu_pd(&workingset.bineq[idx_col]);
            _mm_storeu_pd(&workingset.maxConstrWorkspace[idx_col],
                          _mm_mul_pd(r, _mm_set1_pd(-1.0)));
          }
          for (int idx_row{0}; idx_row <= 17969; idx_row += 151) {
            tol = 0.0;
            offsetQR = idx_row + 30;
            for (int idx_col{idx_row + 1}; idx_col <= offsetQR; idx_col++) {
              tol += workingset.Aineq[idx_col - 1] *
                     workspace[(idx_col - idx_row) - 1];
            }
            offsetQR = div_nde_s32_floor(idx_row, 151);
            workingset.maxConstrWorkspace[offsetQR] += tol;
          }
          for (int idx_col{0}; idx_col < 120; idx_col++) {
            tol = workingset.maxConstrWorkspace[idx_col] -
                  workspace[idx_col + 30];
            workingset.maxConstrWorkspace[idx_col] = tol;
            v = std::fmax(v, tol);
          }
        } else {
          v = 0.0;
          for (int idx_col{0}; idx_col <= 118; idx_col += 2) {
            r = _mm_loadu_pd(&workingset.bineq[idx_col]);
            _mm_storeu_pd(&workingset.maxConstrWorkspace[idx_col],
                          _mm_mul_pd(r, _mm_set1_pd(-1.0)));
          }
          for (int idx_row{0}; idx_row <= 17969; idx_row += 151) {
            tol = 0.0;
            offsetQR = idx_row + workingset.nVar;
            for (int idx_col{idx_row + 1}; idx_col <= offsetQR; idx_col++) {
              tol += workingset.Aineq[idx_col - 1] *
                     workspace[(idx_col - idx_row) - 1];
            }
            offsetQR = div_nde_s32_floor(idx_row, 151);
            workingset.maxConstrWorkspace[offsetQR] += tol;
          }
          for (int idx_col{0}; idx_col < 120; idx_col++) {
            v = std::fmax(v, workingset.maxConstrWorkspace[idx_col]);
          }
        }
        offsetQR = static_cast<unsigned char>(workingset.sizes[3]);
        for (int idx_col{0}; idx_col < offsetQR; idx_col++) {
          br = workingset.indexLB[idx_col] - 1;
          v = std::fmax(v, -workspace[br] - workingset.lb[br]);
        }
        for (int idx_col{0}; idx_col < 30; idx_col++) {
          offsetQR = workingset.indexUB[idx_col] - 1;
          v = std::fmax(v, workspace[offsetQR] - workingset.ub[offsetQR]);
        }
        tol = WorkingSet::maxConstraintViolation(workingset, workspace);
        if ((v <= 2.2204460492503131E-16) || (v < tol)) {
          offsetQR = static_cast<unsigned char>(nVar);
          std::copy(&workspace[0], &workspace[offsetQR], &xCurrent[0]);
        } else {
          offsetQR = static_cast<unsigned char>(nVar);
          std::copy(&workspace[301], &workspace[301 + offsetQR], &xCurrent[0]);
        }
        exitg1 = 1;
      }
    } while (exitg1 == 0);
  }
  return nonDegenerateWset;
}

} // namespace initialize
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (feasibleX0ForWorkingSet.cpp)

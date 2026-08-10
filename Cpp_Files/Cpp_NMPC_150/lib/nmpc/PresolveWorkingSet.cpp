//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// PresolveWorkingSet.cpp
//
// Code generation for function 'PresolveWorkingSet'
//

// Include files
#include "PresolveWorkingSet.h"
#include "RemoveDependentIneq_.h"
#include "computeQ_.h"
#include "countsort.h"
#include "feasibleX0ForWorkingSet.h"
#include "maxConstraintViolation.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "xgeqp3.h"
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace initialize {
void PresolveWorkingSet(e_struct_T &solution, b_struct_T &memspace,
                        g_struct_T &workingset, c_struct_T &qrmanager)
{
  double tol;
  int mTotalWorkingEq;
  int nDepInd;
  int totalRank;
  int workingset_nActiveConstr;
  solution.state = 82;
  workingset_nActiveConstr = workingset.nActiveConstr;
  mTotalWorkingEq = workingset.nWConstr[0] + workingset.nWConstr[1];
  nDepInd = 0;
  if (mTotalWorkingEq > 0) {
    int i;
    int ix;
    i = static_cast<unsigned char>(workingset.nVar);
    for (int idx_row{0}; idx_row < mTotalWorkingEq; idx_row++) {
      for (int idx_col{0}; idx_col < i; idx_col++) {
        qrmanager.QR[idx_row + 301 * idx_col] =
            workingset.ATwset[idx_col + 151 * idx_row];
      }
    }
    nDepInd = mTotalWorkingEq - workingset.nVar;
    if (nDepInd <= 0) {
      nDepInd = 0;
    }
    totalRank = static_cast<unsigned char>(workingset.nVar);
    if (totalRank - 1 >= 0) {
      std::memset(&qrmanager.jpvt[0], 0,
                  static_cast<unsigned int>(totalRank) * sizeof(int));
    }
    if (mTotalWorkingEq * workingset.nVar == 0) {
      qrmanager.mrows = mTotalWorkingEq;
      qrmanager.ncols = workingset.nVar;
      qrmanager.minRowCol = 0;
    } else {
      qrmanager.usedPivoting = true;
      qrmanager.mrows = mTotalWorkingEq;
      qrmanager.ncols = workingset.nVar;
      totalRank = workingset.nVar;
      if (mTotalWorkingEq <= totalRank) {
        totalRank = mTotalWorkingEq;
      }
      qrmanager.minRowCol = totalRank;
      internal::lapack::xgeqp3(qrmanager.QR, mTotalWorkingEq, workingset.nVar,
                               qrmanager.jpvt, qrmanager.tau);
    }
    totalRank = workingset.nVar;
    if (mTotalWorkingEq >= totalRank) {
      totalRank = mTotalWorkingEq;
    }
    tol = std::fmin(1.4901161193847656E-8,
                    2.2204460492503131E-15 * static_cast<double>(totalRank));
    totalRank = workingset.nVar;
    if (totalRank > mTotalWorkingEq) {
      totalRank = mTotalWorkingEq;
    }
    totalRank += 301 * (totalRank - 1);
    while ((totalRank > 0) && (std::abs(qrmanager.QR[totalRank - 1]) <=
                               tol * std::abs(qrmanager.QR[0]))) {
      totalRank -= 302;
      nDepInd++;
    }
    if (nDepInd > 0) {
      boolean_T exitg1;
      QRManager::computeQ_(qrmanager, qrmanager.mrows);
      totalRank = 0;
      exitg1 = false;
      while ((!exitg1) && (totalRank <= nDepInd - 1)) {
        double qtb;
        ix = 301 * ((mTotalWorkingEq - totalRank) - 1);
        qtb = 0.0;
        for (int idx_row{0}; idx_row < mTotalWorkingEq; idx_row++) {
          qtb += qrmanager.Q[ix + idx_row] * workingset.bwset[idx_row];
        }
        if (std::abs(qtb) >= tol) {
          nDepInd = -1;
          exitg1 = true;
        } else {
          totalRank++;
        }
      }
    }
    if (nDepInd > 0) {
      for (int idx_row{0}; idx_row < mTotalWorkingEq; idx_row++) {
        totalRank = 301 * idx_row;
        ix = 151 * idx_row;
        for (int idx_col{0}; idx_col < i; idx_col++) {
          qrmanager.QR[totalRank + idx_col] = workingset.ATwset[ix + idx_col];
        }
      }
      totalRank = workingset.nWConstr[0];
      for (int idx_row{0}; idx_row < totalRank; idx_row++) {
        qrmanager.jpvt[idx_row] = 1;
      }
      totalRank = workingset.nWConstr[0] + 1;
      if (totalRank <= mTotalWorkingEq) {
        std::memset(
            &qrmanager.jpvt[totalRank + -1], 0,
            static_cast<unsigned int>((mTotalWorkingEq - totalRank) + 1) *
                sizeof(int));
      }
      if (workingset.nVar * mTotalWorkingEq == 0) {
        qrmanager.mrows = workingset.nVar;
        qrmanager.ncols = mTotalWorkingEq;
        qrmanager.minRowCol = 0;
      } else {
        qrmanager.usedPivoting = true;
        qrmanager.mrows = workingset.nVar;
        qrmanager.ncols = mTotalWorkingEq;
        totalRank = workingset.nVar;
        if (totalRank > mTotalWorkingEq) {
          totalRank = mTotalWorkingEq;
        }
        qrmanager.minRowCol = totalRank;
        internal::lapack::xgeqp3(qrmanager.QR, workingset.nVar, mTotalWorkingEq,
                                 qrmanager.jpvt, qrmanager.tau);
      }
      for (int idx_row{0}; idx_row < nDepInd; idx_row++) {
        memspace.workspace_int[idx_row] =
            qrmanager.jpvt[(mTotalWorkingEq - nDepInd) + idx_row];
      }
      utils::countsort(memspace.workspace_int, nDepInd, memspace.workspace_sort,
                       1, mTotalWorkingEq);
    }
  }
  if ((nDepInd != -1) && (workingset.nActiveConstr <= 301)) {
    boolean_T guard1;
    boolean_T okWorkingSet;
    RemoveDependentIneq_(workingset, qrmanager, memspace, 1.0);
    okWorkingSet = feasibleX0ForWorkingSet(
        memspace.workspace_float, solution.xstar, workingset, qrmanager);
    guard1 = false;
    if (!okWorkingSet) {
      RemoveDependentIneq_(workingset, qrmanager, memspace, 10.0);
      okWorkingSet = feasibleX0ForWorkingSet(
          memspace.workspace_float, solution.xstar, workingset, qrmanager);
      if (!okWorkingSet) {
        solution.state = -7;
      } else {
        guard1 = true;
      }
    } else {
      guard1 = true;
    }
    if (guard1 &&
        (workingset.nWConstr[0] + workingset.nWConstr[1] == workingset.nVar)) {
      tol = WorkingSet::b_maxConstraintViolation(workingset, solution.xstar);
      if (tol > 1.0E-6) {
        solution.state = -2;
      }
    }
  } else {
    solution.state = -3;
    totalRank = (workingset.nWConstr[0] + workingset.nWConstr[1]) + 1;
    for (int idx_row{totalRank}; idx_row <= workingset_nActiveConstr;
         idx_row++) {
      workingset.isActiveConstr
          [(workingset.isActiveIdx[workingset.Wid[idx_row - 1] - 1] +
            workingset.Wlocalidx[idx_row - 1]) -
           2] = false;
    }
    workingset.nWConstr[2] = 0;
    workingset.nWConstr[3] = 0;
    workingset.nWConstr[4] = 0;
    workingset.nActiveConstr = workingset.nWConstr[0] + workingset.nWConstr[1];
  }
}

} // namespace initialize
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (PresolveWorkingSet.cpp)

//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// iterate.cpp
//
// Code generation for function 'iterate'
//

// Include files
#include "iterate.h"
#include "addBoundToActiveSetMatrix_.h"
#include "checkStoppingAndUpdateFval.h"
#include "computeFval_ReuseHx.h"
#include "computeGrad_StoreHx.h"
#include "computeQ_.h"
#include "compute_deltax.h"
#include "compute_lambda.h"
#include "deleteColMoveEnd.h"
#include "factorQR.h"
#include "feasibleratiotest.h"
#include "removeConstr.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "squareQ_appendCol.h"
#include "xnrm2.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
void iterate(const double H[900], const double f[151], e_struct_T &solution,
             b_struct_T &memspace, g_struct_T &workingset,
             c_struct_T &qrmanager, i_struct_T &cholmanager,
             struct_T &objective, const char options_SolverName[7],
             int runTimeOptions_MaxIterations)
{
  static const char b[7]{'f', 'm', 'i', 'n', 'c', 'o', 'n'};
  int TYPE;
  int activeSetChangeID;
  int globalActiveConstrIdx;
  int idx_local;
  int nVar;
  int ret;
  boolean_T subProblemChanged;
  boolean_T updateFval;
  subProblemChanged = true;
  updateFval = true;
  activeSetChangeID = 0;
  TYPE = objective.objtype;
  nVar = workingset.nVar;
  globalActiveConstrIdx = 0;
  Objective::computeGrad_StoreHx(objective, H, f, solution.xstar);
  solution.fstar = Objective::computeFval_ReuseHx(
      objective, memspace.workspace_float, f, solution.xstar);
  if (solution.iterations < runTimeOptions_MaxIterations) {
    solution.state = -5;
  } else {
    solution.state = 0;
  }
  std::memset(&solution.lambda[0], 0, 301U * sizeof(double));
  int exitg1;
  do {
    exitg1 = 0;
    if (solution.state == -5) {
      double minLambda;
      int i;
      int iAw0;
      boolean_T guard1;
      guard1 = false;
      if (subProblemChanged) {
        switch (activeSetChangeID) {
        case 1:
          QRManager::squareQ_appendCol(qrmanager, workingset.ATwset,
                                       151 * (workingset.nActiveConstr - 1) +
                                           1);
          break;
        case -1:
          QRManager::deleteColMoveEnd(qrmanager, globalActiveConstrIdx);
          break;
        default:
          QRManager::factorQR(qrmanager, workingset.ATwset, nVar,
                              workingset.nActiveConstr);
          QRManager::computeQ_(qrmanager, qrmanager.mrows);
          break;
        }
        ret = std::memcmp(&options_SolverName[0], &b[0], 7);
        compute_deltax(H, solution, memspace, qrmanager, cholmanager, objective,
                       ret == 0);
        if (solution.state != -5) {
          exitg1 = 1;
        } else if ((internal::blas::xnrm2(nVar, solution.searchDir) < 1.0E-6) ||
                   (workingset.nActiveConstr >= nVar)) {
          guard1 = true;
        } else {
          minLambda = feasibleratiotest(
              solution.xstar, solution.searchDir, memspace.workspace_float,
              workingset.nVar, workingset.Aineq, workingset.bineq,
              workingset.lb, workingset.ub, workingset.indexLB,
              workingset.indexUB, workingset.sizes, workingset.isActiveIdx,
              workingset.isActiveConstr, workingset.nWConstr, TYPE == 5,
              updateFval, ret, idx_local);
          if (updateFval) {
            switch (ret) {
            case 3:
              workingset.nWConstr[2]++;
              workingset
                  .isActiveConstr[(workingset.isActiveIdx[2] + idx_local) - 2] =
                  true;
              workingset.nActiveConstr++;
              workingset.Wid[workingset.nActiveConstr - 1] = 3;
              workingset.Wlocalidx[workingset.nActiveConstr - 1] = idx_local;
              ret = 151 * (idx_local - 1);
              iAw0 = 151 * (workingset.nActiveConstr - 1);
              i = workingset.nVar;
              for (int idx{0}; idx < i; idx++) {
                workingset.ATwset[iAw0 + idx] = workingset.Aineq[ret + idx];
              }
              workingset.bwset[workingset.nActiveConstr - 1] =
                  workingset.bineq[idx_local - 1];
              break;
            case 4:
              WorkingSet::addBoundToActiveSetMatrix_(workingset, 4, idx_local);
              break;
            default:
              WorkingSet::addBoundToActiveSetMatrix_(workingset, 5, idx_local);
              break;
            }
            activeSetChangeID = 1;
          } else {
            if (objective.objtype == 5) {
              if (internal::blas::xnrm2(objective.nvar, solution.searchDir) >
                  100.0 * static_cast<double>(objective.nvar) *
                      1.4901161193847656E-8) {
                solution.state = 3;
              } else {
                solution.state = 4;
              }
            }
            subProblemChanged = false;
            if (workingset.nActiveConstr == 0) {
              solution.state = 1;
            }
          }
          if ((nVar >= 1) && (!(minLambda == 0.0))) {
            ret = (nVar / 2) << 1;
            iAw0 = ret - 2;
            for (int idx{0}; idx <= iAw0; idx += 2) {
              __m128d r;
              __m128d r1;
              r = _mm_loadu_pd(&solution.searchDir[idx]);
              r1 = _mm_loadu_pd(&solution.xstar[idx]);
              _mm_storeu_pd(
                  &solution.xstar[idx],
                  _mm_add_pd(r1, _mm_mul_pd(_mm_set1_pd(minLambda), r)));
            }
            for (int idx{ret}; idx < nVar; idx++) {
              solution.xstar[idx] += minLambda * solution.searchDir[idx];
            }
          }
          Objective::computeGrad_StoreHx(objective, H, f, solution.xstar);
          updateFval = true;
          stopping::b_checkStoppingAndUpdateFval(
              activeSetChangeID, solution, memspace, objective, workingset,
              qrmanager, runTimeOptions_MaxIterations, updateFval);
        }
      } else {
        ret = static_cast<unsigned char>(nVar);
        std::memset(&solution.searchDir[0], 0,
                    static_cast<unsigned int>(ret) * sizeof(double));
        guard1 = true;
      }
      if (guard1) {
        compute_lambda(memspace.workspace_float, solution, objective,
                       qrmanager);
        if ((solution.state != -7) || (workingset.nActiveConstr > nVar)) {
          ret = 0;
          minLambda = 0.0;
          iAw0 = (workingset.nWConstr[0] + workingset.nWConstr[1]) + 1;
          i = workingset.nActiveConstr;
          for (int idx{iAw0}; idx <= i; idx++) {
            double d;
            d = solution.lambda[idx - 1];
            if (d < minLambda) {
              minLambda = d;
              ret = idx;
            }
          }
          if (ret == 0) {
            solution.state = 1;
          } else {
            activeSetChangeID = -1;
            globalActiveConstrIdx = ret;
            subProblemChanged = true;
            WorkingSet::removeConstr(workingset, ret);
            if (ret < workingset.nActiveConstr + 1) {
              solution.lambda[ret - 1] =
                  solution.lambda[workingset.nActiveConstr];
            }
            solution.lambda[workingset.nActiveConstr] = 0.0;
          }
        } else {
          ret = workingset.nActiveConstr;
          activeSetChangeID = 0;
          globalActiveConstrIdx = workingset.nActiveConstr;
          subProblemChanged = true;
          WorkingSet::removeConstr(workingset, workingset.nActiveConstr);
          solution.lambda[ret - 1] = 0.0;
        }
        updateFval = false;
        stopping::b_checkStoppingAndUpdateFval(
            activeSetChangeID, solution, memspace, objective, workingset,
            qrmanager, runTimeOptions_MaxIterations, updateFval);
      }
    } else {
      if (!updateFval) {
        solution.fstar = Objective::computeFval_ReuseHx(
            objective, memspace.workspace_float, f, solution.xstar);
      }
      exitg1 = 1;
    }
  } while (exitg1 == 0);
}

} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (iterate.cpp)

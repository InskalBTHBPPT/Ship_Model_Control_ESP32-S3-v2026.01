//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// phaseone.cpp
//
// Code generation for function 'phaseone'
//

// Include files
#include "phaseone.h"
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
#include "setProblemType.h"
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
void phaseone(const double H[900], const double f[151], e_struct_T &solution,
              b_struct_T &memspace, g_struct_T &workingset,
              c_struct_T &qrmanager, i_struct_T &cholmanager,
              struct_T &objective, const char options_SolverName[7],
              const k_struct_T &runTimeOptions)
{
  static const char b[7]{'f', 'm', 'i', 'n', 'c', 'o', 'n'};
  int PROBTYPE_ORIG;
  int activeSetChangeID;
  int b_nVar;
  int globalActiveConstrIdx;
  int idxEndIneq;
  int idxStartIneq;
  int idx_local;
  int nVar;
  boolean_T subProblemChanged;
  boolean_T updateFval;
  PROBTYPE_ORIG = workingset.probType;
  nVar = workingset.nVar;
  solution.xstar[workingset.nVar] = solution.maxConstr + 1.0;
  if (workingset.probType == 3) {
    idxStartIneq = 1;
  } else {
    idxStartIneq = 4;
  }
  WorkingSet::setProblemType(workingset, idxStartIneq);
  idxStartIneq = (workingset.nWConstr[0] + workingset.nWConstr[1]) + 1;
  idxEndIneq = workingset.nActiveConstr;
  for (int idx_global{idxStartIneq}; idx_global <= idxEndIneq; idx_global++) {
    workingset.isActiveConstr
        [(workingset.isActiveIdx[workingset.Wid[idx_global - 1] - 1] +
          workingset.Wlocalidx[idx_global - 1]) -
         2] = false;
  }
  workingset.nWConstr[2] = 0;
  workingset.nWConstr[3] = 0;
  workingset.nWConstr[4] = 0;
  workingset.nActiveConstr = workingset.nWConstr[0] + workingset.nWConstr[1];
  objective.prev_objtype = objective.objtype;
  objective.prev_nvar = objective.nvar;
  objective.prev_hasLinear = objective.hasLinear;
  objective.objtype = 5;
  objective.nvar = nVar + 1;
  objective.gammaScalar = 1.0;
  objective.hasLinear = true;
  subProblemChanged = true;
  updateFval = true;
  activeSetChangeID = 0;
  b_nVar = workingset.nVar;
  globalActiveConstrIdx = 0;
  Objective::computeGrad_StoreHx(objective, H, f, solution.xstar);
  solution.fstar = Objective::computeFval_ReuseHx(
      objective, memspace.workspace_float, f, solution.xstar);
  solution.state = -5;
  std::memset(&solution.lambda[0], 0, 301U * sizeof(double));
  int exitg1;
  do {
    exitg1 = 0;
    if (solution.state == -5) {
      double minLambda;
      int i;
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
          QRManager::factorQR(qrmanager, workingset.ATwset, b_nVar,
                              workingset.nActiveConstr);
          QRManager::computeQ_(qrmanager, qrmanager.mrows);
          break;
        }
        idxStartIneq = std::memcmp(&options_SolverName[0], &b[0], 7);
        compute_deltax(H, solution, memspace, qrmanager, cholmanager, objective,
                       idxStartIneq == 0);
        if (solution.state != -5) {
          exitg1 = 1;
        } else if ((internal::blas::xnrm2(b_nVar, solution.searchDir) <
                    1.4901161193847657E-10) ||
                   (workingset.nActiveConstr >= b_nVar)) {
          guard1 = true;
        } else {
          minLambda = feasibleratiotest(
              solution.xstar, solution.searchDir, memspace.workspace_float,
              workingset.nVar, workingset.Aineq, workingset.bineq,
              workingset.lb, workingset.ub, workingset.indexLB,
              workingset.indexUB, workingset.sizes, workingset.isActiveIdx,
              workingset.isActiveConstr, workingset.nWConstr, true, updateFval,
              idxStartIneq, idx_local);
          if (updateFval) {
            switch (idxStartIneq) {
            case 3:
              workingset.nWConstr[2]++;
              workingset
                  .isActiveConstr[(workingset.isActiveIdx[2] + idx_local) - 2] =
                  true;
              workingset.nActiveConstr++;
              workingset.Wid[workingset.nActiveConstr - 1] = 3;
              workingset.Wlocalidx[workingset.nActiveConstr - 1] = idx_local;
              idxStartIneq = 151 * (idx_local - 1);
              idxEndIneq = 151 * (workingset.nActiveConstr - 1);
              i = workingset.nVar;
              for (int idx_global{0}; idx_global < i; idx_global++) {
                workingset.ATwset[idxEndIneq + idx_global] =
                    workingset.Aineq[idxStartIneq + idx_global];
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
          if ((b_nVar >= 1) && (!(minLambda == 0.0))) {
            idxStartIneq = (b_nVar / 2) << 1;
            idxEndIneq = idxStartIneq - 2;
            for (int idx_global{0}; idx_global <= idxEndIneq; idx_global += 2) {
              __m128d r;
              __m128d r1;
              r = _mm_loadu_pd(&solution.searchDir[idx_global]);
              r1 = _mm_loadu_pd(&solution.xstar[idx_global]);
              _mm_storeu_pd(
                  &solution.xstar[idx_global],
                  _mm_add_pd(r1, _mm_mul_pd(_mm_set1_pd(minLambda), r)));
            }
            for (int idx_global{idxStartIneq}; idx_global < b_nVar;
                 idx_global++) {
              solution.xstar[idx_global] +=
                  minLambda * solution.searchDir[idx_global];
            }
          }
          Objective::computeGrad_StoreHx(objective, H, f, solution.xstar);
          updateFval = true;
          stopping::checkStoppingAndUpdateFval(
              activeSetChangeID, f, solution, memspace, objective, workingset,
              qrmanager, runTimeOptions.MaxIterations, updateFval);
        }
      } else {
        idxStartIneq = static_cast<unsigned char>(b_nVar);
        std::memset(&solution.searchDir[0], 0,
                    static_cast<unsigned int>(idxStartIneq) * sizeof(double));
        guard1 = true;
      }
      if (guard1) {
        compute_lambda(memspace.workspace_float, solution, objective,
                       qrmanager);
        if ((solution.state != -7) || (workingset.nActiveConstr > b_nVar)) {
          idxStartIneq = 0;
          minLambda = 0.0;
          idxEndIneq = (workingset.nWConstr[0] + workingset.nWConstr[1]) + 1;
          i = workingset.nActiveConstr;
          for (int idx_global{idxEndIneq}; idx_global <= i; idx_global++) {
            double d;
            d = solution.lambda[idx_global - 1];
            if (d < minLambda) {
              minLambda = d;
              idxStartIneq = idx_global;
            }
          }
          if (idxStartIneq == 0) {
            solution.state = 1;
          } else {
            activeSetChangeID = -1;
            globalActiveConstrIdx = idxStartIneq;
            subProblemChanged = true;
            WorkingSet::removeConstr(workingset, idxStartIneq);
            if (idxStartIneq < workingset.nActiveConstr + 1) {
              solution.lambda[idxStartIneq - 1] =
                  solution.lambda[workingset.nActiveConstr];
            }
            solution.lambda[workingset.nActiveConstr] = 0.0;
          }
        } else {
          idxStartIneq = workingset.nActiveConstr;
          activeSetChangeID = 0;
          globalActiveConstrIdx = workingset.nActiveConstr;
          subProblemChanged = true;
          WorkingSet::removeConstr(workingset, workingset.nActiveConstr);
          solution.lambda[idxStartIneq - 1] = 0.0;
        }
        updateFval = false;
        stopping::checkStoppingAndUpdateFval(
            activeSetChangeID, f, solution, memspace, objective, workingset,
            qrmanager, runTimeOptions.MaxIterations, updateFval);
      }
    } else {
      if (!updateFval) {
        solution.fstar = Objective::computeFval_ReuseHx(
            objective, memspace.workspace_float, f, solution.xstar);
      }
      exitg1 = 1;
    }
  } while (exitg1 == 0);
  if (workingset
          .isActiveConstr[(workingset.isActiveIdx[3] + workingset.sizes[3]) -
                          2]) {
    boolean_T exitg2;
    idxStartIneq = 1;
    exitg2 = false;
    while ((!exitg2) && (idxStartIneq <= workingset.nActiveConstr)) {
      if ((workingset.Wid[idxStartIneq - 1] == 4) &&
          (workingset.Wlocalidx[idxStartIneq - 1] == workingset.sizes[3])) {
        WorkingSet::removeConstr(workingset, idxStartIneq);
        exitg2 = true;
      } else {
        idxStartIneq++;
      }
    }
  }
  idxStartIneq = workingset.nActiveConstr;
  while ((idxStartIneq > 0) && (idxStartIneq > nVar)) {
    WorkingSet::removeConstr(workingset, idxStartIneq);
    idxStartIneq--;
  }
  solution.maxConstr = solution.xstar[nVar];
  WorkingSet::setProblemType(workingset, PROBTYPE_ORIG);
  objective.objtype = objective.prev_objtype;
  objective.nvar = objective.prev_nvar;
  objective.hasLinear = objective.prev_hasLinear;
}

} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (phaseone.cpp)

//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// test_exit.cpp
//
// Code generation for function 'test_exit'
//

// Include files
#include "test_exit.h"
#include "computeComplError.h"
#include "computeGradLag.h"
#include "computeQ_.h"
#include "factorQRE.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "sortLambdaQP.h"
#include "updateWorkingSetForNewQP.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
void b_test_exit(j_struct_T &Flags, b_struct_T &memspace,
                 h_struct_T &b_MeritFunction, g_struct_T &WorkingSet,
                 e_struct_T &b_TrialState, c_struct_T &b_QRManager)
{
  double c;
  double nlpComplErrorLSQ;
  double optimRelativeFactor;
  int idx_max;
  int ix;
  int mLB;
  int nVar;
  boolean_T dxTooSmall;
  boolean_T exitg1;
  boolean_T isFeasible;
  nVar = WorkingSet.nVar;
  mLB = WorkingSet.sizes[3];
  idx_max = static_cast<unsigned short>(WorkingSet.sizes[3] + 150);
  if (idx_max - 1 >= 0) {
    std::copy(&b_TrialState.lambdasqp[0], &b_TrialState.lambdasqp[idx_max],
              &b_TrialState.lambdaStopTest[0]);
  }
  stopping::computeGradLag(b_TrialState.gradLag, WorkingSet.nVar,
                           b_TrialState.grad, WorkingSet.Aineq,
                           WorkingSet.indexLB, WorkingSet.sizes[3],
                           WorkingSet.indexUB, b_TrialState.lambdaStopTest);
  if (WorkingSet.nVar < 1) {
    idx_max = 0;
  } else {
    idx_max = 1;
    if (WorkingSet.nVar > 1) {
      c = std::abs(b_TrialState.grad[0]);
      for (int idx{2}; idx <= nVar; idx++) {
        nlpComplErrorLSQ = std::abs(b_TrialState.grad[idx - 1]);
        if (nlpComplErrorLSQ > c) {
          idx_max = idx;
          c = nlpComplErrorLSQ;
        }
      }
    }
  }
  optimRelativeFactor =
      std::fmax(1.0, std::abs(b_TrialState.grad[idx_max - 1]));
  if (std::isinf(optimRelativeFactor)) {
    optimRelativeFactor = 1.0;
  }
  c = 0.0;
  for (int idx{0}; idx < 120; idx++) {
    c = std::fmax(c, b_TrialState.cIneq[idx]);
  }
  idx_max = static_cast<unsigned char>(WorkingSet.sizes[3]);
  for (int idx{0}; idx < idx_max; idx++) {
    c = std::fmax(c, -0.6108652381980153 -
                         b_TrialState.xstarsqp[WorkingSet.indexLB[idx] - 1]);
  }
  for (int idx{0}; idx < 30; idx++) {
    c = std::fmax(c, b_TrialState.xstarsqp[WorkingSet.indexUB[idx] - 1] -
                         0.6108652381980153);
  }
  if (b_TrialState.sqpIterations == 0) {
    b_MeritFunction.feasRelativeFactor = std::fmax(1.0, c);
  }
  isFeasible = (c <= 1.0E-6 * b_MeritFunction.feasRelativeFactor);
  dxTooSmall = true;
  c = 0.0;
  idx_max = static_cast<unsigned char>(WorkingSet.nVar);
  ix = 0;
  exitg1 = false;
  while ((!exitg1) && (ix <= idx_max - 1)) {
    dxTooSmall = ((!std::isinf(b_TrialState.gradLag[ix])) &&
                  (!std::isnan(b_TrialState.gradLag[ix])));
    if (!dxTooSmall) {
      exitg1 = true;
    } else {
      c = std::fmax(c, std::abs(b_TrialState.gradLag[ix]));
      ix++;
    }
  }
  b_MeritFunction.nlpDualFeasError = c;
  if (!dxTooSmall) {
    Flags.done = true;
    if (isFeasible) {
      b_TrialState.sqpExitFlag = 2;
    } else {
      b_TrialState.sqpExitFlag = -2;
    }
  } else {
    double nlpComplErrorTmp;
    boolean_T guard1;
    boolean_T guard2;
    b_MeritFunction.nlpComplError = stopping::computeComplError(
        b_TrialState.xstarsqp, b_TrialState.cIneq, WorkingSet.indexLB,
        WorkingSet.sizes[3], WorkingSet.indexUB, b_TrialState.lambdaStopTest);
    if (b_TrialState.sqpIterations > 1) {
      stopping::b_computeGradLag(
          memspace.workspace_float, WorkingSet.nVar, b_TrialState.grad,
          WorkingSet.Aineq, WorkingSet.indexLB, WorkingSet.sizes[3],
          WorkingSet.indexUB, b_TrialState.lambdaStopTestPrev);
      nlpComplErrorLSQ = 0.0;
      idx_max = static_cast<unsigned char>(WorkingSet.nVar);
      ix = 0;
      while ((ix <= idx_max - 1) &&
             ((!std::isinf(memspace.workspace_float[ix])) &&
              (!std::isnan(memspace.workspace_float[ix])))) {
        nlpComplErrorLSQ =
            std::fmax(nlpComplErrorLSQ, std::abs(memspace.workspace_float[ix]));
        ix++;
      }
      nlpComplErrorTmp = stopping::computeComplError(
          b_TrialState.xstarsqp, b_TrialState.cIneq, WorkingSet.indexLB,
          WorkingSet.sizes[3], WorkingSet.indexUB,
          b_TrialState.lambdaStopTestPrev);
      if ((nlpComplErrorLSQ < c) &&
          (nlpComplErrorTmp < b_MeritFunction.nlpComplError)) {
        b_MeritFunction.nlpDualFeasError = nlpComplErrorLSQ;
        b_MeritFunction.nlpComplError = nlpComplErrorTmp;
        idx_max = static_cast<unsigned short>(WorkingSet.sizes[3] + 150);
        if (idx_max - 1 >= 0) {
          std::copy(&b_TrialState.lambdaStopTestPrev[0],
                    &b_TrialState.lambdaStopTestPrev[idx_max],
                    &b_TrialState.lambdaStopTest[0]);
        }
      } else {
        idx_max = static_cast<unsigned short>(WorkingSet.sizes[3] + 150);
        if (idx_max - 1 >= 0) {
          std::copy(&b_TrialState.lambdaStopTest[0],
                    &b_TrialState.lambdaStopTest[idx_max],
                    &b_TrialState.lambdaStopTestPrev[0]);
        }
      }
    } else {
      idx_max = static_cast<unsigned short>(WorkingSet.sizes[3] + 150);
      if (idx_max - 1 >= 0) {
        std::copy(&b_TrialState.lambdaStopTest[0],
                  &b_TrialState.lambdaStopTest[idx_max],
                  &b_TrialState.lambdaStopTestPrev[0]);
      }
    }
    guard1 = false;
    guard2 = false;
    if (isFeasible) {
      c = 1.0E-6 * optimRelativeFactor;
      if ((b_MeritFunction.nlpDualFeasError <= c) &&
          (b_MeritFunction.nlpComplError <= c)) {
        Flags.done = true;
        b_TrialState.sqpExitFlag = 1;
      } else {
        guard2 = true;
      }
    } else {
      guard2 = true;
    }
    if (guard2) {
      Flags.done = false;
      if (isFeasible && (b_TrialState.sqpFval < -1.0E+20)) {
        Flags.done = true;
        b_TrialState.sqpExitFlag = -3;
      } else if (b_TrialState.sqpIterations > 0) {
        dxTooSmall = true;
        idx_max = static_cast<unsigned char>(WorkingSet.nVar);
        ix = 0;
        exitg1 = false;
        while ((!exitg1) && (ix <= idx_max - 1)) {
          if (1.0E-6 * std::fmax(1.0, std::abs(b_TrialState.xstarsqp[ix])) <=
              std::abs(b_TrialState.delta_x[ix])) {
            dxTooSmall = false;
            exitg1 = true;
          } else {
            ix++;
          }
        }
        if (dxTooSmall) {
          if (!isFeasible) {
            if (Flags.stepType == 2) {
              Flags.done = true;
              b_TrialState.sqpExitFlag = -2;
            } else {
              Flags.stepType = 2;
              Flags.failedLineSearch = false;
              Flags.stepAccepted = false;
              guard1 = true;
            }
          } else {
            int nActiveConstr;
            nActiveConstr = WorkingSet.nActiveConstr;
            if (WorkingSet.nActiveConstr == 0) {
              Flags.done = true;
              b_TrialState.sqpExitFlag = 2;
            } else {
              int fullRank_R;
              int rankR;
              internal::updateWorkingSetForNewQP(b_TrialState.xstarsqp,
                                                 WorkingSet, b_TrialState.cIneq,
                                                 WorkingSet.sizes[3]);
              if (nActiveConstr - 1 >= 0) {
                std::memset(&b_TrialState.lambda[0], 0,
                            static_cast<unsigned int>(nActiveConstr) *
                                sizeof(double));
              }
              QRManager::factorQRE(b_QRManager, WorkingSet.ATwset, nVar,
                                   nActiveConstr);
              QRManager::computeQ_(b_QRManager, b_QRManager.mrows);
              idx_max = static_cast<unsigned char>(nVar);
              if (idx_max - 1 >= 0) {
                std::memset(&memspace.workspace_float[0], 0,
                            static_cast<unsigned int>(idx_max) *
                                sizeof(double));
              }
              ix = 301 * (nVar - 1) + 1;
              for (int iac{1}; iac <= ix; iac += 301) {
                c = 0.0;
                idx_max = iac + nVar;
                for (int idx{iac}; idx < idx_max; idx++) {
                  c += b_QRManager.Q[idx - 1] * b_TrialState.grad[idx - iac];
                }
                idx_max = div_nde_s32_floor(iac - 1, 301);
                memspace.workspace_float[idx_max] -= c;
              }
              if (nVar >= nActiveConstr) {
                idx_max = nVar;
              } else {
                idx_max = nActiveConstr;
              }
              c = std::abs(b_QRManager.QR[0]) *
                  std::fmin(1.4901161193847656E-8,
                            static_cast<double>(idx_max) *
                                2.2204460492503131E-16);
              if (nVar <= nActiveConstr) {
                fullRank_R = nVar;
              } else {
                fullRank_R = nActiveConstr;
              }
              rankR = 0;
              idx_max = 0;
              while ((rankR < fullRank_R) &&
                     (std::abs(b_QRManager.QR[idx_max]) > c)) {
                rankR++;
                idx_max += 302;
              }
              if (rankR != 0) {
                for (int idx{rankR}; idx >= 1; idx--) {
                  idx_max = (idx + (idx - 1) * 301) - 1;
                  memspace.workspace_float[idx - 1] /= b_QRManager.QR[idx_max];
                  for (int iac{0}; iac <= idx - 2; iac++) {
                    ix = (idx - iac) - 2;
                    memspace.workspace_float[ix] -=
                        memspace.workspace_float[idx - 1] *
                        b_QRManager.QR[(idx_max - iac) - 1];
                  }
                }
              }
              if (nActiveConstr <= fullRank_R) {
                fullRank_R = nActiveConstr;
              }
              for (int idx{0}; idx < fullRank_R; idx++) {
                b_TrialState.lambda[b_QRManager.jpvt[idx] - 1] =
                    memspace.workspace_float[idx];
              }
              qpactiveset::parseoutput::sortLambdaQP(
                  b_TrialState.lambda, WorkingSet.nActiveConstr,
                  WorkingSet.sizes, WorkingSet.isActiveIdx, WorkingSet.Wid,
                  WorkingSet.Wlocalidx, memspace.workspace_float);
              stopping::b_computeGradLag(
                  memspace.workspace_float, nVar, b_TrialState.grad,
                  WorkingSet.Aineq, WorkingSet.indexLB, mLB, WorkingSet.indexUB,
                  b_TrialState.lambda);
              c = 0.0;
              idx_max = 0;
              while ((idx_max <= static_cast<unsigned char>(nVar) - 1) &&
                     ((!std::isinf(memspace.workspace_float[idx_max])) &&
                      (!std::isnan(memspace.workspace_float[idx_max])))) {
                c = std::fmax(c, std::abs(memspace.workspace_float[idx_max]));
                idx_max++;
              }
              nlpComplErrorLSQ = stopping::computeComplError(
                  b_TrialState.xstarsqp, b_TrialState.cIneq, WorkingSet.indexLB,
                  mLB, WorkingSet.indexUB, b_TrialState.lambda);
              nlpComplErrorTmp = std::fmax(c, nlpComplErrorLSQ);
              if (nlpComplErrorTmp <=
                  std::fmax(b_MeritFunction.nlpDualFeasError,
                            b_MeritFunction.nlpComplError)) {
                b_MeritFunction.nlpDualFeasError = c;
                b_MeritFunction.nlpComplError = nlpComplErrorLSQ;
                idx_max = static_cast<unsigned short>(mLB + 150);
                if (idx_max - 1 >= 0) {
                  std::copy(&b_TrialState.lambda[0],
                            &b_TrialState.lambda[idx_max],
                            &b_TrialState.lambdaStopTest[0]);
                }
              }
              c = 1.0E-6 * optimRelativeFactor;
              if ((b_MeritFunction.nlpDualFeasError <= c) &&
                  (b_MeritFunction.nlpComplError <= c)) {
                b_TrialState.sqpExitFlag = 1;
              } else {
                b_TrialState.sqpExitFlag = 2;
              }
              Flags.done = true;
              guard1 = true;
            }
          }
        } else {
          guard1 = true;
        }
      } else {
        guard1 = true;
      }
    }
    if (guard1) {
      if (b_TrialState.sqpIterations >= 200) {
        Flags.done = true;
        b_TrialState.sqpExitFlag = 0;
      } else if (b_TrialState.FunctionEvaluations >= 3000) {
        Flags.done = true;
        b_TrialState.sqpExitFlag = 0;
      }
    }
  }
}

boolean_T test_exit(h_struct_T &b_MeritFunction, const g_struct_T &WorkingSet,
                    e_struct_T &b_TrialState, boolean_T &Flags_fevalOK,
                    boolean_T &Flags_done, boolean_T &Flags_stepAccepted,
                    boolean_T &Flags_failedLineSearch, int &Flags_stepType)
{
  double optimRelativeFactor;
  double smax;
  int i;
  int idx_max;
  int nVar;
  boolean_T Flags_gradOK;
  boolean_T exitg1;
  boolean_T isFeasible;
  Flags_fevalOK = true;
  Flags_done = false;
  Flags_stepAccepted = false;
  Flags_failedLineSearch = false;
  Flags_stepType = 1;
  nVar = WorkingSet.nVar;
  i = static_cast<unsigned short>(WorkingSet.sizes[3] + 150);
  if (i - 1 >= 0) {
    std::copy(&b_TrialState.lambdasqp[0], &b_TrialState.lambdasqp[i],
              &b_TrialState.lambdaStopTest[0]);
  }
  stopping::computeGradLag(b_TrialState.gradLag, WorkingSet.nVar,
                           b_TrialState.grad, WorkingSet.Aineq,
                           WorkingSet.indexLB, WorkingSet.sizes[3],
                           WorkingSet.indexUB, b_TrialState.lambdaStopTest);
  if (WorkingSet.nVar < 1) {
    idx_max = 0;
  } else {
    idx_max = 1;
    if (WorkingSet.nVar > 1) {
      smax = std::abs(b_TrialState.grad[0]);
      for (int idx{2}; idx <= nVar; idx++) {
        optimRelativeFactor = std::abs(b_TrialState.grad[idx - 1]);
        if (optimRelativeFactor > smax) {
          idx_max = idx;
          smax = optimRelativeFactor;
        }
      }
    }
  }
  optimRelativeFactor =
      std::fmax(1.0, std::abs(b_TrialState.grad[idx_max - 1]));
  if (std::isinf(optimRelativeFactor)) {
    optimRelativeFactor = 1.0;
  }
  smax = 0.0;
  for (int idx{0}; idx < 120; idx++) {
    smax = std::fmax(smax, b_TrialState.cIneq[idx]);
  }
  nVar = static_cast<unsigned char>(WorkingSet.sizes[3]);
  for (int idx{0}; idx < nVar; idx++) {
    smax =
        std::fmax(smax, -0.6108652381980153 -
                            b_TrialState.xstarsqp[WorkingSet.indexLB[idx] - 1]);
  }
  for (int idx{0}; idx < 30; idx++) {
    smax = std::fmax(smax, b_TrialState.xstarsqp[WorkingSet.indexUB[idx] - 1] -
                               0.6108652381980153);
  }
  b_MeritFunction.feasRelativeFactor = std::fmax(1.0, smax);
  isFeasible = (smax <= 1.0E-6 * b_MeritFunction.feasRelativeFactor);
  Flags_gradOK = true;
  smax = 0.0;
  nVar = static_cast<unsigned char>(WorkingSet.nVar);
  idx_max = 0;
  exitg1 = false;
  while ((!exitg1) && (idx_max <= nVar - 1)) {
    Flags_gradOK = ((!std::isinf(b_TrialState.gradLag[idx_max])) &&
                    (!std::isnan(b_TrialState.gradLag[idx_max])));
    if (!Flags_gradOK) {
      exitg1 = true;
    } else {
      smax = std::fmax(smax, std::abs(b_TrialState.gradLag[idx_max]));
      idx_max++;
    }
  }
  b_MeritFunction.nlpDualFeasError = smax;
  if (!Flags_gradOK) {
    Flags_done = true;
    if (isFeasible) {
      b_TrialState.sqpExitFlag = 2;
    } else {
      b_TrialState.sqpExitFlag = -2;
    }
  } else {
    b_MeritFunction.nlpComplError = 0.0;
    if (i - 1 >= 0) {
      std::copy(&b_TrialState.lambdaStopTest[0],
                &b_TrialState.lambdaStopTest[i],
                &b_TrialState.lambdaStopTestPrev[0]);
    }
    if (isFeasible && (smax <= 1.0E-6 * optimRelativeFactor)) {
      Flags_done = true;
      b_TrialState.sqpExitFlag = 1;
    } else if (isFeasible && (b_TrialState.sqpFval < -1.0E+20)) {
      Flags_done = true;
      b_TrialState.sqpExitFlag = -3;
    } else if (b_TrialState.FunctionEvaluations >= 3000) {
      Flags_done = true;
      b_TrialState.sqpExitFlag = 0;
    }
  }
  return Flags_gradOK;
}

} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (test_exit.cpp)

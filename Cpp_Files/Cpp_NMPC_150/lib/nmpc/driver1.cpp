//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// driver1.cpp
//
// Code generation for function 'driver1'
//

// Include files
#include "driver1.h"
#include "BFGSUpdate.h"
#include "computeFiniteDifferences.h"
#include "computeLinearResiduals.h"
#include "evalObjAndConstr.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "step.h"
#include "stickyStruct.h"
#include "test_exit.h"
#include "updateWorkingSetForNewQP.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
void driver(const double bineq[60], e_struct_T &b_TrialState,
            h_struct_T &b_MeritFunction,
            const ::coder::internal::i_stickyStruct &FcnEvaluator,
            l_struct_T &FiniteDifferences, b_struct_T &memspace,
            g_struct_T &WorkingSet, c_struct_T &b_QRManager,
            i_struct_T &b_CholManager, double Hessian[900],
            struct_T &QPObjective)
{
  static const signed char b_Hessian[900]{
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  static const char qpoptions_SolverName[7]{'f', 'm', 'i', 'n', 'c', 'o', 'n'};
  j_struct_T Flags;
  k_struct_T expl_temp;
  int i;
  int iCol;
  int iCol_old;
  int mLB;
  int nVar;
  int qpoptions_MaxIterations;
  std::memset(&QPObjective.grad[0], 0, 151U * sizeof(double));
  std::memset(&QPObjective.Hx[0], 0, 150U * sizeof(double));
  QPObjective.hasLinear = true;
  QPObjective.nvar = 30;
  QPObjective.maxVar = 151;
  QPObjective.beta = 0.0;
  QPObjective.rho = 0.0;
  QPObjective.objtype = 3;
  QPObjective.prev_objtype = 3;
  QPObjective.prev_nvar = 0;
  QPObjective.prev_hasLinear = false;
  QPObjective.gammaScalar = 0.0;
  for (int idx_col{0}; idx_col < 900; idx_col++) {
    Hessian[idx_col] = b_Hessian[idx_col];
  }
  nVar = WorkingSet.nVar;
  mLB = WorkingSet.sizes[3];
  iCol = WorkingSet.nVar;
  iCol_old = WorkingSet.sizes[3] + 150;
  if (iCol >= iCol_old) {
    iCol_old = iCol;
  }
  qpoptions_MaxIterations = 10 * iCol_old;
  b_TrialState.steplength = 1.0;
  test_exit(b_MeritFunction, WorkingSet, b_TrialState, Flags.fevalOK,
            Flags.done, Flags.stepAccepted, Flags.failedLineSearch,
            Flags.stepType);
  iCol = 9059;
  iCol_old = -1;
  i = static_cast<unsigned char>(nVar);
  for (int idx_col{0}; idx_col < 60; idx_col++) {
    for (int k{0}; k < i; k++) {
      b_TrialState.JacCineqTrans_old[(iCol_old + k) + 1] =
          WorkingSet.Aineq[(iCol + k) + 1];
    }
    iCol += 151;
    iCol_old += 151;
  }
  b_TrialState.sqpFval_old = b_TrialState.sqpFval;
  for (int idx_col{0}; idx_col < 30; idx_col++) {
    b_TrialState.xstarsqp_old[idx_col] = b_TrialState.xstarsqp[idx_col];
    b_TrialState.grad_old[idx_col] = b_TrialState.grad[idx_col];
  }
  std::copy(&b_TrialState.cIneq[0], &b_TrialState.cIneq[120],
            &b_TrialState.cIneq_old[0]);
  if (!Flags.done) {
    b_TrialState.sqpIterations = 1;
  }
  while (!Flags.done) {
    __m128d r;
    __m128d r1;
    double constrViolationIneq;
    int vectorUB;
    while (!(Flags.stepAccepted || Flags.failedLineSearch)) {
      double d;
      if (Flags.stepType != 3) {
        internal::updateWorkingSetForNewQP(b_TrialState.xstarsqp, WorkingSet,
                                           b_TrialState.cIneq, mLB);
      }
      expl_temp.MaxIterations = qpoptions_MaxIterations;
      for (int idx_col{0}; idx_col < 7; idx_col++) {
        expl_temp.SolverName[idx_col] = qpoptions_SolverName[idx_col];
      }
      b_step(Flags, Hessian, b_TrialState, b_MeritFunction, memspace,
             WorkingSet, b_QRManager, b_CholManager, QPObjective, expl_temp);
      if (Flags.stepAccepted) {
        iCol = (static_cast<unsigned char>(nVar) >> 1) << 1;
        iCol_old = iCol - 2;
        for (int idx_col{0}; idx_col <= iCol_old; idx_col += 2) {
          r = _mm_loadu_pd(&b_TrialState.xstarsqp[idx_col]);
          r1 = _mm_loadu_pd(&b_TrialState.delta_x[idx_col]);
          _mm_storeu_pd(&b_TrialState.xstarsqp[idx_col], _mm_add_pd(r, r1));
        }
        for (int idx_col{iCol}; idx_col < i; idx_col++) {
          b_TrialState.xstarsqp[idx_col] += b_TrialState.delta_x[idx_col];
        }
        b_TrialState.sqpFval = utils::ObjNonlinEvaluator::evalObjAndConstr(
            FcnEvaluator, b_TrialState.xstarsqp, b_TrialState.cIneq, iCol);
        Flags.fevalOK = (iCol == 1);
        b_TrialState.FunctionEvaluations++;
        internal::computeLinearResiduals(b_TrialState.xstarsqp, nVar,
                                         b_TrialState.cIneq, WorkingSet.Aineq,
                                         bineq);
        if (Flags.fevalOK) {
          constrViolationIneq = 0.0;
          for (int idx_col{0}; idx_col < 120; idx_col++) {
            d = b_TrialState.cIneq[idx_col];
            if (d > 0.0) {
              constrViolationIneq += d;
            }
          }
          b_MeritFunction.phiFullStep =
              b_TrialState.sqpFval +
              b_MeritFunction.penaltyParam * constrViolationIneq;
        } else {
          b_MeritFunction.phiFullStep = rtInf;
        }
      }
      if ((Flags.stepType == 1) && Flags.stepAccepted && Flags.fevalOK &&
          (b_MeritFunction.phi < b_MeritFunction.phiFullStep) &&
          (b_TrialState.sqpFval < b_TrialState.sqpFval_old)) {
        Flags.stepType = 3;
        Flags.stepAccepted = false;
      } else {
        double alpha;
        int b_nVar;
        int exitflagLnSrch;
        boolean_T evalWellDefined;
        boolean_T socTaken;
        if ((Flags.stepType == 3) && Flags.stepAccepted) {
          socTaken = true;
        } else {
          socTaken = false;
        }
        evalWellDefined = Flags.fevalOK;
        b_nVar = WorkingSet.nVar;
        alpha = 1.0;
        exitflagLnSrch = 1;
        constrViolationIneq = b_MeritFunction.phiFullStep;
        if (b_nVar - 1 >= 0) {
          std::copy(&b_TrialState.delta_x[0], &b_TrialState.delta_x[b_nVar],
                    &b_TrialState.searchDir[0]);
        }
        int exitg1;
        do {
          exitg1 = 0;
          if (b_TrialState.FunctionEvaluations < 3000) {
            if (evalWellDefined &&
                (constrViolationIneq <=
                 b_MeritFunction.phi +
                     alpha * 0.0001 * b_MeritFunction.phiPrimePlus)) {
              exitg1 = 1;
            } else {
              int scalarLB;
              boolean_T exitg2;
              boolean_T tooSmallX;
              alpha *= 0.7;
              vectorUB = static_cast<unsigned char>(b_nVar);
              scalarLB = (static_cast<unsigned char>(b_nVar) >> 1) << 1;
              iCol = scalarLB - 2;
              for (int idx_col{0}; idx_col <= iCol; idx_col += 2) {
                r = _mm_loadu_pd(&b_TrialState.xstar[idx_col]);
                _mm_storeu_pd(&b_TrialState.delta_x[idx_col],
                              _mm_mul_pd(_mm_set1_pd(alpha), r));
              }
              for (int idx_col{scalarLB}; idx_col < vectorUB; idx_col++) {
                b_TrialState.delta_x[idx_col] =
                    alpha * b_TrialState.xstar[idx_col];
              }
              if (socTaken) {
                constrViolationIneq = alpha * alpha;
                if ((b_nVar >= 1) && (!(constrViolationIneq == 0.0))) {
                  iCol = (b_nVar / 2) << 1;
                  iCol_old = iCol - 2;
                  for (int idx_col{0}; idx_col <= iCol_old; idx_col += 2) {
                    r = _mm_loadu_pd(&b_TrialState.socDirection[idx_col]);
                    r1 = _mm_loadu_pd(&b_TrialState.delta_x[idx_col]);
                    _mm_storeu_pd(
                        &b_TrialState.delta_x[idx_col],
                        _mm_add_pd(
                            r1,
                            _mm_mul_pd(_mm_set1_pd(constrViolationIneq), r)));
                  }
                  for (int idx_col{iCol}; idx_col < b_nVar; idx_col++) {
                    b_TrialState.delta_x[idx_col] +=
                        constrViolationIneq *
                        b_TrialState.socDirection[idx_col];
                  }
                }
              }
              tooSmallX = true;
              iCol = 0;
              exitg2 = false;
              while ((!exitg2) &&
                     (iCol <= static_cast<unsigned char>(b_nVar) - 1)) {
                if (1.0E-6 *
                        std::fmax(1.0, std::abs(b_TrialState.xstarsqp[iCol])) <=
                    std::abs(b_TrialState.delta_x[iCol])) {
                  tooSmallX = false;
                  exitg2 = true;
                } else {
                  iCol++;
                }
              }
              if (tooSmallX) {
                exitflagLnSrch = -2;
                exitg1 = 1;
              } else {
                iCol = scalarLB - 2;
                for (int idx_col{0}; idx_col <= iCol; idx_col += 2) {
                  r = _mm_loadu_pd(&b_TrialState.xstarsqp_old[idx_col]);
                  r1 = _mm_loadu_pd(&b_TrialState.delta_x[idx_col]);
                  _mm_storeu_pd(&b_TrialState.xstarsqp[idx_col],
                                _mm_add_pd(r, r1));
                }
                for (int idx_col{scalarLB}; idx_col < vectorUB; idx_col++) {
                  b_TrialState.xstarsqp[idx_col] =
                      b_TrialState.xstarsqp_old[idx_col] +
                      b_TrialState.delta_x[idx_col];
                }
                b_TrialState.sqpFval =
                    utils::ObjNonlinEvaluator::evalObjAndConstr(
                        FcnEvaluator, b_TrialState.xstarsqp, b_TrialState.cIneq,
                        iCol);
                internal::computeLinearResiduals(b_TrialState.xstarsqp, b_nVar,
                                                 b_TrialState.cIneq,
                                                 WorkingSet.Aineq, bineq);
                b_TrialState.FunctionEvaluations++;
                evalWellDefined = (iCol == 1);
                if (evalWellDefined) {
                  constrViolationIneq = 0.0;
                  for (int idx_col{0}; idx_col < 120; idx_col++) {
                    d = b_TrialState.cIneq[idx_col];
                    if (d > 0.0) {
                      constrViolationIneq += d;
                    }
                  }
                  constrViolationIneq =
                      b_TrialState.sqpFval +
                      b_MeritFunction.penaltyParam * constrViolationIneq;
                } else {
                  constrViolationIneq = rtInf;
                }
              }
            }
          } else {
            exitflagLnSrch = 0;
            exitg1 = 1;
          }
        } while (exitg1 == 0);
        Flags.fevalOK = evalWellDefined;
        b_TrialState.steplength = alpha;
        if (exitflagLnSrch > 0) {
          Flags.stepAccepted = true;
        } else {
          Flags.failedLineSearch = true;
        }
      }
    }
    if (Flags.stepAccepted && (!Flags.failedLineSearch)) {
      iCol = (static_cast<unsigned char>(nVar) >> 1) << 1;
      iCol_old = iCol - 2;
      for (int idx_col{0}; idx_col <= iCol_old; idx_col += 2) {
        r = _mm_loadu_pd(&b_TrialState.xstarsqp_old[idx_col]);
        r1 = _mm_loadu_pd(&b_TrialState.delta_x[idx_col]);
        _mm_storeu_pd(&b_TrialState.xstarsqp[idx_col], _mm_add_pd(r, r1));
      }
      for (int idx_col{iCol}; idx_col < i; idx_col++) {
        b_TrialState.xstarsqp[idx_col] =
            b_TrialState.xstarsqp_old[idx_col] + b_TrialState.delta_x[idx_col];
      }
      iCol = static_cast<unsigned short>(mLB + 150);
      iCol_old = (static_cast<unsigned short>(mLB + 150) >> 1) << 1;
      vectorUB = iCol_old - 2;
      for (int idx_col{0}; idx_col <= vectorUB; idx_col += 2) {
        r = _mm_loadu_pd(&b_TrialState.lambda[idx_col]);
        r1 = _mm_loadu_pd(&b_TrialState.lambdasqp[idx_col]);
        _mm_storeu_pd(
            &b_TrialState.lambdasqp[idx_col],
            _mm_add_pd(r1, _mm_mul_pd(_mm_set1_pd(b_TrialState.steplength),
                                      _mm_sub_pd(r, r1))));
      }
      for (int idx_col{iCol_old}; idx_col < iCol; idx_col++) {
        constrViolationIneq = b_TrialState.lambdasqp[idx_col];
        constrViolationIneq +=
            b_TrialState.steplength *
            (b_TrialState.lambda[idx_col] - constrViolationIneq);
        b_TrialState.lambdasqp[idx_col] = constrViolationIneq;
      }
      b_TrialState.sqpFval_old = b_TrialState.sqpFval;
      for (int idx_col{0}; idx_col < 30; idx_col++) {
        b_TrialState.xstarsqp_old[idx_col] = b_TrialState.xstarsqp[idx_col];
        b_TrialState.grad_old[idx_col] = b_TrialState.grad[idx_col];
      }
      std::copy(&b_TrialState.cIneq[0], &b_TrialState.cIneq[120],
                &b_TrialState.cIneq_old[0]);
      utils::FiniteDifferences::computeFiniteDifferences(
          FiniteDifferences, b_TrialState.sqpFval, b_TrialState.cIneq,
          b_TrialState.xstarsqp, b_TrialState.grad, WorkingSet.Aineq);
      b_TrialState.FunctionEvaluations += FiniteDifferences.numEvals;
    } else {
      b_TrialState.sqpFval = b_TrialState.sqpFval_old;
      std::copy(&b_TrialState.xstarsqp_old[0], &b_TrialState.xstarsqp_old[30],
                &b_TrialState.xstarsqp[0]);
      std::copy(&b_TrialState.cIneq_old[0], &b_TrialState.cIneq_old[120],
                &b_TrialState.cIneq[0]);
    }
    b_test_exit(Flags, memspace, b_MeritFunction, WorkingSet, b_TrialState,
                b_QRManager);
    if ((!Flags.done) && Flags.stepAccepted) {
      Flags.stepAccepted = false;
      Flags.stepType = 1;
      Flags.failedLineSearch = false;
      std::copy(&b_TrialState.grad[0], &b_TrialState.grad[i],
                &b_TrialState.delta_gradLag[0]);
      if (nVar >= 1) {
        iCol = (nVar / 2) << 1;
        iCol_old = iCol - 2;
        for (int idx_col{0}; idx_col <= iCol_old; idx_col += 2) {
          r = _mm_loadu_pd(&b_TrialState.delta_gradLag[idx_col]);
          r1 = _mm_loadu_pd(&b_TrialState.grad_old[idx_col]);
          _mm_storeu_pd(&b_TrialState.delta_gradLag[idx_col],
                        _mm_sub_pd(r, r1));
        }
        for (int idx_col{iCol}; idx_col < nVar; idx_col++) {
          b_TrialState.delta_gradLag[idx_col] -= b_TrialState.grad_old[idx_col];
        }
      }
      iCol = 60;
      for (int idx_col{0}; idx_col <= 8909; idx_col += 151) {
        iCol_old = (idx_col + nVar) + 9060;
        for (int k{idx_col + 9061}; k <= iCol_old; k++) {
          vectorUB = (k - idx_col) - 9061;
          b_TrialState.delta_gradLag[vectorUB] +=
              WorkingSet.Aineq[k - 1] * b_TrialState.lambdasqp[iCol];
        }
        iCol++;
      }
      iCol = 60;
      for (int idx_col{0}; idx_col <= 8909; idx_col += 151) {
        iCol_old = idx_col + nVar;
        for (int k{idx_col + 1}; k <= iCol_old; k++) {
          vectorUB = (k - idx_col) - 1;
          b_TrialState.delta_gradLag[vectorUB] +=
              b_TrialState.JacCineqTrans_old[k - 1] *
              -b_TrialState.lambdasqp[iCol];
        }
        iCol++;
      }
      iCol = 9059;
      iCol_old = -1;
      for (int idx_col{0}; idx_col < 60; idx_col++) {
        for (int k{0}; k < i; k++) {
          b_TrialState.JacCineqTrans_old[(iCol_old + k) + 1] =
              WorkingSet.Aineq[(iCol + k) + 1];
        }
        iCol += 151;
        iCol_old += 151;
      }
      BFGSUpdate(nVar, Hessian, b_TrialState.delta_x,
                 b_TrialState.delta_gradLag, memspace.workspace_float);
      b_TrialState.sqpIterations++;
    }
  }
}

} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (driver1.cpp)

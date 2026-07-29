//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// normal.cpp
//
// Code generation for function 'normal'
//

// Include files
#include "normal.h"
#include "driver.h"
#include "maxConstraintViolation.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "sortLambdaQP.h"
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace step {
void normal(const double Hessian[900], const double grad[151],
            e_struct_T &b_TrialState, h_struct_T &b_MeritFunction,
            b_struct_T &memspace, g_struct_T &WorkingSet,
            c_struct_T &b_QRManager, i_struct_T &b_CholManager,
            struct_T &QPObjective, const k_struct_T &qpoptions,
            j_struct_T &stepFlags)
{
  ::coder::optim::coder::qpactiveset::driver(
      Hessian, grad, b_TrialState, memspace, WorkingSet, b_QRManager,
      b_CholManager, QPObjective, qpoptions, qpoptions);
  if ((b_TrialState.state > 0) ||
      ((b_TrialState.state == 0) &&
       (qpactiveset::WorkingSet::maxConstraintViolation(
            WorkingSet.nVar, WorkingSet.Aineq, WorkingSet.bineq, WorkingSet.lb,
            WorkingSet.ub, WorkingSet.indexLB, WorkingSet.indexUB,
            WorkingSet.maxConstrWorkspace, WorkingSet.sizes,
            WorkingSet.probType, b_TrialState.xstar) <= 1.0E-6))) {
    double constrViolDelta;
    double constrViolationIneq;
    double linearizedConstrViolPrev;
    double penaltyParamTrial;
    penaltyParamTrial = b_MeritFunction.penaltyParam;
    constrViolationIneq = 0.0;
    for (int idx{0}; idx < 120; idx++) {
      linearizedConstrViolPrev = b_TrialState.cIneq[idx];
      if (linearizedConstrViolPrev > 0.0) {
        constrViolationIneq += linearizedConstrViolPrev;
      }
    }
    linearizedConstrViolPrev = b_MeritFunction.linearizedConstrViol;
    b_MeritFunction.linearizedConstrViol = 0.0;
    constrViolDelta = constrViolationIneq + linearizedConstrViolPrev;
    if ((constrViolDelta > 2.2204460492503131E-16) &&
        (b_TrialState.fstar > 0.0)) {
      if (b_TrialState.sqpFval == 0.0) {
        linearizedConstrViolPrev = 1.0;
      } else {
        linearizedConstrViolPrev = 1.5;
      }
      penaltyParamTrial =
          linearizedConstrViolPrev * b_TrialState.fstar / constrViolDelta;
    }
    if (penaltyParamTrial < b_MeritFunction.penaltyParam) {
      b_MeritFunction.phi =
          b_TrialState.sqpFval + penaltyParamTrial * constrViolationIneq;
      if ((b_MeritFunction.initFval +
           penaltyParamTrial * b_MeritFunction.initConstrViolationIneq) -
              b_MeritFunction.phi >
          static_cast<double>(b_MeritFunction.nPenaltyDecreases) *
              b_MeritFunction.threshold) {
        b_MeritFunction.nPenaltyDecreases++;
        if ((b_MeritFunction.nPenaltyDecreases << 1) >
            b_TrialState.sqpIterations) {
          b_MeritFunction.threshold *= 10.0;
        }
        b_MeritFunction.penaltyParam = std::fmax(penaltyParamTrial, 1.0E-10);
      } else {
        b_MeritFunction.phi =
            b_TrialState.sqpFval +
            b_MeritFunction.penaltyParam * constrViolationIneq;
      }
    } else {
      b_MeritFunction.penaltyParam = std::fmax(penaltyParamTrial, 1.0E-10);
      b_MeritFunction.phi = b_TrialState.sqpFval +
                            b_MeritFunction.penaltyParam * constrViolationIneq;
    }
    b_MeritFunction.phiPrimePlus = std::fmin(
        b_TrialState.fstar - b_MeritFunction.penaltyParam * constrViolationIneq,
        0.0);
  } else if (b_TrialState.state != -6) {
    stepFlags.stepType = 2;
  }
  qpactiveset::parseoutput::sortLambdaQP(
      b_TrialState.lambda, WorkingSet.nActiveConstr, WorkingSet.sizes,
      WorkingSet.isActiveIdx, WorkingSet.Wid, WorkingSet.Wlocalidx,
      memspace.workspace_float);
}

} // namespace step
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (normal.cpp)

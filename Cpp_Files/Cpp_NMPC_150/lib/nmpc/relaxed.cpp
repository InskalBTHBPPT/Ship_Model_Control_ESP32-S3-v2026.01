//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// relaxed.cpp
//
// Code generation for function 'relaxed'
//

// Include files
#include "relaxed.h"
#include "driver.h"
#include "removeConstr.h"
#include "rt_nonfinite.h"
#include "setProblemType.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "sortLambdaQP.h"
#include "omp.h"
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace step {
void b_relaxed(const double Hessian[900], const double grad[151],
               e_struct_T &b_TrialState, h_struct_T &b_MeritFunction,
               b_struct_T &memspace, g_struct_T &WorkingSet,
               c_struct_T &b_QRManager, i_struct_T &b_CholManager,
               struct_T &QPObjective, k_struct_T &qpoptions)
{
  double beta;
  double c;
  double qpfvalQuadExcess;
  double rho;
  int idx_max;
  int nActiveLBArtificial;
  int nVarOrig;
  int temp;
  nVarOrig = WorkingSet.nVar;
  beta = 0.0;
  temp = static_cast<unsigned char>(WorkingSet.nVar);
  for (int idx{0}; idx < temp; idx++) {
    beta += Hessian[idx + 30 * idx];
  }
  beta /= static_cast<double>(WorkingSet.nVar);
  if (b_TrialState.sqpIterations <= 1) {
    temp = QPObjective.nvar;
    if (QPObjective.nvar < 1) {
      idx_max = 0;
    } else {
      idx_max = 1;
      if (QPObjective.nvar > 1) {
        c = std::abs(grad[0]);
        for (int idx{2}; idx <= temp; idx++) {
          qpfvalQuadExcess = std::abs(grad[idx - 1]);
          if (qpfvalQuadExcess > c) {
            idx_max = idx;
            c = qpfvalQuadExcess;
          }
        }
      }
    }
    rho = 100.0 * std::fmax(1.0, std::abs(grad[idx_max - 1]));
  } else {
    temp = WorkingSet.mConstr;
    idx_max = 1;
    c = std::abs(b_TrialState.lambdasqp[0]);
    for (int idx{2}; idx <= temp; idx++) {
      qpfvalQuadExcess = std::abs(b_TrialState.lambdasqp[idx - 1]);
      if (qpfvalQuadExcess > c) {
        idx_max = idx;
        c = qpfvalQuadExcess;
      }
    }
    rho = std::abs(b_TrialState.lambdasqp[idx_max - 1]);
  }
  QPObjective.nvar = WorkingSet.nVar;
  QPObjective.beta = beta;
  QPObjective.rho = rho;
  QPObjective.hasLinear = true;
  QPObjective.objtype = 4;
  qpactiveset::WorkingSet::setProblemType(WorkingSet, 2);
  for (int idx{0}; idx <= 118; idx += 2) {
    __m128d r;
    r = _mm_loadu_pd(&WorkingSet.bineq[idx]);
    r = _mm_mul_pd(r, _mm_set1_pd(-1.0));
    _mm_storeu_pd(&memspace.workspace_float[idx], r);
  }
  for (int iac{0}; iac <= 17969; iac += 151) {
    c = 0.0;
    temp = iac + nVarOrig;
    for (int idx{iac + 1}; idx <= temp; idx++) {
      c += WorkingSet.Aineq[idx - 1] * b_TrialState.xstar[(idx - iac) - 1];
    }
    temp = div_nde_s32_floor(iac, 151);
    memspace.workspace_float[temp] += c;
  }
  for (int idx{0}; idx < 120; idx++) {
    c = memspace.workspace_float[idx];
    b_TrialState.xstar[nVarOrig + idx] = static_cast<double>(c > 0.0) * c;
  }
  temp = qpoptions.MaxIterations;
  qpoptions.MaxIterations =
      (qpoptions.MaxIterations + WorkingSet.nVar) - nVarOrig;
  ::coder::optim::coder::qpactiveset::driver(
      Hessian, grad, b_TrialState, memspace, WorkingSet, b_QRManager,
      b_CholManager, QPObjective, qpoptions, qpoptions);
  qpoptions.MaxIterations = temp;
  nActiveLBArtificial = 0;
  for (int idx{0}; idx < 120; idx++) {
    boolean_T tf;
    tf = WorkingSet.isActiveConstr
             [((WorkingSet.isActiveIdx[3] + WorkingSet.sizes[3]) + idx) - 121];
    memspace.workspace_int[idx] = tf;
    nActiveLBArtificial += tf;
  }
  if (b_TrialState.state != -6) {
    double penaltyParamTrial;
    temp = nVarOrig + 1;
    c = 0.0;
    if (150 - nVarOrig >= 1) {
      for (int idx{temp}; idx < 151; idx++) {
        c += std::abs(b_TrialState.xstar[idx - 1]);
      }
    }
    qpfvalQuadExcess = 0.0;
    if (150 - nVarOrig >= 1) {
      temp = static_cast<unsigned char>(150 - nVarOrig);
      for (int idx{0}; idx < temp; idx++) {
        penaltyParamTrial = b_TrialState.xstar[nVarOrig + idx];
        qpfvalQuadExcess += penaltyParamTrial * penaltyParamTrial;
      }
    }
    rho = (b_TrialState.fstar - rho * c) - beta / 2.0 * qpfvalQuadExcess;
    temp = nVarOrig + 1;
    penaltyParamTrial = b_MeritFunction.penaltyParam;
    beta = 0.0;
    for (int idx{0}; idx < 120; idx++) {
      c = b_TrialState.cIneq[idx];
      if (c > 0.0) {
        beta += c;
      }
    }
    c = b_MeritFunction.linearizedConstrViol;
    qpfvalQuadExcess = 0.0;
    if (150 - nVarOrig >= 1) {
      for (int idx{temp}; idx < 151; idx++) {
        qpfvalQuadExcess += std::abs(b_TrialState.xstar[idx - 1]);
      }
    }
    b_MeritFunction.linearizedConstrViol = qpfvalQuadExcess;
    qpfvalQuadExcess = (beta + c) - qpfvalQuadExcess;
    if ((qpfvalQuadExcess > 2.2204460492503131E-16) && (rho > 0.0)) {
      if (b_TrialState.sqpFval == 0.0) {
        c = 1.0;
      } else {
        c = 1.5;
      }
      penaltyParamTrial = c * rho / qpfvalQuadExcess;
    }
    if (penaltyParamTrial < b_MeritFunction.penaltyParam) {
      b_MeritFunction.phi = b_TrialState.sqpFval + penaltyParamTrial * beta;
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
            b_TrialState.sqpFval + b_MeritFunction.penaltyParam * beta;
      }
    } else {
      b_MeritFunction.penaltyParam = std::fmax(penaltyParamTrial, 1.0E-10);
      b_MeritFunction.phi =
          b_TrialState.sqpFval + b_MeritFunction.penaltyParam * beta;
    }
    b_MeritFunction.phiPrimePlus =
        std::fmin(rho - b_MeritFunction.penaltyParam * beta, 0.0);
    temp = WorkingSet.isActiveIdx[2];
    idx_max = WorkingSet.nActiveConstr;
    if (static_cast<int>((idx_max - temp) + 1 < 1600)) {
      for (int b_idx{temp}; b_idx <= idx_max; b_idx++) {
        if (WorkingSet.Wid[b_idx - 1] == 3) {
          b_TrialState.lambda[b_idx - 1] *= static_cast<double>(
              memspace.workspace_int[WorkingSet.Wlocalidx[b_idx - 1] - 1]);
        }
      }
    } else {
#pragma omp parallel for num_threads(omp_get_max_threads())

      for (int b_idx = temp; b_idx <= idx_max; b_idx++) {
        if (WorkingSet.Wid[b_idx - 1] == 3) {
          b_TrialState.lambda[b_idx - 1] *= static_cast<double>(
              memspace.workspace_int[WorkingSet.Wlocalidx[b_idx - 1] - 1]);
        }
      }
    }
  }
  temp = WorkingSet.sizes[3] - 120;
  idx_max = WorkingSet.nActiveConstr;
  while ((idx_max > 0) && (nActiveLBArtificial > 0)) {
    if ((WorkingSet.Wid[idx_max - 1] == 4) &&
        (WorkingSet.Wlocalidx[idx_max - 1] > temp)) {
      c = b_TrialState.lambda[WorkingSet.nActiveConstr - 1];
      b_TrialState.lambda[WorkingSet.nActiveConstr - 1] = 0.0;
      b_TrialState.lambda[idx_max - 1] = c;
      qpactiveset::WorkingSet::removeConstr(WorkingSet, idx_max);
      nActiveLBArtificial--;
    }
    idx_max--;
  }
  QPObjective.nvar = nVarOrig;
  QPObjective.hasLinear = true;
  QPObjective.objtype = 3;
  qpactiveset::WorkingSet::setProblemType(WorkingSet, 3);
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

// End of code generation (relaxed.cpp)

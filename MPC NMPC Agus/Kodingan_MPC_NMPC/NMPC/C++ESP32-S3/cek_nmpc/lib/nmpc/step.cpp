//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// step.cpp
//
// Code generation for function 'step'
//

// Include files
#include "step.h"
#include "addBoundToActiveSetMatrix_.h"
#include "driver.h"
#include "normal.h"
#include "relaxed.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "sortLambdaQP.h"
#include "xnrm2.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
void b_step(j_struct_T &stepFlags, double Hessian[900],
            e_struct_T &b_TrialState, h_struct_T &b_MeritFunction,
            b_struct_T &memspace, g_struct_T &WorkingSet,
            c_struct_T &b_QRManager, i_struct_T &b_CholManager,
            struct_T &QPObjective, k_struct_T &qpoptions)
{
  double dv[151];
  double violationResid;
  int idxStartIneq;
  int nVar;
  boolean_T checkBoundViolation;
  stepFlags.stepAccepted = true;
  checkBoundViolation = true;
  nVar = WorkingSet.nVar - 1;
  if (stepFlags.stepType != 3) {
    idxStartIneq = static_cast<unsigned char>(WorkingSet.nVar);
    if (idxStartIneq - 1 >= 0) {
      std::copy(&b_TrialState.xstarsqp[0], &b_TrialState.xstarsqp[idxStartIneq],
                &b_TrialState.xstar[0]);
    }
  } else if (nVar >= 0) {
    std::copy(&b_TrialState.xstar[0], &b_TrialState.xstar[nVar + 1],
              &b_TrialState.searchDir[0]);
  }
  int exitg1;
  boolean_T guard1;
  do {
    double nrmDirInf;
    int idxEndIneq;
    exitg1 = 0;
    guard1 = false;
    switch (stepFlags.stepType) {
    case 1:
      step::normal(Hessian, b_TrialState.grad, b_TrialState, b_MeritFunction,
                   memspace, WorkingSet, b_QRManager, b_CholManager,
                   QPObjective, qpoptions, stepFlags);
      if (stepFlags.stepType != 2) {
        if (nVar >= 0) {
          std::copy(&b_TrialState.xstar[0], &b_TrialState.xstar[nVar + 1],
                    &b_TrialState.delta_x[0]);
        }
        guard1 = true;
      }
      break;
    case 2:
      idxStartIneq = (WorkingSet.nWConstr[0] + WorkingSet.nWConstr[1]) + 1;
      idxEndIneq = WorkingSet.nActiveConstr;
      for (int idx_global{idxStartIneq}; idx_global <= idxEndIneq;
           idx_global++) {
        WorkingSet.isActiveConstr
            [(WorkingSet.isActiveIdx[WorkingSet.Wid[idx_global - 1] - 1] +
              WorkingSet.Wlocalidx[idx_global - 1]) -
             2] = false;
      }
      WorkingSet.nWConstr[2] = 0;
      WorkingSet.nWConstr[3] = 0;
      WorkingSet.nWConstr[4] = 0;
      WorkingSet.nActiveConstr =
          WorkingSet.nWConstr[0] + WorkingSet.nWConstr[1];
      std::copy(&b_TrialState.xstar[0], &b_TrialState.xstar[151], &dv[0]);
      idxStartIneq = static_cast<unsigned char>(WorkingSet.sizes[3]);
      for (int idx_global{0}; idx_global < idxStartIneq; idx_global++) {
        if (-dv[WorkingSet.indexLB[idx_global] - 1] >
            WorkingSet.lb[WorkingSet.indexLB[idx_global] - 1]) {
          dv[WorkingSet.indexLB[idx_global] - 1] =
              (WorkingSet.ub[WorkingSet.indexLB[idx_global] - 1] -
               WorkingSet.lb[WorkingSet.indexLB[idx_global] - 1]) /
              2.0;
        }
      }
      for (int idx_global{0}; idx_global < 30; idx_global++) {
        if (dv[WorkingSet.indexUB[idx_global] - 1] >
            WorkingSet.ub[WorkingSet.indexUB[idx_global] - 1]) {
          dv[WorkingSet.indexUB[idx_global] - 1] =
              (WorkingSet.ub[WorkingSet.indexUB[idx_global] - 1] -
               WorkingSet.lb[WorkingSet.indexUB[idx_global] - 1]) /
              2.0;
        }
      }
      std::copy(&dv[0], &dv[151], &b_TrialState.xstar[0]);
      step::b_relaxed(Hessian, b_TrialState.grad, b_TrialState, b_MeritFunction,
                      memspace, WorkingSet, b_QRManager, b_CholManager,
                      QPObjective, qpoptions);
      if (nVar >= 0) {
        std::copy(&b_TrialState.xstar[0], &b_TrialState.xstar[nVar + 1],
                  &b_TrialState.delta_x[0]);
      }
      guard1 = true;
      break;
    default: {
      __m128d r;
      __m128d r1;
      int b_nVar;
      int i;
      int idxIneqOffset;
      int idx_Partition;
      int idx_upper;
      int nWIneq_old;
      int nWLower_old;
      int nWUpper_old;
      nWIneq_old = WorkingSet.nWConstr[2];
      nWLower_old = WorkingSet.nWConstr[3];
      nWUpper_old = WorkingSet.nWConstr[4];
      b_nVar = WorkingSet.nVar;
      idxStartIneq = static_cast<unsigned char>(WorkingSet.nVar);
      std::copy(&b_TrialState.xstarsqp_old[0],
                &b_TrialState.xstarsqp_old[idxStartIneq],
                &b_TrialState.xstarsqp[0]);
      idxStartIneq = static_cast<unsigned char>(WorkingSet.nVar);
      std::copy(&b_TrialState.xstar[0], &b_TrialState.xstar[idxStartIneq],
                &b_TrialState.socDirection[0]);
      std::copy(&b_TrialState.lambda[0], &b_TrialState.lambda[301],
                &b_TrialState.lambdaStopTest[0]);
      idxIneqOffset = WorkingSet.isActiveIdx[2];
      for (int idx_global{0}; idx_global <= 118; idx_global += 2) {
        r = _mm_loadu_pd(&b_TrialState.cIneq[idx_global]);
        _mm_storeu_pd(&WorkingSet.bineq[idx_global],
                      _mm_mul_pd(r, _mm_set1_pd(-1.0)));
      }
      for (int idx_col{0}; idx_col <= 17969; idx_col += 151) {
        violationResid = 0.0;
        idxStartIneq = idx_col + WorkingSet.nVar;
        for (int idx_global{idx_col + 1}; idx_global <= idxStartIneq;
             idx_global++) {
          violationResid += WorkingSet.Aineq[idx_global - 1] *
                            b_TrialState.searchDir[(idx_global - idx_col) - 1];
        }
        idxStartIneq = div_nde_s32_floor(idx_col, 151);
        WorkingSet.bineq[idxStartIneq] += violationResid;
      }
      idxStartIneq = 1;
      idxEndIneq = 121;
      idx_upper = WorkingSet.sizes[3] + 121;
      i = WorkingSet.nActiveConstr;
      for (int idx_global{idxIneqOffset}; idx_global <= i; idx_global++) {
        switch (WorkingSet.Wid[idx_global - 1]) {
        case 3:
          idx_Partition = idxStartIneq;
          idxStartIneq++;
          WorkingSet.bwset[idx_global - 1] =
              WorkingSet.bineq[WorkingSet.Wlocalidx[idx_global - 1] - 1];
          break;
        case 4:
          idx_Partition = idxEndIneq;
          idxEndIneq++;
          break;
        default:
          idx_Partition = idx_upper;
          idx_upper++;
          break;
        }
        b_TrialState.workingset_old[idx_Partition - 1] =
            WorkingSet.Wlocalidx[idx_global - 1];
      }
      idxStartIneq = static_cast<unsigned char>(WorkingSet.nVar);
      std::copy(&b_TrialState.xstarsqp[0], &b_TrialState.xstarsqp[idxStartIneq],
                &b_TrialState.xstar[0]);
      std::copy(&b_TrialState.grad[0], &b_TrialState.grad[151], &dv[0]);
      ::coder::optim::coder::qpactiveset::driver(
          Hessian, dv, b_TrialState, memspace, WorkingSet, b_QRManager,
          b_CholManager, QPObjective, qpoptions, qpoptions);
      idxStartIneq = static_cast<unsigned char>(b_nVar);
      idxEndIneq = (static_cast<unsigned char>(b_nVar) >> 1) << 1;
      idx_upper = idxEndIneq - 2;
      for (int idx_global{0}; idx_global <= idx_upper; idx_global += 2) {
        r = _mm_loadu_pd(&b_TrialState.socDirection[idx_global]);
        r1 = _mm_loadu_pd(&b_TrialState.xstar[idx_global]);
        _mm_storeu_pd(&b_TrialState.socDirection[idx_global],
                      _mm_sub_pd(r1, r));
        _mm_storeu_pd(&b_TrialState.xstar[idx_global], r);
      }
      for (int idx_global{idxEndIneq}; idx_global < idxStartIneq;
           idx_global++) {
        violationResid = b_TrialState.socDirection[idx_global];
        nrmDirInf = violationResid;
        violationResid = b_TrialState.xstar[idx_global] - violationResid;
        b_TrialState.socDirection[idx_global] = violationResid;
        b_TrialState.xstar[idx_global] = nrmDirInf;
      }
      checkBoundViolation =
          (::coder::internal::blas::xnrm2(b_nVar, b_TrialState.socDirection) <=
           2.0 * ::coder::internal::blas::xnrm2(b_nVar, b_TrialState.xstar));
      idx_Partition = WorkingSet.sizes[3];
      for (int idx_global{0}; idx_global <= 118; idx_global += 2) {
        r = _mm_loadu_pd(&b_TrialState.cIneq[idx_global]);
        _mm_storeu_pd(&WorkingSet.bineq[idx_global],
                      _mm_mul_pd(r, _mm_set1_pd(-1.0)));
      }
      if (!checkBoundViolation) {
        idxStartIneq = (WorkingSet.nWConstr[0] + WorkingSet.nWConstr[1]) + 1;
        idxEndIneq = WorkingSet.nActiveConstr;
        for (int idx_global{idxStartIneq}; idx_global <= idxEndIneq;
             idx_global++) {
          WorkingSet.isActiveConstr
              [(WorkingSet.isActiveIdx[WorkingSet.Wid[idx_global - 1] - 1] +
                WorkingSet.Wlocalidx[idx_global - 1]) -
               2] = false;
        }
        WorkingSet.nWConstr[2] = 0;
        WorkingSet.nWConstr[3] = 0;
        WorkingSet.nWConstr[4] = 0;
        WorkingSet.nActiveConstr =
            WorkingSet.nWConstr[0] + WorkingSet.nWConstr[1];
        for (int idx_global{0}; idx_global < nWIneq_old; idx_global++) {
          idxStartIneq = b_TrialState.workingset_old[idx_global];
          WorkingSet.nWConstr[2]++;
          WorkingSet
              .isActiveConstr[(WorkingSet.isActiveIdx[2] + idxStartIneq) - 2] =
              true;
          WorkingSet.nActiveConstr++;
          WorkingSet.Wid[WorkingSet.nActiveConstr - 1] = 3;
          WorkingSet.Wlocalidx[WorkingSet.nActiveConstr - 1] = idxStartIneq;
          idxEndIneq = 151 * (idxStartIneq - 1);
          idx_upper = 151 * (WorkingSet.nActiveConstr - 1);
          i = WorkingSet.nVar;
          for (int idx_col{0}; idx_col < i; idx_col++) {
            WorkingSet.ATwset[idx_upper + idx_col] =
                WorkingSet.Aineq[idxEndIneq + idx_col];
          }
          WorkingSet.bwset[WorkingSet.nActiveConstr - 1] =
              WorkingSet.bineq[idxStartIneq - 1];
        }
        for (int idx_global{0}; idx_global < nWLower_old; idx_global++) {
          qpactiveset::WorkingSet::addBoundToActiveSetMatrix_(
              WorkingSet, 4, b_TrialState.workingset_old[idx_global + 120]);
        }
        for (int idx_global{0}; idx_global < nWUpper_old; idx_global++) {
          qpactiveset::WorkingSet::addBoundToActiveSetMatrix_(
              WorkingSet, 5,
              b_TrialState.workingset_old[(idx_global + idx_Partition) + 120]);
        }
        std::copy(&b_TrialState.lambdaStopTest[0],
                  &b_TrialState.lambdaStopTest[301], &b_TrialState.lambda[0]);
      } else {
        qpactiveset::parseoutput::sortLambdaQP(
            b_TrialState.lambda, WorkingSet.nActiveConstr, WorkingSet.sizes,
            WorkingSet.isActiveIdx, WorkingSet.Wid, WorkingSet.Wlocalidx,
            memspace.workspace_float);
      }
      stepFlags.stepAccepted = checkBoundViolation;
      if (checkBoundViolation && (b_TrialState.state != -6)) {
        idxStartIneq = static_cast<unsigned char>(nVar + 1);
        idxEndIneq = (static_cast<unsigned char>(nVar + 1) >> 1) << 1;
        idx_upper = idxEndIneq - 2;
        for (int idx_global{0}; idx_global <= idx_upper; idx_global += 2) {
          r = _mm_loadu_pd(&b_TrialState.xstar[idx_global]);
          r1 = _mm_loadu_pd(&b_TrialState.socDirection[idx_global]);
          _mm_storeu_pd(&b_TrialState.delta_x[idx_global], _mm_add_pd(r, r1));
        }
        for (int idx_global{idxEndIneq}; idx_global < idxStartIneq;
             idx_global++) {
          b_TrialState.delta_x[idx_global] =
              b_TrialState.xstar[idx_global] +
              b_TrialState.socDirection[idx_global];
        }
      }
      guard1 = true;
    } break;
    }
    if (guard1) {
      if (b_TrialState.state != -6) {
        exitg1 = 1;
      } else {
        violationResid = 0.0;
        nrmDirInf = 1.0;
        for (int idx_global{0}; idx_global < 30; idx_global++) {
          violationResid = std::fmax(violationResid,
                                     std::abs(b_TrialState.grad[idx_global]));
          nrmDirInf =
              std::fmax(nrmDirInf, std::abs(b_TrialState.xstar[idx_global]));
        }
        violationResid =
            std::fmax(2.2204460492503131E-16, violationResid / nrmDirInf);
        for (int idx_col{0}; idx_col < 30; idx_col++) {
          idxStartIneq = 30 * idx_col;
          for (int idx_global{0}; idx_global < idx_col; idx_global++) {
            Hessian[idxStartIneq + idx_global] = 0.0;
          }
          idxStartIneq = idx_col + 30 * idx_col;
          Hessian[idxStartIneq] = violationResid;
          idxEndIneq = 28 - idx_col;
          if (idxEndIneq >= 0) {
            std::memset(&Hessian[idxStartIneq + 1], 0,
                        static_cast<unsigned int>(idxEndIneq + 1) *
                            sizeof(double));
          }
        }
      }
    }
  } while (exitg1 == 0);
  if (checkBoundViolation) {
    std::copy(&b_TrialState.delta_x[0], &b_TrialState.delta_x[151], &dv[0]);
    idxStartIneq = static_cast<unsigned char>(WorkingSet.sizes[3]);
    for (int idx_global{0}; idx_global < idxStartIneq; idx_global++) {
      violationResid =
          (b_TrialState.xstarsqp[WorkingSet.indexLB[idx_global] - 1] +
           dv[WorkingSet.indexLB[idx_global] - 1]) -
          -0.6108652381980153;
      if (violationResid < 0.0) {
        dv[WorkingSet.indexLB[idx_global] - 1] -= violationResid;
        b_TrialState.xstar[WorkingSet.indexLB[idx_global] - 1] -=
            violationResid;
      }
    }
    for (int idx_global{0}; idx_global < 30; idx_global++) {
      violationResid =
          (0.6108652381980153 -
           b_TrialState.xstarsqp[WorkingSet.indexUB[idx_global] - 1]) -
          dv[WorkingSet.indexUB[idx_global] - 1];
      if (violationResid < 0.0) {
        dv[WorkingSet.indexUB[idx_global] - 1] += violationResid;
        b_TrialState.xstar[WorkingSet.indexUB[idx_global] - 1] +=
            violationResid;
      }
    }
    std::copy(&dv[0], &dv[151], &b_TrialState.delta_x[0]);
  }
}

} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (step.cpp)

//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// fmincon.cpp
//
// Code generation for function 'fmincon'
//

// Include files
#include "fmincon.h"
#include "anonymous_function.h"
#include "computeConstraints_.h"
#include "computeFiniteDifferences.h"
#include "driver1.h"
#include "loadProblem.h"
#include "rt_nonfinite.h"
#include "setProblemType.h"
#include "simulate_nmpc_kapal.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_internal_types1.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "stickyStruct.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>
#include "esp_heap_caps.h"

// Function Definitions
namespace coder {
double fmincon(const b_anonymous_function &fun, const double x0[30],
               const double Aineq[1800], const double bineq[60],
               const anonymous_function nonlcon, double x[30], double &exitflag)
{
  static b_struct_T *memspace_ptr = nullptr;
  static c_struct_T *obj_ptr = nullptr;
  static g_struct_T *WorkingSet_ptr = nullptr;
  static i_struct_T *b_obj_ptr = nullptr;

  if (!memspace_ptr) {
    memspace_ptr = (b_struct_T *)heap_caps_malloc(sizeof(b_struct_T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!memspace_ptr) memspace_ptr = (b_struct_T *)malloc(sizeof(b_struct_T));
  }
  if (!obj_ptr) {
    obj_ptr = (c_struct_T *)heap_caps_malloc(sizeof(c_struct_T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!obj_ptr) obj_ptr = (c_struct_T *)malloc(sizeof(c_struct_T));
  }
  if (!WorkingSet_ptr) {
    WorkingSet_ptr = (g_struct_T *)heap_caps_malloc(sizeof(g_struct_T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!WorkingSet_ptr) WorkingSet_ptr = (g_struct_T *)malloc(sizeof(g_struct_T));
  }
  if (!b_obj_ptr) {
    b_obj_ptr = (i_struct_T *)heap_caps_malloc(sizeof(i_struct_T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!b_obj_ptr) b_obj_ptr = (i_struct_T *)malloc(sizeof(i_struct_T));
  }

  b_struct_T &memspace = *memspace_ptr;
  c_struct_T &obj = *obj_ptr;
  g_struct_T &WorkingSet = *WorkingSet_ptr;
  i_struct_T &b_obj = *b_obj_ptr;

  static const signed char b_iv[151]{
      1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
      18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  internal::i_stickyStruct FcnEvaluator;
  e_struct_T TrialState;
  h_struct_T MeritFunction;
  l_struct_T FiniteDifferences;
  struct_T QPObjective;
  double c;
  double fval;
  int idx;
  TrialState.nVarMax = 151;
  TrialState.mNonlinIneq = 60;
  TrialState.mNonlinEq = 0;
  TrialState.mIneq = 120;
  TrialState.mEq = 0;
  TrialState.iNonIneq0 = 61;
  TrialState.iNonEq0 = 1;
  TrialState.sqpFval_old = 0.0;
  TrialState.sqpIterations = 0;
  TrialState.sqpExitFlag = 0;
  std::memset(&TrialState.lambdasqp[0], 0, 301U * sizeof(double));
  TrialState.steplength = 1.0;
  std::memset(&TrialState.delta_x[0], 0, 151U * sizeof(double));
  TrialState.fstar = 0.0;
  TrialState.firstorderopt = 0.0;
  std::memset(&TrialState.lambda[0], 0, 301U * sizeof(double));
  TrialState.state = 0;
  TrialState.maxConstr = 0.0;
  TrialState.iterations = 0;
  std::copy(&x0[0], &x0[30], &TrialState.xstarsqp[0]);
  FcnEvaluator.next.next.next.next.next.next.next.next.value = fun;
  FcnEvaluator.next.next.next.next.next.next.next.value = nonlcon;
  WorkingSet.mConstr = 0;
  WorkingSet.nVar = 30;
  std::memset(&WorkingSet.Aineq[0], 0, 18120U * sizeof(double));
  std::memset(&WorkingSet.bineq[0], 0, 120U * sizeof(double));
  std::memset(&WorkingSet.lb[0], 0, 151U * sizeof(double));
  std::memset(&WorkingSet.ub[0], 0, 151U * sizeof(double));
  std::memset(&WorkingSet.ATwset[0], 0, 45451U * sizeof(double));
  WorkingSet.nActiveConstr = 0;
  std::memset(&WorkingSet.bwset[0], 0, 301U * sizeof(double));
  std::memset(&WorkingSet.maxConstrWorkspace[0], 0, 301U * sizeof(double));
  for (int i{0}; i < 5; i++) {
    WorkingSet.sizes[i] = 0;
    WorkingSet.sizesNormal[i] = 0;
    WorkingSet.sizesPhaseOne[i] = 0;
    WorkingSet.sizesRegularized[i] = 0;
    WorkingSet.sizesRegPhaseOne[i] = 0;
  }
  for (int i{0}; i < 6; i++) {
    WorkingSet.isActiveIdx[i] = 0;
    WorkingSet.isActiveIdxNormal[i] = 0;
    WorkingSet.isActiveIdxPhaseOne[i] = 0;
    WorkingSet.isActiveIdxRegularized[i] = 0;
    WorkingSet.isActiveIdxRegPhaseOne[i] = 0;
  }
  std::memset(&WorkingSet.isActiveConstr[0], 0, 301U * sizeof(boolean_T));
  std::memset(&WorkingSet.Wid[0], 0, 301U * sizeof(int));
  std::memset(&WorkingSet.Wlocalidx[0], 0, 301U * sizeof(int));
  for (int i{0}; i < 5; i++) {
    WorkingSet.nWConstr[i] = 0;
  }
  WorkingSet.probType = 3;
  for (int i{0}; i < 151; i++) {
    signed char b_i;
    b_i = b_iv[i];
    WorkingSet.indexLB[i] = b_i;
    WorkingSet.indexUB[i] = b_i;
  }
  optim::coder::qpactiveset::WorkingSet::loadProblem(WorkingSet, Aineq);
  for (int i{0}; i < 30; i++) {
    idx = WorkingSet.indexLB[i];
    TrialState.xstarsqp[idx - 1] =
        std::fmax(TrialState.xstarsqp[idx - 1], -0.6108652381980153);
  }
  for (int i{0}; i < 30; i++) {
    idx = WorkingSet.indexUB[i];
    TrialState.xstarsqp[idx - 1] =
        std::fmin(TrialState.xstarsqp[idx - 1], 0.6108652381980153);
  }
  std::copy(&TrialState.xstarsqp[0], &TrialState.xstarsqp[30], &x[0]);
  fval = simulate_nmpc_kapal_anonFcn1(fun.workspace.s_nd,
                                      fun.workspace.x_ref_seq, x);
  if ((!std::isinf(fval)) && (!std::isnan(fval))) {
    optim::coder::utils::ObjNonlinEvaluator::computeConstraints_(
        FcnEvaluator, TrialState.xstarsqp, TrialState.cIneq);
  }
  TrialState.sqpFval = fval;
  FiniteDifferences.objfun = fun;
  FiniteDifferences.nonlin = nonlcon;
  FiniteDifferences.f_1 = 0.0;
  FiniteDifferences.numEvals = 0;
  FiniteDifferences.hasBounds = true;
  optim::coder::utils::FiniteDifferences::computeFiniteDifferences(
      FiniteDifferences, fval, TrialState.cIneq, TrialState.xstarsqp,
      TrialState.grad, WorkingSet.Aineq);
  TrialState.FunctionEvaluations = FiniteDifferences.numEvals + 1;
  for (int i{0}; i <= 58; i += 2) {
    _mm_storeu_pd(&TrialState.cIneq[i],
                  _mm_mul_pd(_mm_loadu_pd(&bineq[i]), _mm_set1_pd(-1.0)));
  }
  for (int iac{0}; iac <= 8909; iac += 151) {
    c = 0.0;
    idx = iac + 30;
    for (int i{iac + 1}; i <= idx; i++) {
      c += WorkingSet.Aineq[i - 1] * TrialState.xstarsqp[(i - iac) - 1];
    }
    idx = div_nde_s32_floor(iac, 151);
    TrialState.cIneq[idx] += c;
  }
  for (int i{0}; i <= 118; i += 2) {
    __m128d r;
    r = _mm_loadu_pd(&TrialState.cIneq[i]);
    _mm_storeu_pd(&WorkingSet.bineq[i], _mm_mul_pd(r, _mm_set1_pd(-1.0)));
  }
  for (int i{0}; i < 30; i++) {
    WorkingSet.lb[WorkingSet.indexLB[i] - 1] =
        x0[WorkingSet.indexLB[i] - 1] + 0.6108652381980153;
    WorkingSet.ub[WorkingSet.indexUB[i] - 1] =
        0.6108652381980153 - x0[WorkingSet.indexUB[i] - 1];
  }
  optim::coder::qpactiveset::WorkingSet::setProblemType(WorkingSet, 3);
  idx = WorkingSet.isActiveIdx[2];
  for (int i{idx}; i < 302; i++) {
    WorkingSet.isActiveConstr[i - 1] = false;
  }
  WorkingSet.nWConstr[0] = 0;
  WorkingSet.nWConstr[1] = 0;
  WorkingSet.nWConstr[2] = 0;
  WorkingSet.nWConstr[3] = 0;
  WorkingSet.nWConstr[4] = 0;
  WorkingSet.nActiveConstr = 0;
  MeritFunction.initFval = fval;
  MeritFunction.penaltyParam = 1.0;
  MeritFunction.threshold = 0.0001;
  MeritFunction.nPenaltyDecreases = 0;
  MeritFunction.linearizedConstrViol = 0.0;
  fval = 0.0;
  for (int i{0}; i < 120; i++) {
    c = TrialState.cIneq[i];
    if (c > 0.0) {
      fval += c;
    }
  }
  double Hessian[900];
  MeritFunction.initConstrViolationIneq = fval;
  MeritFunction.phi = 0.0;
  MeritFunction.phiPrimePlus = 0.0;
  MeritFunction.phiFullStep = 0.0;
  MeritFunction.feasRelativeFactor = 0.0;
  MeritFunction.nlpDualFeasError = 0.0;
  MeritFunction.nlpComplError = 0.0;
  std::memset(&obj.Q[0], 0, 90601U * sizeof(double));
  std::memset(&obj.jpvt[0], 0, 301U * sizeof(int));
  obj.mrows = 0;
  obj.ncols = 0;
  obj.minRowCol = 0;
  obj.usedPivoting = false;
  b_obj.ndims = 0;
  b_obj.info = 0;
  b_obj.ConvexCheck = true;
  b_obj.workspace_ = rtInf;
  b_obj.workspace2_ = rtInf;
  optim::coder::fminconsqp::driver(
      bineq, TrialState, MeritFunction, FcnEvaluator, FiniteDifferences,
      memspace, WorkingSet, obj, b_obj, Hessian, QPObjective);
  std::copy(&TrialState.xstarsqp[0], &TrialState.xstarsqp[30], &x[0]);
  fval = TrialState.sqpFval;
  exitflag = TrialState.sqpExitFlag;
  return fval;
}

} // namespace coder

// End of code generation (fmincon.cpp)

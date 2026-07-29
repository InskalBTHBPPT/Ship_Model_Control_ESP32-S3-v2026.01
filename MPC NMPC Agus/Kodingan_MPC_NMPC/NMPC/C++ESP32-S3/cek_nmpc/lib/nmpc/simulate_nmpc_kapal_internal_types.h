//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// simulate_nmpc_kapal_internal_types.h
//
// Code generation for function 'simulate_nmpc_kapal'
//

#ifndef SIMULATE_NMPC_KAPAL_INTERNAL_TYPES_H
#define SIMULATE_NMPC_KAPAL_INTERNAL_TYPES_H

// Include files
#include "anonymous_function.h"
#include "rtwtypes.h"
#include "simulate_nmpc_kapal_types.h"

// Type Definitions
struct struct_T {
  double grad[151];
  double Hx[150];
  boolean_T hasLinear;
  int nvar;
  int maxVar;
  double beta;
  double rho;
  int objtype;
  int prev_objtype;
  int prev_nvar;
  boolean_T prev_hasLinear;
  double gammaScalar;
};

struct b_struct_T {
  double workspace_float[45451];
  int workspace_int[301];
  int workspace_sort[301];
};

struct c_struct_T {
  int ldq;
  double QR[90601];
  double Q[90601];
  int jpvt[301];
  int mrows;
  int ncols;
  double tau[301];
  int minRowCol;
  boolean_T usedPivoting;
};

struct e_struct_T {
  int nVarMax;
  int mNonlinIneq;
  int mNonlinEq;
  int mIneq;
  int mEq;
  int iNonIneq0;
  int iNonEq0;
  double sqpFval;
  double sqpFval_old;
  double xstarsqp[30];
  double xstarsqp_old[30];
  double cIneq[120];
  double cIneq_old[120];
  double grad[151];
  double grad_old[151];
  int FunctionEvaluations;
  int sqpIterations;
  int sqpExitFlag;
  double lambdasqp[301];
  double lambdaStopTest[301];
  double lambdaStopTestPrev[301];
  double steplength;
  double delta_x[151];
  double socDirection[151];
  int workingset_old[301];
  double JacCineqTrans_old[9060];
  double gradLag[151];
  double delta_gradLag[151];
  double xstar[151];
  double fstar;
  double firstorderopt;
  double lambda[301];
  int state;
  double maxConstr;
  int iterations;
  double searchDir[151];
};

struct g_struct_T {
  int mConstr;
  int nVar;
  double Aineq[18120];
  double bineq[120];
  double lb[151];
  double ub[151];
  int indexLB[151];
  int indexUB[151];
  double ATwset[45451];
  double bwset[301];
  int nActiveConstr;
  double maxConstrWorkspace[301];
  int sizes[5];
  int sizesNormal[5];
  int sizesPhaseOne[5];
  int sizesRegularized[5];
  int sizesRegPhaseOne[5];
  int isActiveIdx[6];
  int isActiveIdxNormal[6];
  int isActiveIdxPhaseOne[6];
  int isActiveIdxRegularized[6];
  int isActiveIdxRegPhaseOne[6];
  boolean_T isActiveConstr[301];
  int Wid[301];
  int Wlocalidx[301];
  int nWConstr[5];
  int probType;
};

struct h_struct_T {
  double penaltyParam;
  double threshold;
  int nPenaltyDecreases;
  double linearizedConstrViol;
  double initFval;
  double initConstrViolationIneq;
  double phi;
  double phiPrimePlus;
  double phiFullStep;
  double feasRelativeFactor;
  double nlpDualFeasError;
  double nlpComplError;
};

struct i_struct_T {
  double FMat[90601];
  int ndims;
  int info;
  boolean_T ConvexCheck;
  double workspace_;
  double workspace2_;
};

struct j_struct_T {
  boolean_T fevalOK;
  boolean_T done;
  boolean_T stepAccepted;
  boolean_T failedLineSearch;
  int stepType;
};

struct k_struct_T {
  char SolverName[7];
  int MaxIterations;
};

struct l_struct_T {
  coder::b_anonymous_function objfun;
  coder::anonymous_function nonlin;
  double f_1;
  double cIneq_1[60];
  int numEvals;
  boolean_T hasBounds;
};

#endif
// End of code generation (simulate_nmpc_kapal_internal_types.h)

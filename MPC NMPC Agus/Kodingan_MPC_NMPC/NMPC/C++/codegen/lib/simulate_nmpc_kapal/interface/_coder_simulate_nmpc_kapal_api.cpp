//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_simulate_nmpc_kapal_api.cpp
//
// Code generation for function 'simulate_nmpc_kapal'
//

// Include files
#include "_coder_simulate_nmpc_kapal_api.h"
#include "_coder_simulate_nmpc_kapal_mex.h"

// Variable Definitions
emlrtCTX emlrtRootTLSGlobal{nullptr};

emlrtContext emlrtContextGlobal{
    true,                                                 // bFirstTime
    false,                                                // bInitialized
    131675U,                                              // fVersionInfo
    nullptr,                                              // fErrorFunction
    "simulate_nmpc_kapal",                                // fFunctionName
    nullptr,                                              // fRTCallStack
    false,                                                // bDebugMode
    {2045744189U, 2170104910U, 2743257031U, 4284093946U}, // fSigWrd
    nullptr                                               // fSigMem
};

// Function Declarations
static const mxArray *b_emlrt_marshallOut(real_T u[151]);

static const mxArray *c_emlrt_marshallOut(real_T u[3]);

static void emlrtExitTimeCleanupDtorFcn(const void *r);

static const mxArray *emlrt_marshallOut(real_T u[755]);

// Function Definitions
static const mxArray *b_emlrt_marshallOut(real_T u[151])
{
  static const int32_T iv[2]{0, 0};
  static const int32_T iv1[2]{1, 151};
  const mxArray *m;
  const mxArray *y;
  y = nullptr;
  m = emlrtCreateNumericArray(2, (const void *)&iv[0], mxDOUBLE_CLASS, mxREAL);
  emlrtMxSetData((mxArray *)m, &u[0]);
  emlrtSetDimensions((mxArray *)m, &iv1[0], 2);
  emlrtAssign(&y, m);
  return y;
}

static const mxArray *c_emlrt_marshallOut(real_T u[3])
{
  static const int32_T iv[2]{0, 0};
  static const int32_T iv1[2]{1, 3};
  const mxArray *m;
  const mxArray *y;
  y = nullptr;
  m = emlrtCreateNumericArray(2, (const void *)&iv[0], mxDOUBLE_CLASS, mxREAL);
  emlrtMxSetData((mxArray *)m, &u[0]);
  emlrtSetDimensions((mxArray *)m, &iv1[0], 2);
  emlrtAssign(&y, m);
  return y;
}

static void emlrtExitTimeCleanupDtorFcn(const void *r)
{
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

static const mxArray *emlrt_marshallOut(real_T u[755])
{
  static const int32_T iv[2]{0, 0};
  static const int32_T iv1[2]{5, 151};
  const mxArray *m;
  const mxArray *y;
  y = nullptr;
  m = emlrtCreateNumericArray(2, (const void *)&iv[0], mxDOUBLE_CLASS, mxREAL);
  emlrtMxSetData((mxArray *)m, &u[0]);
  emlrtSetDimensions((mxArray *)m, &iv1[0], 2);
  emlrtAssign(&y, m);
  return y;
}

void simulate_nmpc_kapal_api(int32_T nlhs, const mxArray *plhs[4])
{
  real_T(*hist_dim)[755];
  real_T(*history_input)[151];
  real_T(*time_vector)[151];
  real_T(*rmse_data)[3];
  hist_dim = (real_T(*)[755])mxMalloc(sizeof(real_T[755]));
  history_input = (real_T(*)[151])mxMalloc(sizeof(real_T[151]));
  time_vector = (real_T(*)[151])mxMalloc(sizeof(real_T[151]));
  rmse_data = (real_T(*)[3])mxMalloc(sizeof(real_T[3]));
  // Invoke the target function
  simulate_nmpc_kapal(*hist_dim, *history_input, *time_vector, *rmse_data);
  // Marshall function outputs
  plhs[0] = emlrt_marshallOut(*hist_dim);
  if (nlhs > 1) {
    plhs[1] = b_emlrt_marshallOut(*history_input);
  }
  if (nlhs > 2) {
    plhs[2] = b_emlrt_marshallOut(*time_vector);
  }
  if (nlhs > 3) {
    plhs[3] = c_emlrt_marshallOut(*rmse_data);
  }
}

void simulate_nmpc_kapal_atexit()
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtPushHeapReferenceStackR2021a(&st, false, nullptr,
                                    (void *)&emlrtExitTimeCleanupDtorFcn,
                                    nullptr, nullptr, nullptr);
  emlrtEnterRtStackR2012b(&st);
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
  simulate_nmpc_kapal_xil_terminate();
  simulate_nmpc_kapal_xil_shutdown();
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

void simulate_nmpc_kapal_initialize()
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtClearAllocCountR2012b(&st, false, 0U, nullptr);
  emlrtEnterRtStackR2012b(&st);
  emlrtFirstTimeR2012b(emlrtRootTLSGlobal);
}

void simulate_nmpc_kapal_terminate()
{
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
}

// End of code generation (_coder_simulate_nmpc_kapal_api.cpp)

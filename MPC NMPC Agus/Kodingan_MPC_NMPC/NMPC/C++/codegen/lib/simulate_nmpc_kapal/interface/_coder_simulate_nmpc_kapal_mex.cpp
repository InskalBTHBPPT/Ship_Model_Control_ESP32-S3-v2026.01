//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_simulate_nmpc_kapal_mex.cpp
//
// Code generation for function 'simulate_nmpc_kapal'
//

// Include files
#include "_coder_simulate_nmpc_kapal_mex.h"
#include "_coder_simulate_nmpc_kapal_api.h"

// Function Definitions
void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs, const mxArray *[])
{
  mexAtExit(&simulate_nmpc_kapal_atexit);
  simulate_nmpc_kapal_initialize();
  unsafe_simulate_nmpc_kapal_mexFunction(nlhs, plhs, nrhs);
  simulate_nmpc_kapal_terminate();
}

emlrtCTX mexFunctionCreateRootTLS()
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, nullptr, 1,
                           nullptr, "windows-1252", true);
  return emlrtRootTLSGlobal;
}

void unsafe_simulate_nmpc_kapal_mexFunction(int32_T nlhs, mxArray *plhs[4],
                                            int32_T nrhs)
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  const mxArray *outputs[4];
  int32_T i;
  st.tls = emlrtRootTLSGlobal;
  // Check for proper number of arguments.
  if (nrhs != 0) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 0, 4,
                        19, "simulate_nmpc_kapal");
  }
  if (nlhs > 4) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 19,
                        "simulate_nmpc_kapal");
  }
  // Call the function.
  simulate_nmpc_kapal_api(nlhs, outputs);
  // Copy over outputs to the caller.
  if (nlhs < 1) {
    i = 1;
  } else {
    i = nlhs;
  }
  emlrtReturnArrays(i, &plhs[0], &outputs[0]);
}

// End of code generation (_coder_simulate_nmpc_kapal_mex.cpp)

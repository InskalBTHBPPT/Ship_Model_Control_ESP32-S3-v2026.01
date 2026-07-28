//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_simulate_nmpc_kapal_api.h
//
// Code generation for function 'simulate_nmpc_kapal'
//

#ifndef _CODER_SIMULATE_NMPC_KAPAL_API_H
#define _CODER_SIMULATE_NMPC_KAPAL_API_H

// Include files
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"
#include <algorithm>
#include <cstring>

// Variable Declarations
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

// Function Declarations
void simulate_nmpc_kapal(real_T hist_dim[755], real_T history_input[151],
                         real_T time_vector[151], real_T rmse_data[3]);

void simulate_nmpc_kapal_api(int32_T nlhs, const mxArray *plhs[4]);

void simulate_nmpc_kapal_atexit();

void simulate_nmpc_kapal_initialize();

void simulate_nmpc_kapal_terminate();

void simulate_nmpc_kapal_xil_shutdown();

void simulate_nmpc_kapal_xil_terminate();

#endif
// End of code generation (_coder_simulate_nmpc_kapal_api.h)

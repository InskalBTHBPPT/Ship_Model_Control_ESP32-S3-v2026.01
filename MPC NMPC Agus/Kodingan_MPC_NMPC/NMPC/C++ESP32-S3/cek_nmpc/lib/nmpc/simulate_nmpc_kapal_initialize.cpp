//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// simulate_nmpc_kapal_initialize.cpp
//
// Code generation for function 'simulate_nmpc_kapal_initialize'
//

// Include files
#include "simulate_nmpc_kapal_initialize.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_data.h"
#include "omp.h"
#include <cstring>

// Function Definitions
void simulate_nmpc_kapal_initialize()
{
  omp_init_nest_lock(&simulate_nmpc_kapal_nestLockGlobal);
  isInitialized_simulate_nmpc_kapal = true;
}

// End of code generation (simulate_nmpc_kapal_initialize.cpp)

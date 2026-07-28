//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// simulate_nmpc_kapal_rtwutil.cpp
//
// Code generation for function 'simulate_nmpc_kapal_rtwutil'
//

// Include files
#include "simulate_nmpc_kapal_rtwutil.h"
#include "rt_nonfinite.h"
#include <cstring>

// Function Definitions
int div_nde_s32_floor(int numerator, int denominator)
{
  int quotient;
  if (((numerator < 0) != (denominator < 0)) &&
      (numerator % denominator != 0)) {
    quotient = -1;
  } else {
    quotient = 0;
  }
  quotient += numerator / denominator;
  return quotient;
}

// End of code generation (simulate_nmpc_kapal_rtwutil.cpp)

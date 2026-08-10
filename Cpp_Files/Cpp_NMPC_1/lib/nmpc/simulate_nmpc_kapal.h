//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// simulate_nmpc_kapal.h
//
// Code generation for function 'simulate_nmpc_kapal'
//

#ifndef SIMULATE_NMPC_KAPAL_H
#define SIMULATE_NMPC_KAPAL_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void simulate_nmpc_kapal(double hist_dim[755], double history_input[151],
                                double time_vector[151], double rmse_data[3]);

double simulate_nmpc_kapal_anonFcn1(const double s_nd[5],
                                    const double x_ref_seq[30],
                                    const double U[30]);

void simulate_nmpc_kapal_anonFcn2(const double s_nd[5], const double U[30],
                                  double varargout_1[60]);

#endif
// End of code generation (simulate_nmpc_kapal.h)

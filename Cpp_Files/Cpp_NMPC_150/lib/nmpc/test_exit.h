//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// test_exit.h
//
// Code generation for function 'test_exit'
//

#ifndef TEST_EXIT_H
#define TEST_EXIT_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct h_struct_T;

struct g_struct_T;

struct e_struct_T;

struct j_struct_T;

struct b_struct_T;

struct c_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
void b_test_exit(j_struct_T &Flags, b_struct_T &memspace,
                 h_struct_T &b_MeritFunction, g_struct_T &WorkingSet,
                 e_struct_T &b_TrialState, c_struct_T &b_QRManager);

boolean_T test_exit(h_struct_T &b_MeritFunction, const g_struct_T &WorkingSet,
                    e_struct_T &b_TrialState, boolean_T &Flags_fevalOK,
                    boolean_T &Flags_done, boolean_T &Flags_stepAccepted,
                    boolean_T &Flags_failedLineSearch, int &Flags_stepType);

} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (test_exit.h)

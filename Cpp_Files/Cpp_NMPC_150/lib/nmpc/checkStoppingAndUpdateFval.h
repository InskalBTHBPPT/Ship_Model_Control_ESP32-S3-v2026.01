//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// checkStoppingAndUpdateFval.h
//
// Code generation for function 'checkStoppingAndUpdateFval'
//

#ifndef CHECKSTOPPINGANDUPDATEFVAL_H
#define CHECKSTOPPINGANDUPDATEFVAL_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct e_struct_T;

struct b_struct_T;

struct struct_T;

struct g_struct_T;

struct c_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace stopping {
void b_checkStoppingAndUpdateFval(int &activeSetChangeID, e_struct_T &solution,
                                  b_struct_T &memspace,
                                  const struct_T &objective,
                                  g_struct_T &workingset, c_struct_T &qrmanager,
                                  int runTimeOptions_MaxIterations,
                                  boolean_T &updateFval);

void checkStoppingAndUpdateFval(int &activeSetChangeID, const double f[151],
                                e_struct_T &solution, b_struct_T &memspace,
                                const struct_T &objective,
                                g_struct_T &workingset, c_struct_T &qrmanager,
                                int runTimeOptions_MaxIterations,
                                const boolean_T &updateFval);

} // namespace stopping
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (checkStoppingAndUpdateFval.h)

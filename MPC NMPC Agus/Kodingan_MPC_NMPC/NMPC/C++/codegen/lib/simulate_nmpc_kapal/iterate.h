//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// iterate.h
//
// Code generation for function 'iterate'
//

#ifndef ITERATE_H
#define ITERATE_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct e_struct_T;

struct b_struct_T;

struct g_struct_T;

struct c_struct_T;

struct i_struct_T;

struct struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
void iterate(const double H[900], const double f[151], e_struct_T &solution,
             b_struct_T &memspace, g_struct_T &workingset,
             c_struct_T &qrmanager, i_struct_T &cholmanager,
             struct_T &objective, const char options_SolverName[7],
             int runTimeOptions_MaxIterations);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (iterate.h)

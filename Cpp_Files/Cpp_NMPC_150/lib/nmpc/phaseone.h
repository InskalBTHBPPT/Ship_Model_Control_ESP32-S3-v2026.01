//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// phaseone.h
//
// Code generation for function 'phaseone'
//

#ifndef PHASEONE_H
#define PHASEONE_H

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

struct k_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
void phaseone(const double H[900], const double f[151], e_struct_T &solution,
              b_struct_T &memspace, g_struct_T &workingset,
              c_struct_T &qrmanager, i_struct_T &cholmanager,
              struct_T &objective, const char options_SolverName[7],
              const k_struct_T &runTimeOptions);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (phaseone.h)

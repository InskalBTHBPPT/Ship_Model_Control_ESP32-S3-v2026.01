//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// normal.h
//
// Code generation for function 'normal'
//

#ifndef NORMAL_H
#define NORMAL_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct e_struct_T;

struct h_struct_T;

struct b_struct_T;

struct g_struct_T;

struct c_struct_T;

struct i_struct_T;

struct struct_T;

struct k_struct_T;

struct j_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace step {
void normal(const double Hessian[900], const double grad[151],
            e_struct_T &b_TrialState, h_struct_T &b_MeritFunction,
            b_struct_T &memspace, g_struct_T &WorkingSet,
            c_struct_T &b_QRManager, i_struct_T &b_CholManager,
            struct_T &QPObjective, const k_struct_T &qpoptions,
            j_struct_T &stepFlags);

}
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (normal.h)

//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// driver1.h
//
// Code generation for function 'driver1'
//

#ifndef DRIVER1_H
#define DRIVER1_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct e_struct_T;

struct h_struct_T;

namespace coder {
namespace internal {
class i_stickyStruct;

}
} // namespace coder
struct l_struct_T;

struct b_struct_T;

struct g_struct_T;

struct c_struct_T;

struct i_struct_T;

struct struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
void driver(const double bineq[60], e_struct_T &b_TrialState,
            h_struct_T &b_MeritFunction,
            const ::coder::internal::i_stickyStruct &FcnEvaluator,
            l_struct_T &FiniteDifferences, b_struct_T &memspace,
            g_struct_T &WorkingSet, c_struct_T &b_QRManager,
            i_struct_T &b_CholManager, double Hessian[900],
            struct_T &QPObjective);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (driver1.h)

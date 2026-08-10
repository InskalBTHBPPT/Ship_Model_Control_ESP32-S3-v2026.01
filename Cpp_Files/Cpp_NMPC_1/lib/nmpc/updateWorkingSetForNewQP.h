//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// updateWorkingSetForNewQP.h
//
// Code generation for function 'updateWorkingSetForNewQP'
//

#ifndef UPDATEWORKINGSETFORNEWQP_H
#define UPDATEWORKINGSETFORNEWQP_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct g_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace internal {
void updateWorkingSetForNewQP(const double xk[30], g_struct_T &WorkingSet,
                              const double cIneq[120], int mLB);

}
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (updateWorkingSetForNewQP.h)

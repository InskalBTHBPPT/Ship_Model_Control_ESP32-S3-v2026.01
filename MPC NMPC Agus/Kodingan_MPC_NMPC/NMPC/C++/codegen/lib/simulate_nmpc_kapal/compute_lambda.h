//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// compute_lambda.h
//
// Code generation for function 'compute_lambda'
//

#ifndef COMPUTE_LAMBDA_H
#define COMPUTE_LAMBDA_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct e_struct_T;

struct struct_T;

struct c_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
void compute_lambda(double workspace[45451], e_struct_T &solution,
                    const struct_T &objective, const c_struct_T &qrmanager);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (compute_lambda.h)

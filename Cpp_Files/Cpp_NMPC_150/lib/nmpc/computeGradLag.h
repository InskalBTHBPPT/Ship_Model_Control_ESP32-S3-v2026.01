//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeGradLag.h
//
// Code generation for function 'computeGradLag'
//

#ifndef COMPUTEGRADLAG_H
#define COMPUTEGRADLAG_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace stopping {
void b_computeGradLag(double workspace[45451], int nVar, const double grad[151],
                      const double AineqTrans[18120], const int finiteLB[151],
                      int mLB, const int finiteUB[151],
                      const double lambda[301]);

void computeGradLag(double workspace[151], int nVar, const double grad[151],
                    const double AineqTrans[18120], const int finiteLB[151],
                    int mLB, const int finiteUB[151], const double lambda[301]);

} // namespace stopping
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (computeGradLag.h)

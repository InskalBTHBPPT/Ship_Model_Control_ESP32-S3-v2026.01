//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeComplError.h
//
// Code generation for function 'computeComplError'
//

#ifndef COMPUTECOMPLERROR_H
#define COMPUTECOMPLERROR_H

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
double computeComplError(const double xCurrent[30], const double cIneq[120],
                         const int finiteLB[151], int mLB,
                         const int finiteUB[151], const double lambda[301]);

}
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (computeComplError.h)

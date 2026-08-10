//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeLinearResiduals.h
//
// Code generation for function 'computeLinearResiduals'
//

#ifndef COMPUTELINEARRESIDUALS_H
#define COMPUTELINEARRESIDUALS_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace internal {
void computeLinearResiduals(const double x[30], int nVar,
                            double workspaceIneq[120],
                            const double AineqT[18120], const double bineq[60]);

}
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (computeLinearResiduals.h)

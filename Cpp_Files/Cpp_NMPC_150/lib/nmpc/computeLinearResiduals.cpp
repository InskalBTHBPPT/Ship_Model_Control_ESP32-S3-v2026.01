//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeLinearResiduals.cpp
//
// Code generation for function 'computeLinearResiduals'
//

// Include files
#include "computeLinearResiduals.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace internal {
void computeLinearResiduals(const double x[30], int nVar,
                            double workspaceIneq[120],
                            const double AineqT[18120], const double bineq[60])
{
  for (int k{0}; k <= 58; k += 2) {
    _mm_storeu_pd(&workspaceIneq[k],
                  _mm_mul_pd(_mm_loadu_pd(&bineq[k]), _mm_set1_pd(-1.0)));
  }
  for (int iac{0}; iac <= 8909; iac += 151) {
    double c;
    int i;
    c = 0.0;
    i = iac + nVar;
    for (int k{iac + 1}; k <= i; k++) {
      c += AineqT[k - 1] * x[(k - iac) - 1];
    }
    i = div_nde_s32_floor(iac, 151);
    workspaceIneq[i] += c;
  }
}

} // namespace internal
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (computeLinearResiduals.cpp)

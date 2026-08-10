//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeGradLag.cpp
//
// Code generation for function 'computeGradLag'
//

// Include files
#include "computeGradLag.h"
#include "rt_nonfinite.h"
#include <algorithm>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace stopping {
void b_computeGradLag(double workspace[45451], int nVar, const double grad[151],
                      const double AineqTrans[18120], const int finiteLB[151],
                      int mLB, const int finiteUB[151],
                      const double lambda[301])
{
  int i;
  int ix;
  ix = static_cast<unsigned char>(nVar);
  if (ix - 1 >= 0) {
    std::copy(&grad[0], &grad[ix], &workspace[0]);
  }
  ix = 0;
  for (int iac{0}; iac <= 17969; iac += 151) {
    i = iac + nVar;
    for (int ia{iac + 1}; ia <= i; ia++) {
      int i1;
      i1 = (ia - iac) - 1;
      workspace[i1] += AineqTrans[ia - 1] * lambda[ix];
    }
    ix++;
  }
  ix = static_cast<unsigned char>(mLB);
  for (int iac{0}; iac < ix; iac++) {
    i = finiteLB[iac];
    workspace[i - 1] -= lambda[iac + 120];
  }
  for (int iac{0}; iac < 30; iac++) {
    ix = finiteUB[iac];
    workspace[ix - 1] += lambda[(static_cast<unsigned char>(mLB) + iac) + 120];
  }
}

void computeGradLag(double workspace[151], int nVar, const double grad[151],
                    const double AineqTrans[18120], const int finiteLB[151],
                    int mLB, const int finiteUB[151], const double lambda[301])
{
  int i;
  int ix;
  ix = static_cast<unsigned char>(nVar);
  if (ix - 1 >= 0) {
    std::copy(&grad[0], &grad[ix], &workspace[0]);
  }
  ix = 0;
  for (int iac{0}; iac <= 17969; iac += 151) {
    i = iac + nVar;
    for (int ia{iac + 1}; ia <= i; ia++) {
      int i1;
      i1 = (ia - iac) - 1;
      workspace[i1] += AineqTrans[ia - 1] * lambda[ix];
    }
    ix++;
  }
  ix = static_cast<unsigned char>(mLB);
  for (int iac{0}; iac < ix; iac++) {
    i = finiteLB[iac];
    workspace[i - 1] -= lambda[iac + 120];
  }
  for (int iac{0}; iac < 30; iac++) {
    ix = finiteUB[iac];
    workspace[ix - 1] += lambda[(static_cast<unsigned char>(mLB) + iac) + 120];
  }
}

} // namespace stopping
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (computeGradLag.cpp)

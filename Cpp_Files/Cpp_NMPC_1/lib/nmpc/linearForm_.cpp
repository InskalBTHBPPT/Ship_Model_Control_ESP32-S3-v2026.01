//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// linearForm_.cpp
//
// Code generation for function 'linearForm_'
//

// Include files
#include "linearForm_.h"
#include "rt_nonfinite.h"
#include <algorithm>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace Objective {
void linearForm_(boolean_T obj_hasLinear, int obj_nvar, double workspace[45451],
                 const double H[900], const double f[151], const double x[151])
{
  int ix;
  ix = 0;
  if (obj_hasLinear) {
    if (obj_nvar - 1 >= 0) {
      std::copy(&f[0], &f[obj_nvar], &workspace[0]);
    }
    ix = 1;
  }
  if (obj_nvar != 0) {
    int i;
    if ((ix != 1) && (obj_nvar - 1 >= 0)) {
      std::memset(&workspace[0], 0,
                  static_cast<unsigned int>(obj_nvar) * sizeof(double));
    }
    ix = 0;
    i = obj_nvar * (obj_nvar - 1) + 1;
    for (int iac{1}; obj_nvar < 0 ? iac >= i : iac <= i; iac += obj_nvar) {
      double c;
      int i1;
      c = 0.5 * x[ix];
      i1 = iac + obj_nvar;
      for (int ia{iac}; ia < i1; ia++) {
        int i2;
        i2 = ia - iac;
        workspace[i2] += H[ia - 1] * c;
      }
      ix++;
    }
  }
}

} // namespace Objective
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (linearForm_.cpp)

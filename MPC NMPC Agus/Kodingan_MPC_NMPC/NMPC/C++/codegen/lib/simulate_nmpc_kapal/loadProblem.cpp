//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// loadProblem.cpp
//
// Code generation for function 'loadProblem'
//

// Include files
#include "loadProblem.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace WorkingSet {
void loadProblem(g_struct_T &obj, const double Aineq[1800])
{
  static const signed char b_iv[6]{1, 0, 0, 120, 30, 30};
  static const signed char iv2[6]{1, 0, 0, 120, 31, 30};
  static const unsigned char uv2[6]{1U, 0U, 0U, 120U, 150U, 30U};
  static const unsigned char uv3[6]{1U, 0U, 0U, 120U, 151U, 30U};
  static const signed char iv1[5]{0, 0, 120, 31, 30};
  static const signed char obj_tmp[5]{0, 0, 120, 30, 30};
  static const unsigned char uv[5]{0U, 0U, 120U, 150U, 30U};
  static const unsigned char uv1[5]{0U, 0U, 120U, 151U, 30U};
  obj.mConstr = 180;
  for (int i{0}; i < 5; i++) {
    signed char b_i;
    b_i = obj_tmp[i];
    obj.sizes[i] = b_i;
    obj.sizesNormal[i] = b_i;
    obj.sizesPhaseOne[i] = iv1[i];
    obj.sizesRegularized[i] = uv[i];
    obj.sizesRegPhaseOne[i] = uv1[i];
  }
  for (int i{0}; i < 6; i++) {
    obj.isActiveIdxRegPhaseOne[i] = b_iv[i];
  }
  for (int i{0}; i < 5; i++) {
    obj.isActiveIdxRegPhaseOne[i + 1] += obj.isActiveIdxRegPhaseOne[i];
  }
  for (int i{0}; i < 6; i++) {
    int i1;
    i1 = obj.isActiveIdxRegPhaseOne[i];
    obj.isActiveIdx[i] = i1;
    obj.isActiveIdxNormal[i] = i1;
    obj.isActiveIdxRegPhaseOne[i] = iv2[i];
  }
  for (int i{0}; i < 5; i++) {
    obj.isActiveIdxRegPhaseOne[i + 1] += obj.isActiveIdxRegPhaseOne[i];
  }
  for (int i{0}; i < 6; i++) {
    obj.isActiveIdxPhaseOne[i] = obj.isActiveIdxRegPhaseOne[i];
    obj.isActiveIdxRegPhaseOne[i] = uv2[i];
  }
  for (int i{0}; i < 5; i++) {
    obj.isActiveIdxRegPhaseOne[i + 1] += obj.isActiveIdxRegPhaseOne[i];
  }
  for (int i{0}; i < 6; i++) {
    obj.isActiveIdxRegularized[i] = obj.isActiveIdxRegPhaseOne[i];
    obj.isActiveIdxRegPhaseOne[i] = uv3[i];
  }
  for (int i{0}; i < 5; i++) {
    obj.isActiveIdxRegPhaseOne[i + 1] += obj.isActiveIdxRegPhaseOne[i];
  }
  for (int i{0}; i < 60; i++) {
    for (int idx_row{0}; idx_row < 30; idx_row++) {
      obj.Aineq[idx_row + 151 * i] = Aineq[i + 60 * idx_row];
    }
  }
}

} // namespace WorkingSet
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (loadProblem.cpp)

//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// computeConstraints_.cpp
//
// Code generation for function 'computeConstraints_'
//

// Include files
#include "computeConstraints_.h"
#include "anonymous_function.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal.h"
#include "simulate_nmpc_kapal_internal_types1.h"
#include "stickyStruct.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace utils {
namespace ObjNonlinEvaluator {
int computeConstraints_(const ::coder::internal::i_stickyStruct &obj,
                        const double x[30], double Cineq_workspace[120])
{
  double varargout_1[60];
  int idx_current;
  int status;
  boolean_T allFinite;
  simulate_nmpc_kapal_anonFcn2(
      obj.next.next.next.next.next.next.next.value.workspace.s_nd, x,
      varargout_1);
  std::copy(&varargout_1[0], &varargout_1[60], &Cineq_workspace[60]);
  status = 1;
  allFinite = true;
  idx_current = 60;
  while (allFinite && (idx_current + 1 <= 120)) {
    allFinite = ((!std::isinf(Cineq_workspace[idx_current])) &&
                 (!std::isnan(Cineq_workspace[idx_current])));
    idx_current++;
  }
  if (!allFinite) {
    idx_current--;
    if (std::isnan(Cineq_workspace[idx_current])) {
      status = -3;
    } else if (Cineq_workspace[idx_current] < 0.0) {
      status = -1;
    } else {
      status = -2;
    }
  }
  return status;
}

} // namespace ObjNonlinEvaluator
} // namespace utils
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (computeConstraints_.cpp)

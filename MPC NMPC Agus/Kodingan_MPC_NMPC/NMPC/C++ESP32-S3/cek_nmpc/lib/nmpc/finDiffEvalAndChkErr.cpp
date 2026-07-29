//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// finDiffEvalAndChkErr.cpp
//
// Code generation for function 'finDiffEvalAndChkErr'
//

// Include files
#include "finDiffEvalAndChkErr.h"
#include "anonymous_function.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal.h"
#include "simulate_nmpc_kapal_internal_types1.h"
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace utils {
namespace FiniteDifferences {
namespace internal {
boolean_T finDiffEvalAndChkErr(const b_anonymous_function &obj_objfun,
                               const anonymous_function obj_nonlin,
                               double cIneqPlus[60], int dim, double delta,
                               double xk[30], double &fplus)
{
  double temp;
  boolean_T evalOK;
  temp = xk[dim - 1];
  xk[dim - 1] = temp + delta;
  fplus = simulate_nmpc_kapal_anonFcn1(obj_objfun.workspace.s_nd,
                                       obj_objfun.workspace.x_ref_seq, xk);
  evalOK = ((!std::isinf(fplus)) && (!std::isnan(fplus)));
  if (evalOK) {
    int idx;
    simulate_nmpc_kapal_anonFcn2(obj_nonlin.workspace.s_nd, xk, cIneqPlus);
    idx = 0;
    while (evalOK && (idx + 1 <= 60)) {
      evalOK = ((!std::isinf(cIneqPlus[idx])) && (!std::isnan(cIneqPlus[idx])));
      idx++;
    }
    if (evalOK) {
      xk[dim - 1] = temp;
    }
  }
  return evalOK;
}

} // namespace internal
} // namespace FiniteDifferences
} // namespace utils
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (finDiffEvalAndChkErr.cpp)

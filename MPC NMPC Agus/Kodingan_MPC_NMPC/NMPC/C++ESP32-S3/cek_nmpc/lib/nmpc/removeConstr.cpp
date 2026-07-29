//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// removeConstr.cpp
//
// Code generation for function 'removeConstr'
//

// Include files
#include "removeConstr.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace WorkingSet {
void removeConstr(g_struct_T &obj, int idx_global)
{
  int TYPE;
  TYPE = obj.Wid[idx_global - 1] - 1;
  obj.isActiveConstr[(obj.isActiveIdx[obj.Wid[idx_global - 1] - 1] +
                      obj.Wlocalidx[idx_global - 1]) -
                     2] = false;
  if (idx_global < obj.nActiveConstr) {
    int i;
    obj.Wid[idx_global - 1] = obj.Wid[obj.nActiveConstr - 1];
    obj.Wlocalidx[idx_global - 1] = obj.Wlocalidx[obj.nActiveConstr - 1];
    i = static_cast<unsigned char>(obj.nVar);
    for (int idx{0}; idx < i; idx++) {
      obj.ATwset[idx + 151 * (idx_global - 1)] =
          obj.ATwset[idx + 151 * (obj.nActiveConstr - 1)];
    }
    obj.bwset[idx_global - 1] = obj.bwset[obj.nActiveConstr - 1];
  }
  obj.nActiveConstr--;
  obj.nWConstr[TYPE]--;
}

} // namespace WorkingSet
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (removeConstr.cpp)

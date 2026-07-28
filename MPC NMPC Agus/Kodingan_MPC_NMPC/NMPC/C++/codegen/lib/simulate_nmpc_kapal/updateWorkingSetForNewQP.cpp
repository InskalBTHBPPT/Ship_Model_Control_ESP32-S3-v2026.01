//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// updateWorkingSetForNewQP.cpp
//
// Code generation for function 'updateWorkingSetForNewQP'
//

// Include files
#include "updateWorkingSetForNewQP.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace fminconsqp {
namespace internal {
void updateWorkingSetForNewQP(const double xk[30], g_struct_T &WorkingSet,
                              const double cIneq[120], int mLB)
{
  int i;
  for (int idx{0}; idx <= 118; idx += 2) {
    _mm_storeu_pd(&WorkingSet.bineq[idx],
                  _mm_mul_pd(_mm_loadu_pd(&cIneq[idx]), _mm_set1_pd(-1.0)));
  }
  i = static_cast<unsigned char>(mLB);
  for (int idx{0}; idx < i; idx++) {
    WorkingSet.lb[WorkingSet.indexLB[idx] - 1] =
        xk[WorkingSet.indexLB[idx] - 1] + 0.6108652381980153;
  }
  for (int idx{0}; idx < 30; idx++) {
    WorkingSet.ub[WorkingSet.indexUB[idx] - 1] =
        0.6108652381980153 - xk[WorkingSet.indexUB[idx] - 1];
  }
  if (WorkingSet.nActiveConstr > 0) {
    i = WorkingSet.nActiveConstr;
    for (int idx{1}; idx <= i; idx++) {
      switch (WorkingSet.Wid[idx - 1]) {
      case 4:
        WorkingSet.bwset[idx - 1] =
            WorkingSet
                .lb[WorkingSet.indexLB[WorkingSet.Wlocalidx[idx - 1] - 1] - 1];
        break;
      case 5:
        WorkingSet.bwset[idx - 1] =
            WorkingSet
                .ub[WorkingSet.indexUB[WorkingSet.Wlocalidx[idx - 1] - 1] - 1];
        break;
      default: {
        WorkingSet.bwset[idx - 1] =
            WorkingSet.bineq[WorkingSet.Wlocalidx[idx - 1] - 1];
        if (WorkingSet.Wlocalidx[idx - 1] > 60) {
          int i1;
          int ix0;
          int iy0;
          iy0 = 151 * (idx - 1);
          ix0 = 151 * (WorkingSet.Wlocalidx[idx - 1] - 1);
          i1 = static_cast<unsigned char>(WorkingSet.nVar);
          for (int k{0}; k < i1; k++) {
            WorkingSet.ATwset[iy0 + k] = WorkingSet.Aineq[ix0 + k];
          }
        }
      } break;
      }
    }
  }
}

} // namespace internal
} // namespace fminconsqp
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (updateWorkingSetForNewQP.cpp)

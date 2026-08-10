//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// sortLambdaQP.cpp
//
// Code generation for function 'sortLambdaQP'
//

// Include files
#include "sortLambdaQP.h"
#include "rt_nonfinite.h"
#include <algorithm>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace parseoutput {
void sortLambdaQP(double lambda[301], int WorkingSet_nActiveConstr,
                  const int WorkingSet_sizes[5],
                  const int WorkingSet_isActiveIdx[6],
                  const int WorkingSet_Wid[301],
                  const int WorkingSet_Wlocalidx[301], double workspace[45451])
{
  if (WorkingSet_nActiveConstr != 0) {
    int currentMplier;
    int idx;
    int mAll;
    mAll = WorkingSet_sizes[3] + 149;
    currentMplier = static_cast<unsigned short>(WorkingSet_sizes[3] + 150);
    if (currentMplier - 1 >= 0) {
      std::copy(&lambda[0], &lambda[currentMplier], &workspace[0]);
    }
    if (mAll >= 0) {
      std::memset(&lambda[0], 0,
                  static_cast<unsigned int>(mAll + 1) * sizeof(double));
    }
    currentMplier = 0;
    idx = 0;
    while ((idx + 1 <= WorkingSet_nActiveConstr) &&
           (WorkingSet_Wid[idx] <= 2)) {
      if (WorkingSet_Wid[idx] == 1) {
        mAll = 1;
      } else {
        mAll = WorkingSet_isActiveIdx[1];
      }
      lambda[(mAll + WorkingSet_Wlocalidx[idx]) - 2] = workspace[currentMplier];
      currentMplier++;
      idx++;
    }
    while (idx + 1 <= WorkingSet_nActiveConstr) {
      switch (WorkingSet_Wid[idx]) {
      case 3:
        mAll = WorkingSet_isActiveIdx[2];
        break;
      case 4:
        mAll = WorkingSet_isActiveIdx[3];
        break;
      default:
        mAll = WorkingSet_isActiveIdx[4];
        break;
      }
      lambda[(mAll + WorkingSet_Wlocalidx[idx]) - 2] = workspace[currentMplier];
      currentMplier++;
      idx++;
    }
  }
}

} // namespace parseoutput
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (sortLambdaQP.cpp)

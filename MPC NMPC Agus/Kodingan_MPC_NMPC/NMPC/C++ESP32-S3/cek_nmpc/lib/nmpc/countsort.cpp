//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// countsort.cpp
//
// Code generation for function 'countsort'
//

// Include files
#include "countsort.h"
#include "rt_nonfinite.h"
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace utils {
void countsort(int x[301], int xLen, int workspace[301], int xMin, int xMax)
{
  if ((xLen > 1) && (xMax > xMin)) {
    int i;
    int idxEnd;
    int idxStart;
    i = xMax - xMin;
    if (i >= 0) {
      std::memset(&workspace[0], 0,
                  static_cast<unsigned int>(i + 1) * sizeof(int));
    }
    for (int idx{0}; idx < xLen; idx++) {
      idxStart = x[idx] - xMin;
      workspace[idxStart]++;
    }
    for (int idx{2}; idx <= i + 1; idx++) {
      workspace[idx - 1] += workspace[idx - 2];
    }
    idxStart = 1;
    idxEnd = workspace[0];
    for (int idx{0}; idx < i; idx++) {
      for (int idxFill{idxStart}; idxFill <= idxEnd; idxFill++) {
        x[idxFill - 1] = idx + xMin;
      }
      idxStart = workspace[idx] + 1;
      idxEnd = workspace[idx + 1];
    }
    for (int idx{idxStart}; idx <= idxEnd; idx++) {
      x[idx - 1] = xMax;
    }
  }
}

} // namespace utils
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (countsort.cpp)

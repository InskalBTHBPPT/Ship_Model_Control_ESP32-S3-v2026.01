//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// feasibleratiotest.h
//
// Code generation for function 'feasibleratiotest'
//

#ifndef FEASIBLERATIOTEST_H
#define FEASIBLERATIOTEST_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
double feasibleratiotest(
    const double solution_xstar[151], const double solution_searchDir[151],
    double workspace[45451], int workingset_nVar,
    const double workingset_Aineq[18120], const double workingset_bineq[120],
    const double workingset_lb[151], const double workingset_ub[151],
    const int workingset_indexLB[151], const int workingset_indexUB[151],
    const int workingset_sizes[5], const int workingset_isActiveIdx[6],
    const boolean_T workingset_isActiveConstr[301],
    const int workingset_nWConstr[5], boolean_T isPhaseOne,
    boolean_T &newBlocking, int &constrType, int &constrIdx);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (feasibleratiotest.h)

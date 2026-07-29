//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// feasibleratiotest.cpp
//
// Code generation for function 'feasibleratiotest'
//

// Include files
#include "feasibleratiotest.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "xnrm2.h"
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
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
    boolean_T &newBlocking, int &constrType, int &constrIdx)
{
  double alpha;
  double c;
  double denomTol;
  double phaseOneCorrectionP;
  double phaseOneCorrectionX;
  double ratio;
  int i;
  alpha = 1.0E+30;
  newBlocking = false;
  constrType = 0;
  constrIdx = 0;
  denomTol = 2.2204460492503131E-13 *
             internal::blas::xnrm2(workingset_nVar, solution_searchDir);
  if (workingset_nWConstr[2] < 120) {
    for (int k{0}; k <= 118; k += 2) {
      __m128d r;
      r = _mm_loadu_pd(&workingset_bineq[k]);
      r = _mm_mul_pd(r, _mm_set1_pd(-1.0));
      _mm_storeu_pd(&workspace[k], r);
    }
    for (int iac{0}; iac <= 17969; iac += 151) {
      c = 0.0;
      i = iac + workingset_nVar;
      for (int k{iac + 1}; k <= i; k++) {
        c += workingset_Aineq[k - 1] * solution_xstar[(k - iac) - 1];
      }
      i = div_nde_s32_floor(iac, 151);
      workspace[i] += c;
    }
    std::memset(&workspace[301], 0, 120U * sizeof(double));
    for (int iac{0}; iac <= 17969; iac += 151) {
      c = 0.0;
      i = iac + workingset_nVar;
      for (int k{iac + 1}; k <= i; k++) {
        c += workingset_Aineq[k - 1] * solution_searchDir[(k - iac) - 1];
      }
      i = div_nde_s32_floor(iac, 151) + 301;
      workspace[i] += c;
    }
    for (int k{0}; k < 120; k++) {
      c = workspace[k + 301];
      if ((c > denomTol) &&
          (!workingset_isActiveConstr[(workingset_isActiveIdx[2] + k) - 1])) {
        phaseOneCorrectionX = workspace[k];
        c = std::fmin(std::abs(phaseOneCorrectionX),
                      1.0E-6 - phaseOneCorrectionX) /
            c;
        if (c < alpha) {
          alpha = c;
          constrType = 3;
          constrIdx = k + 1;
          newBlocking = true;
        }
      }
    }
  }
  if (workingset_nWConstr[3] < workingset_sizes[3]) {
    phaseOneCorrectionX =
        static_cast<double>(isPhaseOne) * solution_xstar[workingset_nVar - 1];
    phaseOneCorrectionP = static_cast<double>(isPhaseOne) *
                          solution_searchDir[workingset_nVar - 1];
    i = static_cast<unsigned char>(workingset_sizes[3] - 1);
    for (int k{0}; k < i; k++) {
      int i1;
      i1 = workingset_indexLB[k];
      c = -solution_searchDir[i1 - 1] - phaseOneCorrectionP;
      if ((c > denomTol) &&
          (!workingset_isActiveConstr[(workingset_isActiveIdx[3] + k) - 1])) {
        ratio = (-solution_xstar[i1 - 1] - workingset_lb[i1 - 1]) -
                phaseOneCorrectionX;
        c = std::fmin(std::abs(ratio), 1.0E-6 - ratio) / c;
        if (c < alpha) {
          alpha = c;
          constrType = 4;
          constrIdx = k + 1;
          newBlocking = true;
        }
      }
    }
    i = workingset_indexLB[workingset_sizes[3] - 1] - 1;
    c = -solution_searchDir[i];
    if ((c > denomTol) &&
        (!workingset_isActiveConstr
             [(workingset_isActiveIdx[3] + workingset_sizes[3]) - 2])) {
      ratio = -solution_xstar[i] - workingset_lb[i];
      c = std::fmin(std::abs(ratio), 1.0E-6 - ratio) / c;
      if (c < alpha) {
        alpha = c;
        constrType = 4;
        constrIdx = workingset_sizes[3];
        newBlocking = true;
      }
    }
  }
  if (workingset_nWConstr[4] < 30) {
    phaseOneCorrectionX =
        static_cast<double>(isPhaseOne) * solution_xstar[workingset_nVar - 1];
    phaseOneCorrectionP = static_cast<double>(isPhaseOne) *
                          solution_searchDir[workingset_nVar - 1];
    for (int k{0}; k < 30; k++) {
      i = workingset_indexUB[k];
      c = solution_searchDir[i - 1] - phaseOneCorrectionP;
      if ((c > denomTol) &&
          (!workingset_isActiveConstr[(workingset_isActiveIdx[4] + k) - 1])) {
        ratio = (solution_xstar[i - 1] - workingset_ub[i - 1]) -
                phaseOneCorrectionX;
        c = std::fmin(std::abs(ratio), 1.0E-6 - ratio) / c;
        if (c < alpha) {
          alpha = c;
          constrType = 5;
          constrIdx = k + 1;
          newBlocking = true;
        }
      }
    }
  }
  if (!isPhaseOne) {
    if (newBlocking && (alpha > 1.0)) {
      newBlocking = false;
    }
    alpha = std::fmin(alpha, 1.0);
  }
  return alpha;
}

} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (feasibleratiotest.cpp)

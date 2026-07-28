//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// maxConstraintViolation.cpp
//
// Code generation for function 'maxConstraintViolation'
//

// Include files
#include "maxConstraintViolation.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace WorkingSet {
double b_maxConstraintViolation(g_struct_T &obj, const double x[151])
{
  double v;
  int idxUB;
  if (obj.probType == 2) {
    double c;
    v = 0.0;
    for (int k{0}; k <= 118; k += 2) {
      __m128d r;
      r = _mm_loadu_pd(&obj.bineq[k]);
      _mm_storeu_pd(&obj.maxConstrWorkspace[k],
                    _mm_mul_pd(r, _mm_set1_pd(-1.0)));
    }
    for (int iac{0}; iac <= 17969; iac += 151) {
      c = 0.0;
      idxUB = iac + 30;
      for (int k{iac + 1}; k <= idxUB; k++) {
        c += obj.Aineq[k - 1] * x[(k - iac) - 1];
      }
      idxUB = div_nde_s32_floor(iac, 151);
      obj.maxConstrWorkspace[idxUB] += c;
    }
    for (int k{0}; k < 120; k++) {
      c = obj.maxConstrWorkspace[k] - x[k + 30];
      obj.maxConstrWorkspace[k] = c;
      v = std::fmax(v, c);
    }
  } else {
    v = 0.0;
    for (int k{0}; k <= 118; k += 2) {
      __m128d r;
      r = _mm_loadu_pd(&obj.bineq[k]);
      _mm_storeu_pd(&obj.maxConstrWorkspace[k],
                    _mm_mul_pd(r, _mm_set1_pd(-1.0)));
    }
    for (int iac{0}; iac <= 17969; iac += 151) {
      double c;
      c = 0.0;
      idxUB = iac + obj.nVar;
      for (int k{iac + 1}; k <= idxUB; k++) {
        c += obj.Aineq[k - 1] * x[(k - iac) - 1];
      }
      idxUB = div_nde_s32_floor(iac, 151);
      obj.maxConstrWorkspace[idxUB] += c;
    }
    for (int k{0}; k < 120; k++) {
      v = std::fmax(v, obj.maxConstrWorkspace[k]);
    }
  }
  idxUB = static_cast<unsigned char>(obj.sizes[3]);
  for (int k{0}; k < idxUB; k++) {
    int idxLB;
    idxLB = obj.indexLB[k] - 1;
    v = std::fmax(v, -x[idxLB] - obj.lb[idxLB]);
  }
  for (int k{0}; k < 30; k++) {
    idxUB = obj.indexUB[k] - 1;
    v = std::fmax(v, x[idxUB] - obj.ub[idxUB]);
  }
  return v;
}

double maxConstraintViolation(
    int obj_nVar, const double obj_Aineq[18120], const double obj_bineq[120],
    const double obj_lb[151], const double obj_ub[151],
    const int obj_indexLB[151], const int obj_indexUB[151],
    const double obj_maxConstrWorkspace[301], const int obj_sizes[5],
    int obj_probType, const double x[151])
{
  double b_obj_maxConstrWorkspace[301];
  double v;
  int obj_maxConstrWorkspace_tmp;
  if (obj_probType == 2) {
    double c;
    v = 0.0;
    std::copy(&obj_maxConstrWorkspace[0], &obj_maxConstrWorkspace[301],
              &b_obj_maxConstrWorkspace[0]);
    for (int k{0}; k <= 118; k += 2) {
      _mm_storeu_pd(&b_obj_maxConstrWorkspace[k],
                    _mm_mul_pd(_mm_loadu_pd(&obj_bineq[k]), _mm_set1_pd(-1.0)));
    }
    for (int iac{0}; iac <= 17969; iac += 151) {
      c = 0.0;
      obj_maxConstrWorkspace_tmp = iac + 30;
      for (int k{iac + 1}; k <= obj_maxConstrWorkspace_tmp; k++) {
        c += obj_Aineq[k - 1] * x[(k - iac) - 1];
      }
      obj_maxConstrWorkspace_tmp = div_nde_s32_floor(iac, 151);
      b_obj_maxConstrWorkspace[obj_maxConstrWorkspace_tmp] += c;
    }
    for (int k{0}; k < 120; k++) {
      c = b_obj_maxConstrWorkspace[k] - x[k + 30];
      b_obj_maxConstrWorkspace[k] = c;
      v = std::fmax(v, c);
    }
  } else {
    v = 0.0;
    std::copy(&obj_maxConstrWorkspace[0], &obj_maxConstrWorkspace[301],
              &b_obj_maxConstrWorkspace[0]);
    for (int k{0}; k <= 118; k += 2) {
      _mm_storeu_pd(&b_obj_maxConstrWorkspace[k],
                    _mm_mul_pd(_mm_loadu_pd(&obj_bineq[k]), _mm_set1_pd(-1.0)));
    }
    for (int iac{0}; iac <= 17969; iac += 151) {
      double c;
      c = 0.0;
      obj_maxConstrWorkspace_tmp = iac + obj_nVar;
      for (int k{iac + 1}; k <= obj_maxConstrWorkspace_tmp; k++) {
        c += obj_Aineq[k - 1] * x[(k - iac) - 1];
      }
      obj_maxConstrWorkspace_tmp = div_nde_s32_floor(iac, 151);
      b_obj_maxConstrWorkspace[obj_maxConstrWorkspace_tmp] += c;
    }
    for (int k{0}; k < 120; k++) {
      v = std::fmax(v, b_obj_maxConstrWorkspace[k]);
    }
  }
  obj_maxConstrWorkspace_tmp = static_cast<unsigned char>(obj_sizes[3]);
  for (int k{0}; k < obj_maxConstrWorkspace_tmp; k++) {
    int i;
    i = obj_indexLB[k];
    v = std::fmax(v, -x[i - 1] - obj_lb[i - 1]);
  }
  for (int k{0}; k < 30; k++) {
    obj_maxConstrWorkspace_tmp = obj_indexUB[k];
    v = std::fmax(v, x[obj_maxConstrWorkspace_tmp - 1] -
                         obj_ub[obj_maxConstrWorkspace_tmp - 1]);
  }
  return v;
}

double maxConstraintViolation(g_struct_T &obj, const double x[45451])
{
  double v;
  int idxUB;
  if (obj.probType == 2) {
    double c;
    v = 0.0;
    for (int k{0}; k <= 118; k += 2) {
      __m128d r;
      r = _mm_loadu_pd(&obj.bineq[k]);
      _mm_storeu_pd(&obj.maxConstrWorkspace[k],
                    _mm_mul_pd(r, _mm_set1_pd(-1.0)));
    }
    for (int iac{0}; iac <= 17969; iac += 151) {
      c = 0.0;
      idxUB = iac + 30;
      for (int k{iac + 1}; k <= idxUB; k++) {
        c += obj.Aineq[k - 1] * x[(k - iac) + 300];
      }
      idxUB = div_nde_s32_floor(iac, 151);
      obj.maxConstrWorkspace[idxUB] += c;
    }
    for (int k{0}; k < 120; k++) {
      c = obj.maxConstrWorkspace[k] - x[k + 331];
      obj.maxConstrWorkspace[k] = c;
      v = std::fmax(v, c);
    }
  } else {
    v = 0.0;
    for (int k{0}; k <= 118; k += 2) {
      __m128d r;
      r = _mm_loadu_pd(&obj.bineq[k]);
      _mm_storeu_pd(&obj.maxConstrWorkspace[k],
                    _mm_mul_pd(r, _mm_set1_pd(-1.0)));
    }
    for (int iac{0}; iac <= 17969; iac += 151) {
      double c;
      c = 0.0;
      idxUB = iac + obj.nVar;
      for (int k{iac + 1}; k <= idxUB; k++) {
        c += obj.Aineq[k - 1] * x[(k - iac) + 300];
      }
      idxUB = div_nde_s32_floor(iac, 151);
      obj.maxConstrWorkspace[idxUB] += c;
    }
    for (int k{0}; k < 120; k++) {
      v = std::fmax(v, obj.maxConstrWorkspace[k]);
    }
  }
  idxUB = static_cast<unsigned char>(obj.sizes[3]);
  for (int k{0}; k < idxUB; k++) {
    int idxLB;
    idxLB = obj.indexLB[k];
    v = std::fmax(v, -x[idxLB + 300] - obj.lb[idxLB - 1]);
  }
  for (int k{0}; k < 30; k++) {
    idxUB = obj.indexUB[k];
    v = std::fmax(v, x[idxUB + 300] - obj.ub[idxUB - 1]);
  }
  return v;
}

} // namespace WorkingSet
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (maxConstraintViolation.cpp)

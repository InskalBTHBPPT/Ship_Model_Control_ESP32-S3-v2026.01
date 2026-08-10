//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// compute_deltax.cpp
//
// Code generation for function 'compute_deltax'
//

// Include files
#include "compute_deltax.h"
#include "fullColLDL2_.h"
#include "partialColLDL3_.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "simulate_nmpc_kapal_rtwutil.h"
#include "solve.h"
#include "xgemm.h"
#include "xpotrf.h"
#include <cstring>
#include <emmintrin.h>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
void compute_deltax(const double H[900], e_struct_T &solution,
                    b_struct_T &memspace, const c_struct_T &qrmanager,
                    i_struct_T &cholmanager, const struct_T &objective,
                    boolean_T alwaysPositiveDef)
{
  int mNull;
  int nVar;
  nVar = qrmanager.mrows - 1;
  mNull = qrmanager.mrows - qrmanager.ncols;
  if (mNull <= 0) {
    if (nVar >= 0) {
      std::memset(&solution.searchDir[0], 0,
                  static_cast<unsigned int>(nVar + 1) * sizeof(double));
    }
  } else {
    __m128d r;
    int LD_diagOffset;
    int ix;
    ix = ((nVar + 1) / 2) << 1;
    LD_diagOffset = ix - 2;
    for (int idx{0}; idx <= LD_diagOffset; idx += 2) {
      r = _mm_loadu_pd(&objective.grad[idx]);
      _mm_storeu_pd(&solution.searchDir[idx], _mm_mul_pd(r, _mm_set1_pd(-1.0)));
    }
    for (int idx{ix}; idx <= nVar; idx++) {
      solution.searchDir[idx] = -objective.grad[idx];
    }
    if (qrmanager.ncols <= 0) {
      switch (objective.objtype) {
      case 5:
        break;
      case 3: {
        int b_ix;
        int jjA;
        if (alwaysPositiveDef) {
          ix = qrmanager.mrows;
          cholmanager.ndims = qrmanager.mrows;
          for (int idx{0}; idx < ix; idx++) {
            LD_diagOffset = qrmanager.mrows * idx;
            jjA = 301 * idx;
            for (int k{0}; k < ix; k++) {
              cholmanager.FMat[jjA + k] = H[LD_diagOffset + k];
            }
          }
          cholmanager.info =
              internal::lapack::xpotrf(qrmanager.mrows, cholmanager.FMat);
        } else {
          b_ix = qrmanager.mrows;
          cholmanager.ndims = qrmanager.mrows;
          for (int idx{0}; idx < b_ix; idx++) {
            LD_diagOffset = qrmanager.mrows * idx;
            jjA = 301 * idx;
            for (int k{0}; k < b_ix; k++) {
              cholmanager.FMat[jjA + k] = H[LD_diagOffset + k];
            }
          }
          if (qrmanager.mrows > 128) {
            boolean_T exitg1;
            ix = 0;
            exitg1 = false;
            while ((!exitg1) && (ix < b_ix)) {
              LD_diagOffset = 302 * ix + 1;
              jjA = b_ix - ix;
              if (ix + 48 <= b_ix) {
                DynamicRegCholManager::partialColLDL3_(cholmanager,
                                                       LD_diagOffset, jjA);
                ix += 48;
              } else {
                DynamicRegCholManager::fullColLDL2_(cholmanager, LD_diagOffset,
                                                    jjA);
                exitg1 = true;
              }
            }
          } else {
            DynamicRegCholManager::fullColLDL2_(cholmanager, 1,
                                                qrmanager.mrows);
          }
          if (cholmanager.ConvexCheck) {
            ix = 0;
            int exitg2;
            do {
              exitg2 = 0;
              if (ix <= b_ix - 1) {
                if (cholmanager.FMat[ix + 301 * ix] <= 0.0) {
                  cholmanager.info = -ix - 1;
                  exitg2 = 1;
                } else {
                  ix++;
                }
              } else {
                cholmanager.ConvexCheck = false;
                exitg2 = 1;
              }
            } while (exitg2 == 0);
          }
        }
        if (cholmanager.info != 0) {
          solution.state = -6;
        } else if (alwaysPositiveDef) {
          CholManager::solve(cholmanager, solution.searchDir);
        } else {
          LD_diagOffset = cholmanager.ndims - 2;
          if (cholmanager.ndims != 0) {
            for (int idx{0}; idx <= LD_diagOffset + 1; idx++) {
              jjA = idx + idx * 301;
              b_ix = LD_diagOffset - idx;
              for (int k{0}; k <= b_ix; k++) {
                ix = (idx + k) + 1;
                solution.searchDir[ix] -=
                    solution.searchDir[idx] * cholmanager.FMat[(jjA + k) + 1];
              }
            }
          }
          b_ix = cholmanager.ndims;
          for (int idx{0}; idx < b_ix; idx++) {
            solution.searchDir[idx] /= cholmanager.FMat[idx + 301 * idx];
          }
          if (cholmanager.ndims != 0) {
            for (int idx{b_ix}; idx >= 1; idx--) {
              double temp;
              ix = (idx - 1) * 301;
              temp = solution.searchDir[idx - 1];
              LD_diagOffset = idx + 1;
              for (int k{b_ix}; k >= LD_diagOffset; k--) {
                temp -=
                    cholmanager.FMat[(ix + k) - 1] * solution.searchDir[k - 1];
              }
              solution.searchDir[idx - 1] = temp;
            }
          }
        }
      } break;
      case 4: {
        if (alwaysPositiveDef) {
          int jjA;
          ix = objective.nvar;
          cholmanager.ndims = objective.nvar;
          for (int idx{0}; idx < ix; idx++) {
            LD_diagOffset = objective.nvar * idx;
            jjA = 301 * idx;
            for (int k{0}; k < ix; k++) {
              cholmanager.FMat[jjA + k] = H[LD_diagOffset + k];
            }
          }
          cholmanager.info =
              internal::lapack::xpotrf(objective.nvar, cholmanager.FMat);
          if (cholmanager.info != 0) {
            solution.state = -6;
          } else {
            double temp;
            int b_ix;
            CholManager::solve(cholmanager, solution.searchDir);
            temp = 1.0 / objective.beta;
            ix = objective.nvar + 1;
            LD_diagOffset = qrmanager.mrows;
            jjA = ((((LD_diagOffset - ix) + 1) / 2) << 1) + ix;
            b_ix = jjA - 2;
            for (int idx{ix}; idx <= b_ix; idx += 2) {
              r = _mm_loadu_pd(&solution.searchDir[idx - 1]);
              _mm_storeu_pd(&solution.searchDir[idx - 1],
                            _mm_mul_pd(_mm_set1_pd(temp), r));
            }
            for (int idx{jjA}; idx <= LD_diagOffset; idx++) {
              solution.searchDir[idx - 1] *= temp;
            }
          }
        }
      } break;
      }
    } else {
      int nullStartIdx;
      nullStartIdx = 301 * qrmanager.ncols + 1;
      if (objective.objtype == 5) {
        for (int idx{0}; idx < mNull; idx++) {
          memspace.workspace_float[idx] =
              -qrmanager.Q[nVar + 301 * (qrmanager.ncols + idx)];
        }
        if (qrmanager.mrows != 0) {
          int b_ix;
          if (nVar >= 0) {
            std::memset(&solution.searchDir[0], 0,
                        static_cast<unsigned int>(nVar + 1) * sizeof(double));
          }
          b_ix = 0;
          LD_diagOffset = nullStartIdx + 301 * (mNull - 1);
          for (int idx{nullStartIdx}; idx <= LD_diagOffset; idx += 301) {
            int jjA;
            jjA = idx + nVar;
            for (int k{idx}; k <= jjA; k++) {
              ix = k - idx;
              solution.searchDir[ix] +=
                  qrmanager.Q[k - 1] * memspace.workspace_float[b_ix];
            }
            b_ix++;
          }
        }
      } else {
        int b_ix;
        int jjA;
        if (objective.objtype == 3) {
          internal::blas::xgemm(qrmanager.mrows, mNull, qrmanager.mrows, H,
                                qrmanager.mrows, qrmanager.Q, nullStartIdx,
                                memspace.workspace_float);
          internal::blas::xgemm(mNull, mNull, qrmanager.mrows, qrmanager.Q,
                                nullStartIdx, memspace.workspace_float,
                                cholmanager.FMat);
        } else if (alwaysPositiveDef) {
          LD_diagOffset = qrmanager.mrows;
          internal::blas::xgemm(objective.nvar, mNull, objective.nvar, H,
                                objective.nvar, qrmanager.Q, nullStartIdx,
                                memspace.workspace_float);
          jjA = objective.nvar + 1;
          ix = ((((LD_diagOffset - jjA) + 1) / 2) << 1) + jjA;
          b_ix = ix - 2;
          for (int idx{0}; idx < mNull; idx++) {
            for (int k{jjA}; k <= b_ix; k += 2) {
              r = _mm_loadu_pd(
                  &qrmanager.Q[(k + 301 * (idx + qrmanager.ncols)) - 1]);
              _mm_storeu_pd(&memspace.workspace_float[(k + 301 * idx) - 1],
                            _mm_mul_pd(_mm_set1_pd(objective.beta), r));
            }
            for (int k{ix}; k <= LD_diagOffset; k++) {
              memspace.workspace_float[(k + 301 * idx) - 1] =
                  objective.beta *
                  qrmanager.Q[(k + 301 * (idx + qrmanager.ncols)) - 1];
            }
          }
          internal::blas::xgemm(mNull, mNull, qrmanager.mrows, qrmanager.Q,
                                nullStartIdx, memspace.workspace_float,
                                cholmanager.FMat);
        }
        if (alwaysPositiveDef) {
          cholmanager.ndims = mNull;
          cholmanager.info = internal::lapack::xpotrf(mNull, cholmanager.FMat);
        } else {
          cholmanager.ndims = mNull;
          if (mNull > 128) {
            boolean_T exitg1;
            LD_diagOffset = 0;
            exitg1 = false;
            while ((!exitg1) && (LD_diagOffset < mNull)) {
              b_ix = 302 * LD_diagOffset + 1;
              jjA = mNull - LD_diagOffset;
              if (LD_diagOffset + 48 <= mNull) {
                DynamicRegCholManager::partialColLDL3_(cholmanager, b_ix, jjA);
                LD_diagOffset += 48;
              } else {
                DynamicRegCholManager::fullColLDL2_(cholmanager, b_ix, jjA);
                exitg1 = true;
              }
            }
          } else {
            DynamicRegCholManager::fullColLDL2_(cholmanager, 1, mNull);
          }
          if (cholmanager.ConvexCheck) {
            ix = 0;
            int exitg2;
            do {
              exitg2 = 0;
              if (ix <= mNull - 1) {
                if (cholmanager.FMat[ix + 301 * ix] <= 0.0) {
                  cholmanager.info = -ix - 1;
                  exitg2 = 1;
                } else {
                  ix++;
                }
              } else {
                cholmanager.ConvexCheck = false;
                exitg2 = 1;
              }
            } while (exitg2 == 0);
          }
        }
        if (cholmanager.info != 0) {
          solution.state = -6;
        } else {
          double temp;
          if (qrmanager.mrows != 0) {
            std::memset(&memspace.workspace_float[0], 0,
                        static_cast<unsigned int>(mNull) * sizeof(double));
            jjA = nullStartIdx + 301 * (mNull - 1);
            for (int k{nullStartIdx}; k <= jjA; k += 301) {
              temp = 0.0;
              LD_diagOffset = k + nVar;
              for (int idx{k}; idx <= LD_diagOffset; idx++) {
                temp += qrmanager.Q[idx - 1] * objective.grad[idx - k];
              }
              LD_diagOffset = div_nde_s32_floor(k - nullStartIdx, 301);
              memspace.workspace_float[LD_diagOffset] -= temp;
            }
          }
          if (alwaysPositiveDef) {
            ix = cholmanager.ndims;
            if (cholmanager.ndims != 0) {
              for (int idx{0}; idx < ix; idx++) {
                LD_diagOffset = idx * 301;
                temp = memspace.workspace_float[idx];
                for (int k{0}; k < idx; k++) {
                  temp -= cholmanager.FMat[LD_diagOffset + k] *
                          memspace.workspace_float[k];
                }
                memspace.workspace_float[idx] =
                    temp / cholmanager.FMat[LD_diagOffset + idx];
              }
            }
            if (cholmanager.ndims != 0) {
              for (int idx{ix}; idx >= 1; idx--) {
                LD_diagOffset = (idx + (idx - 1) * 301) - 1;
                memspace.workspace_float[idx - 1] /=
                    cholmanager.FMat[LD_diagOffset];
                for (int k{0}; k <= idx - 2; k++) {
                  jjA = (idx - k) - 2;
                  memspace.workspace_float[jjA] -=
                      memspace.workspace_float[idx - 1] *
                      cholmanager.FMat[(LD_diagOffset - k) - 1];
                }
              }
            }
          } else {
            LD_diagOffset = cholmanager.ndims - 2;
            if (cholmanager.ndims != 0) {
              for (int idx{0}; idx <= LD_diagOffset + 1; idx++) {
                jjA = idx + idx * 301;
                ix = LD_diagOffset - idx;
                for (int k{0}; k <= ix; k++) {
                  b_ix = (idx + k) + 1;
                  memspace.workspace_float[b_ix] -=
                      memspace.workspace_float[idx] *
                      cholmanager.FMat[(jjA + k) + 1];
                }
              }
            }
            ix = cholmanager.ndims;
            for (int idx{0}; idx < ix; idx++) {
              memspace.workspace_float[idx] /=
                  cholmanager.FMat[idx + 301 * idx];
            }
            if (cholmanager.ndims != 0) {
              for (int idx{ix}; idx >= 1; idx--) {
                LD_diagOffset = (idx - 1) * 301;
                temp = memspace.workspace_float[idx - 1];
                jjA = idx + 1;
                for (int k{ix}; k >= jjA; k--) {
                  temp -= cholmanager.FMat[(LD_diagOffset + k) - 1] *
                          memspace.workspace_float[k - 1];
                }
                memspace.workspace_float[idx - 1] = temp;
              }
            }
          }
          if (qrmanager.mrows != 0) {
            if (nVar >= 0) {
              std::memset(&solution.searchDir[0], 0,
                          static_cast<unsigned int>(nVar + 1) * sizeof(double));
            }
            b_ix = 0;
            LD_diagOffset = nullStartIdx + 301 * (mNull - 1);
            for (int idx{nullStartIdx}; idx <= LD_diagOffset; idx += 301) {
              jjA = idx + nVar;
              for (int k{idx}; k <= jjA; k++) {
                ix = k - idx;
                solution.searchDir[ix] +=
                    qrmanager.Q[k - 1] * memspace.workspace_float[b_ix];
              }
              b_ix++;
            }
          }
        }
      }
    }
  }
}

} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (compute_deltax.cpp)

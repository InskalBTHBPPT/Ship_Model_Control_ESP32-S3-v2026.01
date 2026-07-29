//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// RemoveDependentIneq_.cpp
//
// Code generation for function 'RemoveDependentIneq_'
//

// Include files
#include "RemoveDependentIneq_.h"
#include "countsort.h"
#include "removeConstr.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include "xgeqp3.h"
#include <cmath>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace initialize {
void RemoveDependentIneq_(g_struct_T &workingset, c_struct_T &qrmanager,
                          b_struct_T &memspace, double tolfactor)
{
  int nActiveConstr;
  int nFixedConstr;
  int nVar;
  nActiveConstr = workingset.nActiveConstr;
  nFixedConstr = workingset.nWConstr[0] + workingset.nWConstr[1];
  nVar = workingset.nVar;
  if ((workingset.nWConstr[2] + workingset.nWConstr[3]) +
          workingset.nWConstr[4] >
      0) {
    double maxDiag;
    double tol;
    int idxDiag;
    int nDepIneq;
    int u1;
    idxDiag = workingset.nVar;
    u1 = workingset.nActiveConstr;
    if (idxDiag >= u1) {
      u1 = idxDiag;
    }
    tol =
        tolfactor * std::fmin(1.4901161193847656E-8,
                              2.2204460492503131E-15 * static_cast<double>(u1));
    for (int idx{0}; idx < nFixedConstr; idx++) {
      qrmanager.jpvt[idx] = 1;
    }
    idxDiag = nFixedConstr + 1;
    if (idxDiag <= nActiveConstr) {
      std::memset(&qrmanager.jpvt[idxDiag + -1], 0,
                  static_cast<unsigned int>((nActiveConstr - idxDiag) + 1) *
                      sizeof(int));
    }
    for (int idx{0}; idx < nActiveConstr; idx++) {
      idxDiag = 301 * idx;
      u1 = 151 * idx;
      nDepIneq = static_cast<unsigned char>(nVar);
      for (int k{0}; k < nDepIneq; k++) {
        qrmanager.QR[idxDiag + k] = workingset.ATwset[u1 + k];
      }
    }
    if (workingset.nVar * workingset.nActiveConstr == 0) {
      qrmanager.mrows = workingset.nVar;
      qrmanager.ncols = workingset.nActiveConstr;
      qrmanager.minRowCol = 0;
    } else {
      qrmanager.usedPivoting = true;
      qrmanager.mrows = workingset.nVar;
      qrmanager.ncols = workingset.nActiveConstr;
      idxDiag = workingset.nVar;
      u1 = workingset.nActiveConstr;
      if (idxDiag <= u1) {
        u1 = idxDiag;
      }
      qrmanager.minRowCol = u1;
      internal::lapack::xgeqp3(qrmanager.QR, workingset.nVar,
                               workingset.nActiveConstr, qrmanager.jpvt,
                               qrmanager.tau);
    }
    nDepIneq = 0;
    for (u1 = workingset.nActiveConstr - 1; u1 + 1 > nVar; u1--) {
      nDepIneq++;
      memspace.workspace_int[nDepIneq - 1] = qrmanager.jpvt[u1];
    }
    maxDiag = std::abs(qrmanager.QR[0]);
    for (int idx{0}; idx < u1; idx++) {
      maxDiag = std::fmax(maxDiag,
                          std::abs(qrmanager.QR[(301 * (idx + 1) + idx) + 1]));
    }
    if (u1 + 1 <= workingset.nVar) {
      idxDiag = u1 + 301 * u1;
      while ((u1 + 1 > nFixedConstr) &&
             (std::abs(qrmanager.QR[idxDiag]) < tol * maxDiag)) {
        nDepIneq++;
        memspace.workspace_int[nDepIneq - 1] = qrmanager.jpvt[u1];
        u1--;
        idxDiag -= 302;
      }
    }
    utils::countsort(memspace.workspace_int, nDepIneq, memspace.workspace_sort,
                     nFixedConstr + 1, workingset.nActiveConstr);
    for (int idx{nDepIneq}; idx >= 1; idx--) {
      WorkingSet::removeConstr(workingset, memspace.workspace_int[idx - 1]);
    }
  }
}

} // namespace initialize
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (RemoveDependentIneq_.cpp)

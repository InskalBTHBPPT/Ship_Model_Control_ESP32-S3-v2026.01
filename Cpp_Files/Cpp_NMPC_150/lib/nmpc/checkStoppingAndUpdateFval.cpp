//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// checkStoppingAndUpdateFval.cpp
//
// Code generation for function 'checkStoppingAndUpdateFval'
//

// Include files
#include "checkStoppingAndUpdateFval.h"
#include "computeFval_ReuseHx.h"
#include "feasibleX0ForWorkingSet.h"
#include "maxConstraintViolation.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_internal_types.h"
#include <algorithm>
#include <cstring>

// Function Definitions
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace stopping {
void b_checkStoppingAndUpdateFval(int &activeSetChangeID, e_struct_T &solution,
                                  b_struct_T &memspace,
                                  const struct_T &objective,
                                  g_struct_T &workingset, c_struct_T &qrmanager,
                                  int runTimeOptions_MaxIterations,
                                  boolean_T &updateFval)
{
  int nVar;
  solution.iterations++;
  nVar = objective.nvar;
  if ((solution.iterations >= runTimeOptions_MaxIterations) &&
      ((solution.state != 1) || (objective.objtype == 5))) {
    solution.state = 0;
  }
  if (solution.iterations - solution.iterations / 50 * 50 == 0) {
    double tempMaxConstr;
    tempMaxConstr =
        WorkingSet::b_maxConstraintViolation(workingset, solution.xstar);
    solution.maxConstr = tempMaxConstr;
    if (objective.objtype == 5) {
      tempMaxConstr = solution.maxConstr - solution.xstar[objective.nvar - 1];
    }
    if (tempMaxConstr > 1.0E-6) {
      boolean_T nonDegenerateWset;
      if (nVar - 1 >= 0) {
        std::copy(&solution.xstar[0], &solution.xstar[nVar],
                  &solution.searchDir[0]);
      }
      nonDegenerateWset = initialize::feasibleX0ForWorkingSet(
          memspace.workspace_float, solution.searchDir, workingset, qrmanager);
      if ((!nonDegenerateWset) && (solution.state != 0)) {
        solution.state = -2;
      }
      activeSetChangeID = 0;
      tempMaxConstr =
          WorkingSet::b_maxConstraintViolation(workingset, solution.searchDir);
      if (tempMaxConstr < solution.maxConstr) {
        if (nVar - 1 >= 0) {
          std::copy(&solution.searchDir[0], &solution.searchDir[nVar],
                    &solution.xstar[0]);
        }
        solution.maxConstr = tempMaxConstr;
      }
    }
  }
  if (updateFval) {
    updateFval = false;
  }
}

void checkStoppingAndUpdateFval(int &activeSetChangeID, const double f[151],
                                e_struct_T &solution, b_struct_T &memspace,
                                const struct_T &objective,
                                g_struct_T &workingset, c_struct_T &qrmanager,
                                int runTimeOptions_MaxIterations,
                                const boolean_T &updateFval)
{
  int nVar;
  solution.iterations++;
  nVar = objective.nvar;
  if ((solution.iterations >= runTimeOptions_MaxIterations) &&
      ((solution.state != 1) || (objective.objtype == 5))) {
    solution.state = 0;
  }
  if (solution.iterations - solution.iterations / 50 * 50 == 0) {
    double tempMaxConstr;
    tempMaxConstr =
        WorkingSet::b_maxConstraintViolation(workingset, solution.xstar);
    solution.maxConstr = tempMaxConstr;
    if (objective.objtype == 5) {
      tempMaxConstr = solution.maxConstr - solution.xstar[objective.nvar - 1];
    }
    if (tempMaxConstr > 1.0E-6) {
      boolean_T nonDegenerateWset;
      if (nVar - 1 >= 0) {
        std::copy(&solution.xstar[0], &solution.xstar[nVar],
                  &solution.searchDir[0]);
      }
      nonDegenerateWset = initialize::feasibleX0ForWorkingSet(
          memspace.workspace_float, solution.searchDir, workingset, qrmanager);
      if ((!nonDegenerateWset) && (solution.state != 0)) {
        solution.state = -2;
      }
      activeSetChangeID = 0;
      tempMaxConstr =
          WorkingSet::b_maxConstraintViolation(workingset, solution.searchDir);
      if (tempMaxConstr < solution.maxConstr) {
        if (nVar - 1 >= 0) {
          std::copy(&solution.searchDir[0], &solution.searchDir[nVar],
                    &solution.xstar[0]);
        }
        solution.maxConstr = tempMaxConstr;
      }
    }
  }
  if (updateFval) {
    solution.fstar = Objective::computeFval_ReuseHx(
        objective, memspace.workspace_float, f, solution.xstar);
    if ((solution.fstar < 1.0E-6) &&
        ((solution.state != 0) || (objective.objtype != 5))) {
      solution.state = 2;
    }
  }
}

} // namespace stopping
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

// End of code generation (checkStoppingAndUpdateFval.cpp)

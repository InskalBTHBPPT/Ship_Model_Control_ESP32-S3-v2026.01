//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// maxConstraintViolation.h
//
// Code generation for function 'maxConstraintViolation'
//

#ifndef MAXCONSTRAINTVIOLATION_H
#define MAXCONSTRAINTVIOLATION_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct g_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace qpactiveset {
namespace WorkingSet {
double b_maxConstraintViolation(g_struct_T &obj, const double x[151]);

double maxConstraintViolation(
    int obj_nVar, const double obj_Aineq[18120], const double obj_bineq[120],
    const double obj_lb[151], const double obj_ub[151],
    const int obj_indexLB[151], const int obj_indexUB[151],
    const double obj_maxConstrWorkspace[301], const int obj_sizes[5],
    int obj_probType, const double x[151]);

double maxConstraintViolation(g_struct_T &obj, const double x[45451]);

} // namespace WorkingSet
} // namespace qpactiveset
} // namespace coder
} // namespace optim
} // namespace coder

#endif
// End of code generation (maxConstraintViolation.h)

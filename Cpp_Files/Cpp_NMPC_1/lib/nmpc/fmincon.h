//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// fmincon.h
//
// Code generation for function 'fmincon'
//

#ifndef FMINCON_H
#define FMINCON_H

// Include files
#include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
namespace coder {
class b_anonymous_function;

class anonymous_function;

} // namespace coder

// Function Declarations
namespace coder {
double fmincon(const b_anonymous_function &fun, const double x0[30],
               const double Aineq[1800], const double bineq[60],
               const anonymous_function nonlcon, double x[30],
               double &exitflag);

}

#endif
// End of code generation (fmincon.h)

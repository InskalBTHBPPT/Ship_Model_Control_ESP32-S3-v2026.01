#ifndef NMPC_SOLVE_ONCE_H
#define NMPC_SOLVE_ONCE_H

#include "rtwtypes.h"

// Satu solve NMPC: fmincon horizon N=30, terapkan U_opt[0].
// state_before_dim / state_after_dim: [v, r, x, y, psi] dimensional (sama urutan MATLAB).
extern void nmpc_solve_once(double *u_rudder_rad, double *exitflag,
                            double state_before_dim[5],
                            double state_after_dim[5]);

#endif

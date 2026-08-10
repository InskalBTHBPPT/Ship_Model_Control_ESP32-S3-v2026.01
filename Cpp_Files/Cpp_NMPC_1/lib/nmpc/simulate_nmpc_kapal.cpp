//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// simulate_nmpc_kapal.cpp
//
// Code generation for function 'simulate_nmpc_kapal'
//

// Include files
#include "simulate_nmpc_kapal.h"
#include "anonymous_function.h"
#include "fmincon.h"
#include "rt_nonfinite.h"
#include "simulate_nmpc_kapal_data.h"
#include "simulate_nmpc_kapal_initialize.h"
#include "simulate_nmpc_kapal_internal_types1.h"
#include <cmath>
#include <cstring>
#include <emmintrin.h>

// Function Declarations
static void du_constraints(double u_prev, double A[1800], double b[60]);

// Function Definitions
static void du_constraints(double u_prev, double A[1800], double b[60])
{
  std::memset(&A[0], 0, 1800U * sizeof(double));
  A[0] = 1.0;
  b[0] = u_prev + 0.087266462599716474;
  b[30] = 0.087266462599716474 - u_prev;
  for (int i{0}; i < 29; i++) {
    int A_tmp;
    int b_A_tmp;
    A_tmp = i + 60 * i;
    A[A_tmp + 1] = -1.0;
    b_A_tmp = i + 60 * (i + 1);
    A[b_A_tmp + 1] = 1.0;
    b[i + 1] = 0.087266462599716474;
    A[A_tmp + 31] = 1.0;
    A[b_A_tmp + 31] = -1.0;
    b[i + 31] = 0.087266462599716474;
  }
  A[30] = -1.0;
}

static void nondim_to_dim(const double s_nd[5], double s_dim[5])
{
  s_dim[0] = s_nd[0] * 15.4;
  s_dim[1] = s_nd[1] * 15.4 / 101.07;
  s_dim[2] = s_nd[2] * 101.07;
  s_dim[3] = s_nd[3] * 101.07;
  s_dim[4] = s_nd[4];
}

void nmpc_solve_once(double *u_rudder_rad, double *exitflag_out,
                     double state_before_dim[5], double state_after_dim[5])
{
  static const double s0_nd[5]{0.0, 0.0, 0.0, 0.98941327792618983, 0.0};
  coder::b_anonymous_function cost_fun;
  coder::anonymous_function nonlcon;
  double A_du[1800];
  double U_opt[30];
  double b_u_prev[30];
  double b_du[60];
  double dv[2];
  double exitflag;
  double u_prev;
  double x_dot_tmp;
  double b_cost_fun;
  double s_before_nd[5];

  if (!isInitialized_simulate_nmpc_kapal) {
    simulate_nmpc_kapal_initialize();
  }

  u_prev = 0.0;
  du_constraints(0.0, A_du, b_du);

  for (int i{0}; i < 5; i++) {
    cost_fun.workspace.s_nd[i] = s0_nd[i];
    s_before_nd[i] = s0_nd[i];
  }

  for (int i{0}; i <= 28; i += 2) {
    __m128d r;
    dv[0] = i;
    dv[1] = static_cast<double>(i) + 1.0;
    r = _mm_loadu_pd(&dv[0]);
    _mm_storeu_pd(
        &cost_fun.workspace.x_ref_seq[i],
        _mm_div_pd(_mm_mul_pd(_mm_add_pd(_mm_set1_pd(0.0),
                                          _mm_add_pd(_mm_set1_pd(1.0), r)),
                               _mm_set1_pd(15.4)),
                   _mm_set1_pd(101.07)));
  }

  for (int i{0}; i < 5; i++) {
    nonlcon.workspace.s_nd[i] = cost_fun.workspace.s_nd[i];
  }
  for (int i{0}; i < 30; i++) {
    b_u_prev[i] = u_prev;
  }

  coder::fmincon(cost_fun, b_u_prev, A_du, b_du, nonlcon, U_opt, exitflag);

  if (exitflag <= 0.0) {
    for (int i{0}; i < 30; i++) {
      U_opt[i] = u_prev;
    }
  }

  *u_rudder_rad = U_opt[0];
  *exitflag_out = exitflag;

  nondim_to_dim(s_before_nd, state_before_dim);

  u_prev = cost_fun.workspace.s_nd[0];
  exitflag = std::sin(cost_fun.workspace.s_nd[4]);
  x_dot_tmp = std::cos(cost_fun.workspace.s_nd[4]);
  b_cost_fun = cost_fun.workspace.s_nd[1];
  cost_fun.workspace.s_nd[0] +=
      0.15236964480063322 *
      ((-0.61373138167832486 * cost_fun.workspace.s_nd[0] +
        -0.1017805438031183 * cost_fun.workspace.s_nd[1]) +
       0.01 * U_opt[0]);
  cost_fun.workspace.s_nd[1] +=
      0.15236964480063322 *
      ((-5.0966239122212551 * u_prev +
        -3.4085423899828422 * cost_fun.workspace.s_nd[1]) +
       U_opt[0]);
  cost_fun.workspace.s_nd[2] +=
      0.15236964480063322 * (x_dot_tmp - u_prev * exitflag);
  cost_fun.workspace.s_nd[3] +=
      0.15236964480063322 * (exitflag + u_prev * x_dot_tmp);
  cost_fun.workspace.s_nd[4] += 0.15236964480063322 * b_cost_fun;

  nondim_to_dim(cost_fun.workspace.s_nd, state_after_dim);
}

double simulate_nmpc_kapal_anonFcn1(const double s_nd[5],
                                    const double x_ref_seq[30],
                                    const double U[30])
{
  static const signed char b_iv[9]{10, 0, 0, 0, 1, 0, 0, 0, 1};
  double s[5];
  double b_err[3];
  double err[3];
  double varargout_1;
  for (int i{0}; i < 5; i++) {
    s[i] = s_nd[i];
  }
  varargout_1 = 0.0;
  for (int i{0}; i < 30; i++) {
    double b_s;
    double b_x_dot_tmp;
    double c_s;
    double d;
    double d_s;
    double e_s;
    double f_s;
    double x_dot_tmp;
    //  Akhir dari fungsi utama
    //  --- FUNGSI LOKAL ---
    b_s = s[0];
    c_s = s[1];
    d = U[i];
    d_s = s[4];
    x_dot_tmp = std::sin(s[4]);
    b_x_dot_tmp = std::cos(s[4]);
    e_s = s[2];
    f_s = s[3];
    s[0] = b_s + 0.15236964480063322 *
                     ((-0.61373138167832486 * b_s + -0.1017805438031183 * c_s) +
                      0.01 * d);
    s[1] =
        c_s + 0.15236964480063322 *
                  ((-5.0966239122212551 * b_s + -3.4085423899828422 * c_s) + d);
    e_s += 0.15236964480063322 * (b_x_dot_tmp - b_s * x_dot_tmp);
    s[2] = e_s;
    b_x_dot_tmp = f_s + 0.15236964480063322 * (x_dot_tmp + b_s * b_x_dot_tmp);
    s[3] = b_x_dot_tmp;
    x_dot_tmp = d_s + 0.15236964480063322 * c_s;
    s[4] = x_dot_tmp;
    err[0] = e_s - x_ref_seq[i];
    err[1] = b_x_dot_tmp;
    err[2] = x_dot_tmp;
    std::memset(&b_err[0], 0, 3U * sizeof(double));
    e_s = 0.0;
    for (int b_i{0}; b_i < 3; b_i++) {
      b_s = ((b_err[b_i] + err[0] * static_cast<double>(b_iv[3 * b_i])) +
             b_x_dot_tmp * static_cast<double>(b_iv[3 * b_i + 1])) +
            x_dot_tmp * static_cast<double>(b_iv[3 * b_i + 2]);
      b_err[b_i] = b_s;
      e_s += b_s * err[b_i];
    }
    varargout_1 = (varargout_1 + e_s) + d * d;
  }
  return varargout_1;
}

void simulate_nmpc_kapal_anonFcn2(const double s_nd[5], const double U[30],
                                  double varargout_1[60])
{
  double s[5];
  for (int i{0}; i < 5; i++) {
    s[i] = s_nd[i];
  }
  std::memset(&varargout_1[0], 0, 60U * sizeof(double));
  //  Prealokasi kendala
  for (int i{0}; i < 30; i++) {
    double b_s;
    double c_s;
    double d;
    int varargout_1_tmp;
    //  Akhir dari fungsi utama
    //  --- FUNGSI LOKAL ---
    b_s = s[0];
    c_s = s[1];
    d = U[i];
    s[0] = b_s + 0.15236964480063322 *
                     ((-0.61373138167832486 * b_s + -0.1017805438031183 * c_s) +
                      0.01 * d);
    b_s =
        c_s + 0.15236964480063322 *
                  ((-5.0966239122212551 * b_s + -3.4085423899828422 * c_s) + d);
    s[1] = b_s;
    varargout_1_tmp = i << 1;
    varargout_1[varargout_1_tmp] = b_s - 0.61167038961038955;
    varargout_1[varargout_1_tmp + 1] = -0.61167038961038955 - b_s;
  }
}

// End of code generation (simulate_nmpc_kapal.cpp)

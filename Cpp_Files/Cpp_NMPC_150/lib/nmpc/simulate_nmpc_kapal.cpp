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
#include "mean.h"
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

void simulate_nmpc_kapal(double hist_dim[755], double history_input[151],
                         double time_vector[151], double rmse_data[3])
{
  static const double s0_nd[5]{0.0, 0.0, 0.0, 0.98941327792618983, 0.0};
  coder::b_anonymous_function cost_fun;
  double A_du[1800];
  double history_state_nd[755];
  double b_y[151];
  double c_y[151];
  double y[151];
  double b_du[60];
  double dv[2];
  double exitflag;
  double u_prev;
  double x_dot_tmp;
  if (!isInitialized_simulate_nmpc_kapal) {
    simulate_nmpc_kapal_initialize();
  }
  //  Parameter Kapal
  //  Koefisien Hidrodinamika
  //  Model Gerak Kapal
  //  Setup NMPC
  u_prev = 0.0;
  du_constraints(0.0, A_du, b_du);
  //  ========================================================
  //  PREALOKASI MEMORI UNTUK C++ (PENTING)
  //  ========================================================
  std::memset(&history_state_nd[0], 0, 755U * sizeof(double));
  for (int i{0}; i < 5; i++) {
    history_state_nd[i] = s0_nd[i];
    cost_fun.workspace.s_nd[i] = s0_nd[i];
  }
  for (int k{0}; k < 150; k++) {
    coder::anonymous_function nonlcon;
    double U_opt[30];
    double b_u_prev[30];
    double b_cost_fun;
    for (int i{0}; i <= 28; i += 2) {
      __m128d r;
      dv[0] = i;
      dv[1] = static_cast<double>(i) + 1.0;
      r = _mm_loadu_pd(&dv[0]);
      _mm_storeu_pd(
          &cost_fun.workspace.x_ref_seq[i],
          _mm_div_pd(
              _mm_mul_pd(
                  _mm_add_pd(_mm_set1_pd((static_cast<double>(k) + 1.0) - 1.0),
                             _mm_add_pd(_mm_set1_pd(1.0), r)),
                  _mm_set1_pd(15.4)),
              _mm_set1_pd(101.07)));
    }
    //  Pastikan vektor kolom (N x 1)
    for (int i{0}; i < 5; i++) {
      nonlcon.workspace.s_nd[i] = cost_fun.workspace.s_nd[i];
    }
    for (int i{0}; i < 30; i++) {
      b_u_prev[i] = u_prev;
    }
    coder::fmincon(cost_fun, b_u_prev, A_du, b_du, nonlcon, U_opt, exitflag);
    if (exitflag <= 0.0) {
      //  Menghapus warning() karena I/O string kurang direkomendasikan saat
      //  compile C++ murni
      for (int i{0}; i < 30; i++) {
        U_opt[i] = u_prev;
      }
    }
    history_input[k] = U_opt[0];
    //  Akhir dari fungsi utama
    //  --- FUNGSI LOKAL ---
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
    for (int i{0}; i < 5; i++) {
      history_state_nd[i + 5 * (k + 1)] = cost_fun.workspace.s_nd[i];
    }
    u_prev = U_opt[0];
    du_constraints(U_opt[0], A_du, b_du);
  }
  history_input[150] = history_input[149];
  //  Mengisi input terakhir
  //  Konversi State ke Dimensional
  //  Perhitungan RMSE
  for (int i{0}; i < 151; i++) {
    int hist_dim_tmp;
    time_vector[i] = i;
    hist_dim[5 * i] = history_state_nd[5 * i] * 15.4;
    hist_dim_tmp = 5 * i + 1;
    hist_dim[hist_dim_tmp] = history_state_nd[hist_dim_tmp] * 15.4 / 101.07;
    hist_dim_tmp = 5 * i + 2;
    u_prev = history_state_nd[hist_dim_tmp] * 101.07;
    hist_dim[hist_dim_tmp] = u_prev;
    hist_dim_tmp = 5 * i + 3;
    exitflag = history_state_nd[hist_dim_tmp] * 101.07;
    hist_dim[hist_dim_tmp] = exitflag;
    hist_dim_tmp = 5 * i + 4;
    x_dot_tmp = history_state_nd[hist_dim_tmp];
    hist_dim[hist_dim_tmp] = x_dot_tmp;
    u_prev -= 15.4 * static_cast<double>(i);
    y[i] = u_prev * u_prev;
    b_y[i] = exitflag * exitflag;
    c_y[i] = x_dot_tmp * x_dot_tmp;
  }
  rmse_data[0] = std::sqrt(coder::mean(y));
  rmse_data[1] = std::sqrt(coder::mean(b_y));
  rmse_data[2] = std::sqrt(coder::mean(c_y));
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

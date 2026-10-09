/**
 * @file    nmpc_kapal_waypoint.h
 * @brief   Embedded (allocation-free) NMPC waypoint-tracking controller for a
 *          surface vessel, using the 11-parameter WyNDA ship model.
 *
 *          fmincon/SQP is replaced with a static projected-gradient + quadratic-penalty solver
 *          see the "SOLVER NOTE" in nmpc_kapal_waypoint.c for details/tradeoffs.
 *
 * @note    No dynamic memory allocation. All buffers are static/stack.
 */
#ifndef NMPC_KAPAL_WAYPOINT_H
#define NMPC_KAPAL_WAYPOINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------------ */
/* Compile-time capacity limits (caller's N must be <= these)               */
/* ------------------------------------------------------------------------ */
#define NMPC_MAX_HORIZON   30u   /* max prediction horizon N               */
#define NMPC_NUM_THETA     11u   /* WyNDA basis parameter count            */
#define NMPC_NUM_STATES    5u    /* [v, r, x, y, psi] (nondimensional)     */

/* ------------------------------------------------------------------------ */
/* Types                                                                    */
/* ------------------------------------------------------------------------ */

/** Nondimensional ship state: v'=sway, r'=yaw rate, x'/y'=position, psi=heading (rad) */
typedef struct {
    double v;
    double r;
    double x;
    double y;
    double psi;
} NMPC_ShipState_t;

/** Controller / solver configuration. Populate once via NMPC_InitDefaultConfig(). */
typedef struct {
    double   L;                          /* ship length [m]                        */
    double   theta[NMPC_NUM_THETA];      /* WyNDA identified parameter vector      */

    double   Qx;                         /* cost weight: x tracking error          */
    double   Qy;                         /* cost weight: y tracking error          */
    double   Qpsi;                       /* cost weight: heading tracking error    */
    double   R;                          /* cost weight: rudder effort penalty     */

    double   u_limit_rad;                /* rudder angle bound  (+/-)  [rad]       */
    double   du_max_rad;                 /* rudder rate-of-change bound [rad/step] */
    double   r_limit_nd;                 /* nondim. yaw-rate bound (caller computes
                                             this per-step: deg2rad(45)*L/u0)       */

    uint32_t max_iter;                   /* solver iterations                      */
    double   step_size;                  /* gradient-descent base step size        */
    double   penalty_rho;                /* quadratic penalty weight for yaw limit */
} NMPC_Config_t;

/**
 * @brief Populate a config with the same numeric defaults as the MATLAB script
 *        (Q = diag([10,10,10]), R = 1, +/-45 deg rudder, +/-30 deg/step rate).
 *
 * @param cfg   [out] config to initialize
 * @param L     ship length [m]
 */
void NMPC_InitDefaultConfig(NMPC_Config_t *cfg, double L);

/**
 * @brief One-step NMPC solve.
 *
 * Caller allocates all reference-sequence arrays with length >= N
 * (N <= NMPC_MAX_HORIZON).
 *
 * @param cfg               [in]  controller configuration (theta, weights, limits)
 * @param current_state_nd  [in]  current nondimensional state [v,r,x,y,psi]
 * @param u_prev             [in]  previous rudder command [rad]
 * @param u0_speed           [in]  nominal surge speed u0 [m/s]
 * @param x_ref_seq          [in]  reference X sequence, length N (nondimensional)
 * @param y_ref_seq          [in]  reference Y sequence, length N (nondimensional)
 * @param psi_ref_seq        [in]  reference heading sequence, length N [rad]
 * @param N                  [in]  horizon length, 1 <= N <= NMPC_MAX_HORIZON
 * @param u_opt              [out] optimal rudder command for this step [rad]
 *
 * @return  1  on success (constraints satisfied within tolerance)
 *          0  on soft failure (solver ran but constraints violated; u_opt
 *             falls back to u_prev, matching MATLAB's exitflag<=0 branch)
 *         -1  on invalid input (N == 0 or N > NMPC_MAX_HORIZON)
 */
int32_t NMPC_Solve(const NMPC_Config_t   *cfg,
                    const double          current_state_nd[NMPC_NUM_STATES],
                    double                u_prev,
                    double                u0_speed,
                    const double         *x_ref_seq,
                    const double         *y_ref_seq,
                    const double         *psi_ref_seq,
                    uint32_t              N,
                    double               *u_opt);

/**
 * @brief Propagate the WyNDA ship model one Euler step (plant or predictor use).
 *
 * @param s      [in]  current state [v,r,x,y,psi]
 * @param delta  [in]  rudder angle [rad]
 * @param theta  [in]  WyNDA parameter vector (length NMPC_NUM_THETA)
 * @param s_next [out] next state [v,r,x,y,psi]
 */
void NMPC_EulerStep(const double s[NMPC_NUM_STATES],
                     double       delta,
                     const double theta[NMPC_NUM_THETA],
                     double       s_next[NMPC_NUM_STATES]);

#ifdef __cplusplus
}
#endif

#endif /* NMPC_KAPAL_WAYPOINT_H */

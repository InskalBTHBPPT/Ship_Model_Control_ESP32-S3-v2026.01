/**
 * @file    nmpc_kapal_waypoint.c
 * @brief   Implementation of the WyNDA-model NMPC waypoint tracker.
 *
 * SOLVER NOTE
 * -----------
 * The MATLAB reference uses fmincon('sqp') to solve a constrained nonlinear
 * program each control step. A full SQP implementation (with a QP sub-solver,
 * Hessian approximation, and an active-set / interior-point step) is heavy
 * for a real-time embedded target and pulls in dynamic memory + matrix
 * factorization.
 *
 * This port instead uses a **static projected-gradient descent with a
 * quadratic penalty** for the nonlinear yaw-rate constraint:
 *   - Box constraints (rudder angle limits)      -> exact projection (clamp)
 *   - Linear rate constraints (du/dt limits)      -> sequential clamp projection
 *   - Nonlinear constraint (yaw-rate limit)       -> quadratic penalty added
 *                                                     to the cost gradient
 *
 * This is a common, deterministic, allocation-free substitute for embedded
 * NMPC when a full QP/SQP library (e.g. qpOASES, OSQP, a code-generated
 * ACADO/CVXGEN solver) is not available on target. It trades solve accuracy
 * for determinism and zero dynamic memory. If your application needs
 * fmincon-equivalent optimality, replace NMPC_Solve()'s inner loop with a
 * call into a certified embedded QP/SQP solver, reusing NMPC_ComputeAugmentedCost()
 * and NMPC_EulerStep() as the model/cost building blocks.
 *
 * INDEXING NOTE
 * -------------
 * MATLAB arrays are 1-based; all C arrays here are 0-based. Anywhere the
 * MATLAB code reads U(1), U(i), x_ref_seq(h), etc., the C code below reads
 * U[0], U[i-1], x_ref_seq[h-1], etc.
 */

#include "nmpc_kapal_waypoint.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------------ */
/* Internal tunables (mirroring the MATLAB script's literal constants)      */
/* ------------------------------------------------------------------------ */
#define NMPC_FD_EPS          1.0e-6   /* finite-difference step for gradient */
#define NMPC_FEAS_TOL         1.0e-6  /* constraint feasibility tolerance    */

/* ========================================================================
 * Public API
 * ======================================================================== */

void NMPC_InitDefaultConfig(NMPC_Config_t *cfg, double L)
{
    static const double theta_default[NMPC_NUM_THETA] = {
        -9.2816e-01,  /* theta_1  : v pada v_dot        */
        -2.6644e-01,  /* theta_2  : r pada v_dot        */
         1.2074e-01,  /* theta_3  : delta pada v_dot    */
         2.6348e-03,  /* theta_4  : v pada r_dot        */
        -1.0577e-02,  /* theta_5  : r pada r_dot        */
        -1.3502e-02,  /* theta_6  : delta pada r_dot    */
         5.8118e-02,  /* theta_7  : u0*cos(psi)         */
         1.4903e-03,  /* theta_8  : v*sin(psi)          */
         4.7426e-02,  /* theta_9  : u0*sin(psi)         */
        -4.6814e-03,  /* theta_10 : v*cos(psi)          */
         4.5806e-02   /* theta_11 : r pada psi_dot      */
    };

    if (cfg == NULL) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));

    cfg->L = L;
    memcpy(cfg->theta, theta_default, sizeof(theta_default));

    /* Q = diag([10, 10, 10]); R = 1; (from MATLAB) */
    cfg->Qx   = 10.0;
    cfg->Qy   = 10.0;
    cfg->Qpsi = 10.0;
    cfg->R    = 1.0;

    /* +/-45 deg rudder, +/-30 deg/step rate (from MATLAB) */
    cfg->u_limit_rad  = 45.0 * (M_PI / 180.0);
    cfg->du_max_rad   = 30.0 * (M_PI / 180.0);
    cfg->r_limit_nd   = 0.0;  /* caller must set per-step: see NMPC_Solve header */

    /* Solver tuning (embedded projected-gradient substitute for fmincon/sqp) */
    cfg->max_iter     = 150u;
    cfg->step_size    = 5.0e-3;
    cfg->penalty_rho  = 1.0e4;
}

void NMPC_EulerStep(const double s[NMPC_NUM_STATES],
                     double       delta,
                     const double theta[NMPC_NUM_THETA],
                     double       s_next[NMPC_NUM_STATES])
{
    const double v   = s[0];
    const double r   = s[1];
    const double psi = s[4];

    const double v_dot   = theta[0] * v + theta[1] * r + theta[2] * delta;
    const double r_dot   = theta[3] * v + theta[4] * r + theta[5] * delta;
    const double x_dot   = theta[6] * cos(psi) - theta[7] * v * sin(psi);
    const double y_dot   = theta[8] * sin(psi) + theta[9] * v * cos(psi);
    const double psi_dot = theta[10] * r;

    s_next[0] = s[0] + v_dot;
    s_next[1] = s[1] + r_dot;
    s_next[2] = s[2] + x_dot;
    s_next[3] = s[3] + y_dot;
    s_next[4] = s[4] + psi_dot;
}

/* ========================================================================
 * Internal helpers
 * ======================================================================== */

/**
 * @brief Roll the model forward over the whole horizon and accumulate the
 *        tracking + effort cost, plus a quadratic penalty for any yaw-rate
 *        constraint violation (soft substitute for fmincon's nonlcon).
 *
 * Equivalent to MATLAB's mpc_cost() + state_constraints() combined.
 */
static double NMPC_ComputeAugmentedCost(const NMPC_Config_t *cfg,
                                         const double          s0[NMPC_NUM_STATES],
                                         const double         *U,
                                         const double         *x_ref_seq,
                                         const double         *y_ref_seq,
                                         const double         *psi_ref_seq,
                                         uint32_t              N)
{
    double s[NMPC_NUM_STATES];
    double s_next[NMPC_NUM_STATES];
    double J = 0.0;
    uint32_t i;

    memcpy(s, s0, sizeof(s));

    for (i = 0; i < N; ++i) {
        const double u = U[i];

        NMPC_EulerStep(s, u, cfg->theta, s_next);
        memcpy(s, s_next, sizeof(s));

        {
            const double dpsi     = s[4] - psi_ref_seq[i];
            const double psi_err  = atan2(sin(dpsi), cos(dpsi)); /* shortest angular distance */
            const double ex       = s[2] - x_ref_seq[i];
            const double ey       = s[3] - y_ref_seq[i];

            J += cfg->Qx   * ex      * ex;
            J += cfg->Qy   * ey      * ey;
            J += cfg->Qpsi * psi_err * psi_err;
            J += cfg->R    * u       * u;
        }

        /* Soft (penalty) form of state_constraints(): r_limit_nd bound on yaw rate */
        {
            const double r_state   = s[1];
            const double viol_hi   = r_state - cfg->r_limit_nd;      /* <= 0 desired */
            const double viol_lo   = -cfg->r_limit_nd - r_state;     /* <= 0 desired */

            if (viol_hi > 0.0) {
                J += cfg->penalty_rho * viol_hi * viol_hi;
            }
            if (viol_lo > 0.0) {
                J += cfg->penalty_rho * viol_lo * viol_lo;
            }
        }
    }

    return J;
}

/**
 * @brief Central-difference gradient of NMPC_ComputeAugmentedCost() w.r.t. U.
 */
static void NMPC_NumericalGradient(const NMPC_Config_t *cfg,
                                    const double          s0[NMPC_NUM_STATES],
                                    double               *U,
                                    const double         *x_ref_seq,
                                    const double         *y_ref_seq,
                                    const double         *psi_ref_seq,
                                    uint32_t              N,
                                    double               *grad_out)
{
    uint32_t i;

    for (i = 0; i < N; ++i) {
        const double orig = U[i];
        double j_plus, j_minus;

        U[i] = orig + NMPC_FD_EPS;
        j_plus = NMPC_ComputeAugmentedCost(cfg, s0, U, x_ref_seq, y_ref_seq, psi_ref_seq, N);

        U[i] = orig - NMPC_FD_EPS;
        j_minus = NMPC_ComputeAugmentedCost(cfg, s0, U, x_ref_seq, y_ref_seq, psi_ref_seq, N);

        U[i] = orig;
        grad_out[i] = (j_plus - j_minus) / (2.0 * NMPC_FD_EPS);
    }
}

/**
 * @brief Project U onto the feasible set for box + rate-of-change constraints.
 *
 * Equivalent to MATLAB's lb/ub box bounds combined with du_constraints().
 * Uses a sequential clamp (not an exact polytope projection, but exact for
 * the box, and exact for each individual rate link when applied in order),
 * which is standard practice for lightweight embedded NMPC.
 */
static void NMPC_ProjectFeasible(const NMPC_Config_t *cfg,
                                  double               *U,
                                  double                u_prev,
                                  uint32_t              N)
{
    uint32_t i;
    double prev = u_prev;

    for (i = 0; i < N; ++i) {
        /* Rate-of-change bound relative to previous (accepted) command */
        double lo_rate = prev - cfg->du_max_rad;
        double hi_rate = prev + cfg->du_max_rad;

        /* Box bound */
        double lo = (lo_rate > -cfg->u_limit_rad) ? lo_rate : -cfg->u_limit_rad;
        double hi = (hi_rate <  cfg->u_limit_rad) ? hi_rate :  cfg->u_limit_rad;

        if (U[i] < lo) {
            U[i] = lo;
        } else if (U[i] > hi) {
            U[i] = hi;
        }

        prev = U[i];
    }
}

/**
 * @brief Check whether U (given u_prev) satisfies box, rate, and (rolled-out)
 *        yaw-rate constraints within tolerance. Mirrors fmincon's success
 *        criterion at a coarse level.
 */
static int32_t NMPC_CheckFeasible(const NMPC_Config_t *cfg,
                                   const double          s0[NMPC_NUM_STATES],
                                   const double         *U,
                                   double                u_prev,
                                   uint32_t              N)
{
    double s[NMPC_NUM_STATES];
    double s_next[NMPC_NUM_STATES];
    double prev = u_prev;
    uint32_t i;

    memcpy(s, s0, sizeof(s));

    for (i = 0; i < N; ++i) {
        const double u = U[i];

        if (u < -cfg->u_limit_rad - NMPC_FEAS_TOL || u > cfg->u_limit_rad + NMPC_FEAS_TOL) {
            return 0;
        }
        if (fabs(u - prev) > cfg->du_max_rad + NMPC_FEAS_TOL) {
            return 0;
        }
        prev = u;

        NMPC_EulerStep(s, u, cfg->theta, s_next);
        memcpy(s, s_next, sizeof(s));

        if (fabs(s[1]) > cfg->r_limit_nd + NMPC_FEAS_TOL) {
            return 0;
        }
    }

    return 1;
}

/* ========================================================================
 * Public solve entry point
 * ======================================================================== */

int32_t NMPC_Solve(const NMPC_Config_t *cfg,
                    const double          current_state_nd[NMPC_NUM_STATES],
                    double                u_prev,
                    double                u0_speed,
                    const double         *x_ref_seq,
                    const double         *y_ref_seq,
                    const double         *psi_ref_seq,
                    uint32_t              N,
                    double               *u_opt)
{
    double U[NMPC_MAX_HORIZON];
    double grad[NMPC_MAX_HORIZON];
    uint32_t iter;
    uint32_t i;

    (void)u0_speed; /* kept in signature for interface parity with the MATLAB
                        function; r_limit_nd must be precomputed by the caller
                        into cfg->r_limit_nd = deg2rad(45)*L/u0, matching the
                        MATLAB script's per-call r_limit_nd derivation.      */

    if (cfg == NULL || current_state_nd == NULL || x_ref_seq == NULL ||
        y_ref_seq == NULL || psi_ref_seq == NULL || u_opt == NULL) {
        return -1;
    }
    if (N == 0u || N > NMPC_MAX_HORIZON) {
        return -1;
    }

    /* U0 = u_prev * ones(N,1);  (warm start, matches MATLAB) */
    for (i = 0; i < N; ++i) {
        U[i] = u_prev;
    }

    for (iter = 0; iter < cfg->max_iter; ++iter) {
        const double decay = 1.0 / (1.0 + 0.02 * (double)iter); /* diminishing step */

        NMPC_NumericalGradient(cfg, current_state_nd, U,
                                x_ref_seq, y_ref_seq, psi_ref_seq, N, grad);

        for (i = 0; i < N; ++i) {
            U[i] -= cfg->step_size * decay * grad[i];
        }

        NMPC_ProjectFeasible(cfg, U, u_prev, N);
    }

    if (NMPC_CheckFeasible(cfg, current_state_nd, U, u_prev, N)) {
        *u_opt = U[0];
        return 1;
    } else {
        /* Mirrors: if exitflag <= 0, u_opt = u_prev; */
        *u_opt = u_prev;
        return 0;
    }
}

/**
 * @file    main.c
 * @brief   Full multi-waypoint NMPC mission example :
 *            1. Define a home Lat/Lon origin and a sequence of Lat/Lon waypoints
 *            2. Convert them to local ENU meters (geo_enu.c)
 *            3. Sequentially track them with a switching radius (waypoint_manager.c)
 *            4. Solve one-step NMPC each control tick (nmpc_kapal_waypoint.c)
 *            5. Step the WyNDA plant model forward and log Lat/Lon + ENU
 *
 * Build:
 *   gcc -std=c99 -O2 -Wall -Wextra -o nmpc_demo \
 *       main.c nmpc_kapal_waypoint.c geo_enu.c waypoint_manager.c -lm
 */

#include <stdio.h>
#include <math.h>

#include "nmpc_kapal_waypoint.h"
#include "geo_enu.h"
#include "waypoint_manager.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double NormalizeAngle(double inp) {
    if(inp < 0.0f) return 360.0f + inp;
    else if(inp > 360.0f) return inp - 360.0f;
    else return inp;
}

int main(void)
{
    /* ---------------------------------------------------------------- */
    /* 1. Ship model / mission constants (mirrors run_nmpc.m Section 1) */
    /* ---------------------------------------------------------------- */
    const double L        = 1.0107;   /* ship length [m]            */
    const double u0_speed = 0.6114;   /* nominal surge speed [m/s]  */
    const double T_sim    = 0.10;     /* sample time [s], 10 Hz     */
    const double T_final  = 400.0;    /* max mission duration [s]   */
    const uint32_t N_STEPS_MAX = (uint32_t)(T_final / T_sim);
    const uint32_t N      = 20u;      /* prediction horizon          */
    const double r_tran   = 3.0;      /* switching radius [m]        */

    /* ---------------------------------------------------------------- */
    /* 2. Home origin + waypoints in Lat/Lon (mirrors run_nmpc.m Sec. 2) */
    /* ---------------------------------------------------------------- */
    const double lat0 = -7.28715;
    const double lon0 = 112.79600;
    const double heading0_deg = 90.0;

    const double wp_lat[] = { -7.28675, -7.28630, -7.28675, -7.28715 };
    const double wp_lon[] = { 112.79600, 112.79600, 112.79610, 112.79600 };
    const uint32_t num_waypoints = (uint32_t)(sizeof(wp_lat) / sizeof(wp_lat[0]));

    double wp_east[WP_MAX_WAYPOINTS];
    double wp_north[WP_MAX_WAYPOINTS];
    uint32_t i;

    /* ---------------------------------------------------------------- */
    /* 3. Convert Lat/Lon waypoints -> ENU meters (mirrors Sec. 3)      */
    /* ---------------------------------------------------------------- */
    printf("[INIT] Home Point Locked at Lat: %.6f, Lon: %.6f\n", lat0, lon0);
    printf("[INIT] %u Waypoints converted to ENU:\n", num_waypoints);

    for (i = 0; i < num_waypoints; ++i) {
        GEO_LLA2ENU(lat0, lon0, wp_lat[i], wp_lon[i],
                    GEO_EARTH_RADIUS_M, &wp_east[i], &wp_north[i]);
        printf("  -> WP %u: East (X) = %7.2f m, North (Y) = %7.2f m\n",
               i + 1u, wp_east[i], wp_north[i]);
    }

    /* ---------------------------------------------------------------- */
    /* 4. Setup waypoint manager, NMPC config, initial state             */
    /* ---------------------------------------------------------------- */
    WP_Manager_t wp_mgr;
    if (!WP_Init(&wp_mgr, wp_east, wp_north, num_waypoints, r_tran)) {
        fprintf(stderr, "WP_Init failed\n");
        return 1;
    }

    NMPC_Config_t cfg;
    NMPC_InitDefaultConfig(&cfg, L);

    NMPC_ShipState_t state = { .v = 0.0, .r = 0.0, .x = 0.0, .y = 0.0,
                                .psi = heading0_deg * (M_PI / 180.0) };
    double u_prev = 0.0;

    printf("\n[RUN] Starting NMPC waypoint-tracking mission...\n");
    printf("step,   t[s],   x[m],    y[m],  active_wp,   lat,       lon,       psi[deg],     delta[deg]\n");

    /* ---------------------------------------------------------------- */
    /* 5. Main mission loop (mirrors run_nmpc.m Section 6)               */
    /* ---------------------------------------------------------------- */
    uint32_t k;
    for (k = 0; k < N_STEPS_MAX; ++k) {
        /* Ship position in dimensional ENU meters */
        const double x_ship = state.x * L;
        const double y_ship = state.y * L;

        /* Ship position in Lat/Lon (for logging / operator display) */
        double lat_ship, lon_ship;
        GEO_ENU2LLA(lat0, lon0, x_ship, y_ship, GEO_EARTH_RADIUS_M,
                    &lat_ship, &lon_ship);

        /* Update active waypoint (switches on entering r_tran) */
        double dist_to_wp;
        const int32_t switched = WP_UpdateActive(&wp_mgr, x_ship, y_ship, &dist_to_wp);

        if (switched) {
            printf("[STEP %5u | t=%6.1f s] Entered r_tran (%.1f m) -> switched to WP %u\n",
                   k, k * T_sim, r_tran, wp_mgr.active_idx + 1u);
        }

        double wp_target_east, wp_target_north;
        WP_GetActiveTarget(&wp_mgr, &wp_target_east, &wp_target_north);

        /* Target bearing toward the active waypoint */
        const double theta_target = atan2(wp_target_north - y_ship, wp_target_east - x_ship);

        /* Build N-step reference horizon (straight line at nominal speed) */
        double x_ref_seq[NMPC_MAX_HORIZON];
        double y_ref_seq[NMPC_MAX_HORIZON];
        double psi_ref_seq[NMPC_MAX_HORIZON];
        uint32_t h;

        for (h = 0; h < N; ++h) {
            const double dist_step = (double)(h + 1) * u0_speed * T_sim;
            const double x_ref_dim = x_ship + dist_step * cos(theta_target);
            const double y_ref_dim = y_ship + dist_step * sin(theta_target);

            x_ref_seq[h]   = x_ref_dim / L;
            y_ref_seq[h]   = y_ref_dim / L;
            psi_ref_seq[h] = theta_target;
        }

        /* Per-step nondimensional yaw-rate bound (depends on u0_speed) */
        cfg.r_limit_nd = (45.0 * (M_PI / 180.0)) * (L / u0_speed);

        double s_nd[NMPC_NUM_STATES]      = { state.v, state.r, state.x, state.y, state.psi };
        double s_next_nd[NMPC_NUM_STATES];
        double u_opt;

        const int32_t status = NMPC_Solve(&cfg, s_nd, u_prev, u0_speed,
                                           x_ref_seq, y_ref_seq, psi_ref_seq, N, &u_opt);
        if (status < 0) {
            fprintf(stderr, "NMPC_Solve: invalid input at step %u\n", k);
            return 1;
        }

        if ((k % 20u) == 0u) { /* print every ~2s to keep output readable */
            printf("%5u, %6.1f, %7.2f, %7.2f,    WP%u,   %9.6f, %10.6f,   %6.2f,   %6.2f\n",
                   k, k * T_sim, x_ship, y_ship, wp_mgr.active_idx + 1u,
                   lat_ship, lon_ship, state.psi * (180.0 / M_PI), u_opt * (180.0 / M_PI));
        }

        /* Mission-complete check: within r_tran of the LAST waypoint */
        if (WP_IsMissionComplete(&wp_mgr, dist_to_wp)) {
            printf("\n[GOAL] Reached final waypoint (WP %u)\n",
                   num_waypoints);
            printf("[GOAL] Mission complete at step %u (t = %.1f s)\n", k, k * T_sim);
            break;
        }

        /* Advance the plant one Euler step (WyNDA model) */
        NMPC_EulerStep(s_nd, u_opt, cfg.theta, s_next_nd);
        state.v   = s_next_nd[0];
        state.r   = s_next_nd[1];
        state.x   = s_next_nd[2];
        state.y   = s_next_nd[3];
        state.psi = s_next_nd[4];

        u_prev = u_opt;
    }

    if (k >= N_STEPS_MAX) {
        printf("\n[TIMEOUT] Mission did not complete within %.0f s (%u steps)\n",
               T_final, N_STEPS_MAX);
    }

    return 0;
}

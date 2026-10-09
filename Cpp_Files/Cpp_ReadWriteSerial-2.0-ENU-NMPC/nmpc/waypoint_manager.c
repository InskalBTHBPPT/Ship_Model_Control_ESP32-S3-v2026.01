/**
 * @file    waypoint_manager.c
 * @brief   Implementation of the sequential multi-waypoint manager.
 */

#include "waypoint_manager.h"
#include <math.h>
#include <string.h>
#include <stddef.h>

int32_t WP_Init(WP_Manager_t *mgr,
                 const double *east_m,
                 const double *north_m,
                 uint32_t      count,
                 double        switch_radius_m)
{
    uint32_t i;

    if (mgr == NULL || east_m == NULL || north_m == NULL) {
        return 0;
    }
    if (count == 0u || count > WP_MAX_WAYPOINTS) {
        return 0;
    }

    for (i = 0; i < count; ++i) {
        mgr->east_m[i]  = east_m[i];
        mgr->north_m[i] = north_m[i];
    }

    mgr->count            = count;
    mgr->active_idx        = 0u;
    mgr->switch_radius_m   = switch_radius_m;

    return 1;
}

void WP_GetActiveTarget(const WP_Manager_t *mgr, double *east_out, double *north_out)
{
    if (mgr == NULL || east_out == NULL || north_out == NULL) {
        return;
    }
    *east_out  = mgr->east_m[mgr->active_idx];
    *north_out = mgr->north_m[mgr->active_idx];
}

int32_t WP_UpdateActive(WP_Manager_t *mgr,
                         double        ship_east_m,
                         double        ship_north_m,
                         double       *dist_out)
{
    double dx, dy, dist;
    int32_t switched = 0;

    if (mgr == NULL || dist_out == NULL) {
        return 0;
    }

    dx   = mgr->east_m[mgr->active_idx]  - ship_east_m;
    dy   = mgr->north_m[mgr->active_idx] - ship_north_m;
    dist = hypot(dx, dy);

    /* Mirrors:
     *   if dist_to_wp <= r_tran && active_wp_idx < num_waypoints
     *       active_wp_idx = active_wp_idx + 1;
     *       ... recompute wp_target / dist_to_wp
     *   end                                                          */
    if (dist <= mgr->switch_radius_m && (mgr->active_idx + 1u) < mgr->count) {
        mgr->active_idx += 1u;

        dx   = mgr->east_m[mgr->active_idx]  - ship_east_m;
        dy   = mgr->north_m[mgr->active_idx] - ship_north_m;
        dist = hypot(dx, dy);

        switched = 1;
    }

    *dist_out = dist;
    return switched;
}

int32_t WP_IsMissionComplete(const WP_Manager_t *mgr, double dist_to_active)
{
    if (mgr == NULL) {
        return 0;
    }

    return ((mgr->active_idx + 1u) == mgr->count &&
            dist_to_active <= mgr->switch_radius_m) ? 1 : 0;
}

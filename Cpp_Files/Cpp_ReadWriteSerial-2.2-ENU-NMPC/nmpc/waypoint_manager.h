/**
 * @file    waypoint_manager.h
 * @brief   Sequential multi-waypoint manager with switching-radius (r_tran)
 *          logic.
 *
 *          The ship tracks waypoint[active_idx]. Once it comes within
 *          switch_radius_m of that waypoint (and it isn't the last one),
 *          the manager advances to the next waypoint. When the ship comes
 *          within switch_radius_m of the LAST waypoint, the mission is
 *          reported complete.
 *
 * @note    Statically sized (WP_MAX_WAYPOINTS), no dynamic memory.
 */
#ifndef WAYPOINT_MANAGER_H
#define WAYPOINT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define WP_MAX_WAYPOINTS   20u

typedef struct {
    double   east_m[WP_MAX_WAYPOINTS];   /* waypoint East (ENU) coordinates [m]  */
    double   north_m[WP_MAX_WAYPOINTS];  /* waypoint North (ENU) coordinates [m] */
    uint32_t count;                      /* number of waypoints loaded            */
    uint32_t active_idx;                 /* 0-based index of current target       */
    double   switch_radius_m;            /* r_tran: switching radius [m]          */
} WP_Manager_t;

/**
 * @brief Initialize the manager with a sequence of ENU waypoints.
 *
 * @param mgr               [out] manager to initialize
 * @param east_m            [in]  array of East coordinates, length >= count
 * @param north_m           [in]  array of North coordinates, length >= count
 * @param count              [in]  number of waypoints (1 <= count <= WP_MAX_WAYPOINTS)
 * @param switch_radius_m    [in]  r_tran switching radius [m]
 *
 * @return 1 on success, 0 on invalid input (count == 0 or count > WP_MAX_WAYPOINTS)
 */
int32_t WP_Init(WP_Manager_t *mgr,
                 const double *east_m,
                 const double *north_m,
                 uint32_t      count,
                 double        switch_radius_m);

/**
 * @brief Get the currently active target waypoint.
 *
 * @param mgr        [in]  manager
 * @param east_out   [out] active target East [m]
 * @param north_out  [out] active target North [m]
 */
void WP_GetActiveTarget(const WP_Manager_t *mgr, double *east_out, double *north_out);

/**
 * @brief Update the active waypoint based on current ship ENU position.
 *
 * If the ship is within switch_radius_m of the current (non-final) active
 * waypoint, advances active_idx to the next waypoint and
 * recomputes distance to the new target.
 *
 * @param mgr            [in,out] manager (active_idx may advance)
 * @param ship_east_m    [in]  ship East position [m]
 * @param ship_north_m   [in]  ship North position [m]
 * @param dist_out       [out] distance from ship to the (possibly new) active
 *                             target [m]
 *
 * @return 1 if the active waypoint advanced this call, 0 otherwise
 */
int32_t WP_UpdateActive(WP_Manager_t *mgr,
                         double        ship_east_m,
                         double        ship_north_m,
                         double       *dist_out);

/**
 * @brief Mission-complete check: true once the ship is within
 *        switch_radius_m of the LAST waypoint in the sequence.
 *
 * @param mgr            [in] manager
 * @param dist_to_active  [in] distance to the active target, e.g. from
 *                             WP_UpdateActive()'s dist_out
 *
 * @return 1 if the mission is complete, 0 otherwise
 */
int32_t WP_IsMissionComplete(const WP_Manager_t *mgr, double dist_to_active);

#ifdef __cplusplus
}
#endif

#endif /* WAYPOINT_MANAGER_H */

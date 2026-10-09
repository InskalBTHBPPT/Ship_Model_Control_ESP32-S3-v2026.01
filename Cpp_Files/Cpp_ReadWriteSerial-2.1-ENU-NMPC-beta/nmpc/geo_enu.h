/**
 * @file    geo_enu.h
 * @brief   Latitude/Longitude <-> local ENU (East-North-Up) conversion.
 *
 * Implements the same equirectangular (flat-earth) approximation used in
 * run_nmpc.m:
 *
 *   dLat   = deg2rad(lat - lat0)
 *   dLon   = deg2rad(lon - lon0)
 *   north  = R_earth * dLat
 *   east   = R_earth * cos(deg2rad(lat0)) * dLon
 *
 * and its inverse. This is accurate for small-area operations (test ponds,
 * harbors, local USV trials on the order of kilometers) where the local
 * ground track can be treated as planar and the Earth's radius of curvature
 * along the ship's operating area is approximately constant. It is NOT a
 * geodetic-grade transform (no ellipsoid model, no WGS84 flattening); for
 * large-area or high-precision navigation use ECEF and a proper WGS84
 * ellipsoidal ENU transform instead.
 */
#ifndef GEO_ENU_H
#define GEO_ENU_H

#ifdef __cplusplus
extern "C" {
#endif

/** Mean Earth radius [m] **/
#define GEO_EARTH_RADIUS_M   6371000.0

/**
 * @brief Convert a Lat/Lon point to local ENU meters relative to a reference
 *        (home) Lat/Lon origin.
 *
 * @param lat0_deg          [in]  reference (origin) latitude  [deg]
 * @param lon0_deg          [in]  reference (origin) longitude [deg]
 * @param lat_deg           [in]  target latitude  [deg]
 * @param lon_deg           [in]  target longitude [deg]
 * @param earth_radius_m    [in]  Earth radius to use [m] (GEO_EARTH_RADIUS_M is typical)
 * @param east_m            [out] East offset from origin [m]
 * @param north_m           [out] North offset from origin [m]
 */
void GEO_LLA2ENU(double lat0_deg, double lon0_deg,
                  double lat_deg,  double lon_deg,
                  double earth_radius_m,
                  double *east_m, double *north_m);

/**
 * @brief Inverse of GEO_LLA2ENU(): convert local ENU meters back to Lat/Lon.
 *
 * @param lat0_deg          [in]  reference (origin) latitude  [deg]
 * @param lon0_deg          [in]  reference (origin) longitude [deg]
 * @param east_m            [in]  East offset from origin [m]
 * @param north_m           [in]  North offset from origin [m]
 * @param earth_radius_m    [in]  Earth radius to use [m] (GEO_EARTH_RADIUS_M is typical)
 * @param lat_deg           [out] resulting latitude  [deg]
 * @param lon_deg           [out] resulting longitude [deg]
 */
void GEO_ENU2LLA(double lat0_deg, double lon0_deg,
                  double east_m,  double north_m,
                  double earth_radius_m,
                  double *lat_deg, double *lon_deg);

#ifdef __cplusplus
}
#endif

#endif /* GEO_ENU_H */

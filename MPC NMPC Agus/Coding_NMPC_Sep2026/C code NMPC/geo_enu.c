/**
 * @file    geo_enu.c
 * @brief   Implementation of GEO_LLA2ENU / GEO_ENU2LLA.
 */

#include "geo_enu.h"
#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x)  ((x) * (M_PI / 180.0))
#define RAD2DEG(x)  ((x) * (180.0 / M_PI))

void GEO_LLA2ENU(double lat0_deg, double lon0_deg,
                  double lat_deg,  double lon_deg,
                  double earth_radius_m,
                  double *east_m, double *north_m)
{
    const double dLat = DEG2RAD(lat_deg - lat0_deg);
    const double dLon = DEG2RAD(lon_deg - lon0_deg);

    if (east_m == NULL || north_m == NULL) {
        return;
    }

    *north_m = earth_radius_m * dLat;
    *east_m  = earth_radius_m * cos(DEG2RAD(lat0_deg)) * dLon;
}

void GEO_ENU2LLA(double lat0_deg, double lon0_deg,
                  double east_m,  double north_m,
                  double earth_radius_m,
                  double *lat_deg, double *lon_deg)
{
    if (lat_deg == NULL || lon_deg == NULL) {
        return;
    }

    *lat_deg = lat0_deg + RAD2DEG(north_m / earth_radius_m);
    *lon_deg = lon0_deg + RAD2DEG(east_m / (earth_radius_m * cos(DEG2RAD(lat0_deg))));
}

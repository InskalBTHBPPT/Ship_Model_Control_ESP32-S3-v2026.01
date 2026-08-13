#pragma once

#include <cmath>
#include <optional>
#include <sstream>
#include <string>

enum class LocalFrameKind { Enu, Ned };

// Ubah konstanta ini per folder: ENU-beta = Enu, NED-beta = Ned.
inline constexpr LocalFrameKind kLocalFrame = LocalFrameKind::Enu;

inline constexpr double kDefaultOriginLat = -7.2867106;
inline constexpr double kDefaultOriginLon = 112.7958064;
inline constexpr double kEarthRadiusM = 6371000.0;
inline constexpr double kPi = 3.14159265358979323846;

struct Origin {
  double lat = kDefaultOriginLat;
  double lon = kDefaultOriginLon;
  bool from_home = false;
};

struct LocalXy {
  double x = 0.0;
  double y = 0.0;
};

inline const char *local_frame_name() {
  return kLocalFrame == LocalFrameKind::Enu ? "ENU" : "NED";
}

inline const char *local_x_axis_name() {
  return kLocalFrame == LocalFrameKind::Enu ? "East" : "North";
}

inline const char *local_y_axis_name() {
  return kLocalFrame == LocalFrameKind::Enu ? "North" : "East";
}

inline Origin default_origin() {
  Origin o;
  o.lat = kDefaultOriginLat;
  o.lon = kDefaultOriginLon;
  o.from_home = false;
  return o;
}

// Bidang datar lokal (equirectangular) relatif origin tetap.
// ENU: x=East (m), y=North (m)
// NED: x=North (m), y=East (m)
inline LocalXy ll_to_local(double lat, double lon, const Origin &origin) {
  const double lat0_rad = origin.lat * (kPi / 180.0);
  const double dlat_rad = (lat - origin.lat) * (kPi / 180.0);
  const double dlon_rad = (lon - origin.lon) * (kPi / 180.0);
  const double east_m = dlon_rad * kEarthRadiusM * std::cos(lat0_rad);
  const double north_m = dlat_rad * kEarthRadiusM;
  if (kLocalFrame == LocalFrameKind::Ned) {
    return LocalXy{north_m, east_m};
  }
  return LocalXy{east_m, north_m};
}

inline std::string trim_ws(std::string s) {
  const auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  const auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

// Parse "[WP] Home: lat, lon" atau "[WP] Home: <none>".
// Baris [WP] lain → nullopt.
inline std::optional<Origin> try_parse_wp_home(const std::string &line) {
  static const std::string kPrefix = "[WP] Home:";
  if (line.size() < kPrefix.size() ||
      line.compare(0, kPrefix.size(), kPrefix) != 0) {
    return std::nullopt;
  }

  const std::string rest = trim_ws(line.substr(kPrefix.size()));
  if (rest.empty() || rest == "<none>") {
    return default_origin();
  }

  std::stringstream ss(rest);
  Origin o;
  char comma = 0;
  if (!(ss >> o.lat)) {
    return std::nullopt;
  }
  if (!(ss >> comma) || comma != ',') {
    return std::nullopt;
  }
  if (!(ss >> o.lon)) {
    return std::nullopt;
  }
  if (o.lat < -90.0 || o.lat > 90.0 || o.lon < -180.0 || o.lon > 180.0) {
    return std::nullopt;
  }
  o.from_home = true;
  return o;
}

inline bool origin_changed(const Origin &a, const Origin &b) {
  return a.from_home != b.from_home ||
         std::fabs(a.lat - b.lat) > 1e-9 ||
         std::fabs(a.lon - b.lon) > 1e-9;
}

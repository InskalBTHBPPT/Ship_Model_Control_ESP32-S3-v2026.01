/**
 * @file wp_serial.hpp
 * @brief Parse baris [WP] mentah dari Remote-Side-05.
 */
#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>

inline constexpr double kDefaultOriginLat = -7.2867106;
inline constexpr double kDefaultOriginLon = 112.7958064;

struct HomeOrigin {
  double lat = kDefaultOriginLat;
  double lon = kDefaultOriginLon;
  bool from_home = false;
};

struct WpLatLon {
  uint32_t index_1based = 0;
  double lat = 0.0;
  double lon = 0.0;
};

inline std::string trim_ws(std::string s) {
  const auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  const auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

inline bool is_waypoint_line(const std::string &line) {
  return line.size() >= 4 && line.compare(0, 4, "[WP]") == 0;
}

inline HomeOrigin default_home_origin() {
  HomeOrigin o;
  o.lat = kDefaultOriginLat;
  o.lon = kDefaultOriginLon;
  o.from_home = false;
  return o;
}

// "[WP] Home: lat, lon" atau "[WP] Home: <none>"
inline std::optional<HomeOrigin> try_parse_wp_home(const std::string &line) {
  static const std::string kPrefix = "[WP] Home:";
  if (line.size() < kPrefix.size() ||
      line.compare(0, kPrefix.size(), kPrefix) != 0) {
    return std::nullopt;
  }

  const std::string rest = trim_ws(line.substr(kPrefix.size()));
  if (rest.empty() || rest == "<none>") {
    return default_home_origin();
  }

  std::stringstream ss(rest);
  HomeOrigin o;
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

// "[WP] #1: lat, lon"
inline std::optional<WpLatLon> try_parse_wp_numbered(const std::string &line) {
  static const std::string kPrefix = "[WP] #";
  if (line.size() < kPrefix.size() ||
      line.compare(0, kPrefix.size(), kPrefix) != 0) {
    return std::nullopt;
  }

  std::stringstream ss(line.substr(kPrefix.size()));
  WpLatLon wp;
  char colon = 0;
  char comma = 0;
  if (!(ss >> wp.index_1based)) {
    return std::nullopt;
  }
  if (!(ss >> colon) || colon != ':') {
    return std::nullopt;
  }
  if (!(ss >> wp.lat)) {
    return std::nullopt;
  }
  if (!(ss >> comma) || comma != ',') {
    return std::nullopt;
  }
  if (!(ss >> wp.lon)) {
    return std::nullopt;
  }
  if (wp.index_1based == 0 || wp.lat < -90.0 || wp.lat > 90.0 ||
      wp.lon < -180.0 || wp.lon > 180.0) {
    return std::nullopt;
  }
  return wp;
}

inline bool origin_changed(const HomeOrigin &a, const HomeOrigin &b) {
  return a.from_home != b.from_home || std::fabs(a.lat - b.lat) > 1e-9 ||
         std::fabs(a.lon - b.lon) > 1e-9;
}

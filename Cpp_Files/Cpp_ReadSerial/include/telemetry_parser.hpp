#pragma once

#include <cctype>
#include <optional>
#include <sstream>
#include <string>

struct TelemetryRow {
  double timestamp;
  double lat;
  double lon;
  double calc_deg_servo_1;
  double calc_deg_servo_2;
  double yaw;
  double gyro_z;
  double yaw_rate;
};

inline bool is_header_line(const std::string &line) {
  return line.rfind("timestamp,", 0) == 0;
}

inline std::optional<TelemetryRow> parse_telemetry_line(const std::string &line) {
  if (line.empty() || is_header_line(line)) {
    return std::nullopt;
  }

  std::stringstream ss(line);
  TelemetryRow row{};
  char comma = ',';

  auto read_field = [&](double &value) -> bool {
    return static_cast<bool>(ss >> value);
  };

  auto read_comma = [&]() -> bool {
    return static_cast<bool>(ss >> comma) && comma == ',';
  };

  if (!read_field(row.timestamp) || !read_comma()) return std::nullopt;
  if (!read_field(row.lat) || !read_comma()) return std::nullopt;
  if (!read_field(row.lon) || !read_comma()) return std::nullopt;
  if (!read_field(row.calc_deg_servo_1) || !read_comma()) return std::nullopt;
  if (!read_field(row.calc_deg_servo_2) || !read_comma()) return std::nullopt;
  if (!read_field(row.yaw) || !read_comma()) return std::nullopt;
  if (!read_field(row.gyro_z) || !read_comma()) return std::nullopt;
  if (!read_field(row.yaw_rate)) return std::nullopt;

  return row;
}

inline std::string trim_cr(std::string line) {
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  return line;
}

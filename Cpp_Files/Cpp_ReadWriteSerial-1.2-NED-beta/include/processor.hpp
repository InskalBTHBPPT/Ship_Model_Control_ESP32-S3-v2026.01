#pragma once

#include "telemetry_parser.hpp"

#include <cmath>
#include <optional>
#include <string>

enum class MathOp { Add, Sub, Mul, Div };

enum class TelemetryField {
  Timestamp,
  Lat,
  Lon,
  CalcDegServo1,
  CalcDegServo2,
  Yaw,
  GyroZ,
  YawRate,
};

inline std::optional<TelemetryField> parse_field_name(const std::string &name) {
  if (name == "timestamp") return TelemetryField::Timestamp;
  if (name == "lat") return TelemetryField::Lat;
  if (name == "lon") return TelemetryField::Lon;
  if (name == "calc_deg_servo_1") return TelemetryField::CalcDegServo1;
  if (name == "calc_deg_servo_2") return TelemetryField::CalcDegServo2;
  if (name == "yaw") return TelemetryField::Yaw;
  if (name == "gyro_z") return TelemetryField::GyroZ;
  if (name == "yaw_rate") return TelemetryField::YawRate;
  return std::nullopt;
}

inline std::optional<MathOp> parse_math_op(const std::string &op) {
  if (op == "add") return MathOp::Add;
  if (op == "sub") return MathOp::Sub;
  if (op == "mul") return MathOp::Mul;
  if (op == "div") return MathOp::Div;
  return std::nullopt;
}

inline double field_value(const TelemetryRow &row, TelemetryField field) {
  switch (field) {
    case TelemetryField::Timestamp: return row.timestamp;
    case TelemetryField::Lat: return row.lat;
    case TelemetryField::Lon: return row.lon;
    case TelemetryField::CalcDegServo1: return row.calc_deg_servo_1;
    case TelemetryField::CalcDegServo2: return row.calc_deg_servo_2;
    case TelemetryField::Yaw: return row.yaw;
    case TelemetryField::GyroZ: return row.gyro_z;
    case TelemetryField::YawRate: return row.yaw_rate;
  }
  return 0.0;
}

inline std::optional<double> apply_math(MathOp op, double a, double b) {
  switch (op) {
    case MathOp::Add: return a + b;
    case MathOp::Sub: return a - b;
    case MathOp::Mul: return a * b;
    case MathOp::Div:
      if (std::fabs(b) < 1e-12) {
        return std::nullopt;
      }
      return a / b;
  }
  return std::nullopt;
}

inline std::optional<double> compute_result(
    const TelemetryRow &row,
    MathOp op,
    TelemetryField field_a,
    TelemetryField field_b) {
  return apply_math(op, field_value(row, field_a), field_value(row, field_b));
}

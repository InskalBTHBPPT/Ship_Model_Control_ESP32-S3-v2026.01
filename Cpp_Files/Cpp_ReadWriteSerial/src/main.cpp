#include "processor.hpp"
#include "serial_port.hpp"
#include "telemetry_parser.hpp"

#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
volatile std::sig_atomic_t g_running = 1;

void handle_signal(int) { g_running = 0; }

void print_usage(const char *program_name) {
  std::cerr
      << "Penggunaan:\n"
      << "  " << program_name << " [opsi]\n\n"
      << "Opsi:\n"
      << "  --port <nama_port>       Port serial (default: COM16 / /dev/ttyUSB0)\n"
      << "  --baud <rate>            Baud rate (default: 115200)\n"
      << "  --timeout <ms>           Timeout baca baris (default: 1000)\n"
      << "  --op <add|sub|mul|div>   Operasi matematika (default: sub)\n"
      << "  --field-a <nama_field>   Field pertama (default: calc_deg_servo_1)\n"
      << "  --field-b <nama_field>   Field kedua (default: calc_deg_servo_2)\n"
      << "  --help                   Tampilkan bantuan ini\n\n"
      << "Field yang didukung:\n"
      << "  timestamp, lat, lon, calc_deg_servo_1, calc_deg_servo_2,\n"
      << "  yaw, gyro_z, yaw_rate\n\n"
      << "Perilaku:\n"
      << "  - Baris CSV asli dari ESP32 -> stdout (tidak ke serial)\n"
      << "  - Baris timestamp,result   -> serial TX saja (tidak ke stdout)\n";
}

std::string default_port() {
#ifdef _WIN32
  return "COM16";
#else
  return "/dev/ttyUSB0";
#endif
}

std::string format_result_line(double timestamp, double result) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(3) << timestamp << ","
      << std::setprecision(2) << result;
  return oss.str();
}
} // namespace

int main(int argc, char **argv) {
  std::string port = default_port();
  uint32_t baud = 115200;
  uint32_t timeout_ms = 1000;
  MathOp math_op = MathOp::Sub;
  TelemetryField field_a = TelemetryField::CalcDegServo1;
  TelemetryField field_b = TelemetryField::CalcDegServo2;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    }
    if (arg == "--port" && i + 1 < argc) {
      port = argv[++i];
      continue;
    }
    if (arg == "--baud" && i + 1 < argc) {
      baud = static_cast<uint32_t>(std::stoul(argv[++i]));
      continue;
    }
    if (arg == "--timeout" && i + 1 < argc) {
      timeout_ms = static_cast<uint32_t>(std::stoul(argv[++i]));
      continue;
    }
    if (arg == "--op" && i + 1 < argc) {
      const auto parsed = parse_math_op(argv[++i]);
      if (!parsed) {
        std::cerr << "[ERROR] Operasi tidak dikenal. Gunakan: add, sub, mul, div\n";
        return 1;
      }
      math_op = *parsed;
      continue;
    }
    if (arg == "--field-a" && i + 1 < argc) {
      const auto parsed = parse_field_name(argv[++i]);
      if (!parsed) {
        std::cerr << "[ERROR] field-a tidak dikenal\n";
        return 1;
      }
      field_a = *parsed;
      continue;
    }
    if (arg == "--field-b" && i + 1 < argc) {
      const auto parsed = parse_field_name(argv[++i]);
      if (!parsed) {
        std::cerr << "[ERROR] field-b tidak dikenal\n";
        return 1;
      }
      field_b = *parsed;
      continue;
    }

    std::cerr << "Argumen tidak dikenal: " << arg << "\n";
    print_usage(argv[0]);
    return 1;
  }

  std::signal(SIGINT, handle_signal);
#ifndef _WIN32
  std::signal(SIGTERM, handle_signal);
#endif

  SerialPort serial;
  if (!serial.open(port, baud)) {
    std::cerr << "[ERROR] " << serial.last_error() << "\n";
    return 1;
  }

  std::cerr << "[INFO] Port " << port << " @ " << baud << " baud (read + write)\n";
  std::cerr << "[INFO] CSV asli -> stdout | timestamp,result -> serial TX\n";
  std::cerr << "[INFO] Tekan Ctrl+C untuk berhenti\n";
  std::cerr << "[INFO] Catatan: firmware ESP32 belum membaca baris result (pending)\n";

  uint64_t valid_lines = 0;
  uint64_t skipped_lines = 0;
  uint64_t write_errors = 0;

  while (g_running) {
    std::string raw_line;
    if (!serial.read_line(raw_line, timeout_ms)) {
      continue;
    }

    const std::string line = trim_cr(raw_line);
    if (line.empty()) {
      continue;
    }

    if (is_header_line(line)) {
      std::cout << line << "\n";
      continue;
    }

    const auto row = parse_telemetry_line(line);
    if (!row) {
      ++skipped_lines;
      continue;
    }

    const auto result = compute_result(*row, math_op, field_a, field_b);
    if (!result) {
      ++skipped_lines;
      std::cerr << "[WARN] Hitung result gagal (mis. div by zero) pada t="
                << row->timestamp << "\n";
      std::cout << line << "\n";
      continue;
    }

    ++valid_lines;
    std::cout << line << "\n";

    const std::string result_line =
        format_result_line(row->timestamp, *result);
    if (!serial.write_line(result_line)) {
      ++write_errors;
      std::cerr << "[ERROR] Gagal tulis ke serial: " << serial.last_error()
                << " | line=" << result_line << "\n";
    }
  }

  std::cerr << "\n[INFO] Selesai. Baris valid: " << valid_lines
            << ", dilewati: " << skipped_lines
            << ", gagal tulis serial: " << write_errors << "\n";
  return 0;
}

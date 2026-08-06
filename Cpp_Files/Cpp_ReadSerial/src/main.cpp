#include "serial_port.hpp"
#include "telemetry_parser.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace {
volatile std::sig_atomic_t g_running = 1;

void handle_signal(int) { g_running = 0; }

void print_usage(const char *program_name) {
  std::cout
      << "Penggunaan:\n"
      << "  " << program_name << " [opsi]\n\n"
      << "Opsi:\n"
      << "  --port <nama_port>     Port serial (default: COM16 / /dev/ttyUSB0)\n"
      << "  --baud <rate>          Baud rate (default: 115200)\n"
      << "  --output <file.csv>    Simpan baris valid ke CSV (opsional)\n"
      << "  --timeout <ms>         Timeout baca baris (default: 1000)\n"
      << "  --help                 Tampilkan bantuan ini\n\n"
      << "Heartbeat:\n"
      << "  Mengirim $HB ke ESP32 setiap 1 detik (manual/auto).\n";
}

std::string default_port() {
#ifdef _WIN32
  return "COM16";
#else
  return "/dev/ttyUSB0";
#endif
}
} // namespace

int main(int argc, char **argv) {
  std::string port = default_port();
  uint32_t baud = 115200;
  uint32_t timeout_ms = 1000;
  std::string output_file;

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
    if (arg == "--output" && i + 1 < argc) {
      output_file = argv[++i];
      continue;
    }
    if (arg == "--timeout" && i + 1 < argc) {
      timeout_ms = static_cast<uint32_t>(std::stoul(argv[++i]));
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

  std::ofstream log_file;
  if (!output_file.empty()) {
    log_file.open(output_file, std::ios::out | std::ios::trunc);
    if (!log_file) {
      std::cerr << "[ERROR] Tidak bisa membuka file output: " << output_file << "\n";
      return 1;
    }
  }

  std::cerr << "[INFO] Membaca port " << port << " @ " << baud << " baud\n";
  std::cerr << "[INFO] Heartbeat $HB -> ESP32 setiap 1 detik\n";
  std::cerr << "[INFO] Tekan Ctrl+C untuk berhenti\n";
  std::cerr << "[INFO] Output CSV ke stdout (format sama dengan serial port)\n";

  uint64_t valid_lines = 0;
  uint64_t skipped_lines = 0;
  auto last_hb = std::chrono::steady_clock::now();

  while (g_running) {
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hb).count() >= 1000) {
      if (!serial.write_line("$HB")) {
        std::cerr << "[WARN] Gagal kirim heartbeat: " << serial.last_error() << "\n";
      }
      last_hb = now;
    }

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
      if (log_file) {
        log_file << line << "\n";
      }
      continue;
    }

    const auto row = parse_telemetry_line(line);
    if (!row) {
      ++skipped_lines;
      continue;
    }

    (void)row;
    ++valid_lines;
    std::cout << line << "\n";

    if (log_file) {
      log_file << line << "\n";
    }
  }

  std::cerr << "\n[INFO] Selesai. Baris valid: " << valid_lines
            << ", baris dilewati: " << skipped_lines << "\n";
  return 0;
}

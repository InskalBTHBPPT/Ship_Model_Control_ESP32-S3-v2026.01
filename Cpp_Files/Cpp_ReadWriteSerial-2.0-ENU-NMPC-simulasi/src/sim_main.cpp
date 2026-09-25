/**
 * @file sim_main.cpp
 * @brief Replay file skenario (format USB Remote) → timestamp,result.
 * Tidak membuka COM. Bukan untuk kapal.
 */

#include "nmpc_tick.hpp"
#include "telemetry_parser.hpp"
#include "wp_serial.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace {
void print_usage(const char *program_name) {
  std::cerr
      << "Cpp_ReadWriteSerial-2.0-ENU-NMPC-simulasi\n"
      << "Replay baris seperti USB Remote; keluaran timestamp,result (stdout).\n\n"
      << "Penggunaan:\n"
      << "  " << program_name << " --scenario <file.txt> [--r-tran 3.0] [--quiet]\n"
      << "  " << program_name << " --help\n\n"
      << "File: [WP] Home, [WP] #n, CSV 8 kolom. Baris kosong / diawali '#' (bukan [WP]) dilewati.\n";
}

bool is_comment_line(const std::string &line) {
  return !line.empty() && line[0] == '#' && !is_waypoint_line(line);
}
} // namespace

int main(int argc, char **argv) {
  std::string scenario_path;
  double r_tran = kDefaultRTranM;
  bool quiet = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    }
    if (arg == "--scenario" && i + 1 < argc) {
      scenario_path = argv[++i];
      continue;
    }
    if (arg == "--r-tran" && i + 1 < argc) {
      r_tran = std::stod(argv[++i]);
      continue;
    }
    if (arg == "--quiet") {
      quiet = true;
      continue;
    }
    std::cerr << "Argumen tidak dikenal: " << arg << "\n";
    print_usage(argv[0]);
    return 1;
  }

  if (scenario_path.empty()) {
    print_usage(argv[0]);
    return 1;
  }

  std::ifstream in(scenario_path);
  if (!in) {
    std::cerr << "[ERROR] Tidak bisa buka: " << scenario_path << "\n";
    return 1;
  }

  NmpcSession session;
  session.init(r_tran);

  if (!quiet) {
    std::cerr << "[SIM] " << scenario_path << " | r_tran=" << r_tran
              << " m | v=0 | r dari yaw_rate | psi=pi/2-yaw (0=N 90=E CW) | tanpa serial\n";
  }

  uint64_t csv_n = 0;
  uint64_t wp_n = 0;
  uint64_t skip_n = 0;
  std::string raw;
  while (std::getline(in, raw)) {
    const std::string line = trim_cr(raw);
    if (line.empty() || is_comment_line(line) || is_header_line(line)) {
      continue;
    }

    if (is_waypoint_line(line)) {
      ++wp_n;
      std::string info;
      session.apply_wp_line(line, &info);
      if (!quiet && !info.empty()) {
        std::cerr << "[INFO] " << info << "\n";
      }
      continue;
    }

    const auto row = parse_telemetry_line(line);
    if (!row) {
      ++skip_n;
      continue;
    }

    const NmpcTickInfo tick = session.tick(*row);
    ++csv_n;
    if (!quiet && tick.switched) {
      std::cerr << "[INFO] Masuk r_tran → WP " << tick.active_wp_1based << "\n";
    }
    if (!quiet && tick.mission_complete) {
      std::cerr << "[INFO] Misi selesai\n";
    }
    std::cout << format_result_line(row->timestamp, tick.result_deg) << "\n";
  }

  if (!quiet) {
    std::cerr << "[SIM] selesai. CSV=" << csv_n << " WP=" << wp_n
              << " skip=" << skip_n << "\n";
  }
  return 0;
}

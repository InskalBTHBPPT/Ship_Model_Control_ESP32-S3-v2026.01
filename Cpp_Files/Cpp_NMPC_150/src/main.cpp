// NMPC kapal — simulasi 150 langkah + laporan waktu komputasi (PC benchmark)
#include "simulate_nmpc_kapal.h"
#include "simulate_nmpc_kapal_initialize.h"
#include "simulate_nmpc_kapal_terminate.h"

#include <chrono>
#include <cmath>
#include <iostream>

namespace {

constexpr int kTotalSteps = 150;
constexpr double kRadToDeg = 57.29577951308232;

}  // namespace

int main() {
  double hist_dim[755];
  double history_input[151];
  double time_vector[151];
  double rmse_data[3];

  std::cout << "Memulai simulasi NMPC (" << kTotalSteps << " langkah, dt=1 s)...\n";

  simulate_nmpc_kapal_initialize();

  const auto start = std::chrono::high_resolution_clock::now();
  simulate_nmpc_kapal(hist_dim, history_input, time_vector, rmse_data);
  const auto end = std::chrono::high_resolution_clock::now();

  simulate_nmpc_kapal_terminate();

  const double elapsed_s =
      std::chrono::duration<double>(end - start).count();
  const double elapsed_ms = elapsed_s * 1000.0;
  const double avg_step_ms = elapsed_ms / kTotalSteps;

  std::cout << "\nSIMULASI SELESAI\n";
  std::cout << "Total Waktu Komputasi : " << elapsed_s << " detik\n";
  std::cout << "Total Waktu (ms)      : " << elapsed_ms << " ms\n";
  if (avg_step_ms > 0.0) {
    std::cout << "Rata-rata per step    : " << avg_step_ms << " ms ("
              << (1000.0 / avg_step_ms) << " Hz)\n";
  }

  std::cout << "\n--- HASIL PERHITUNGAN RMSE ---\n";
  std::cout << "RMSE X   : " << rmse_data[0] << " meter\n";
  std::cout << "RMSE Y   : " << rmse_data[1] << " meter\n";
  std::cout << "RMSE Psi : " << rmse_data[2] << " rad ("
            << (rmse_data[2] * kRadToDeg) << " derajat)\n";

  return 0;
}

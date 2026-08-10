// NMPC kapal — satu solve (1× fmincon, horizon N=30) + waktu komputasi
#include "nmpc_solve_once.h"
#include "simulate_nmpc_kapal_initialize.h"
#include "simulate_nmpc_kapal_terminate.h"

#include <chrono>
#include <cmath>
#include <iostream>

namespace {

constexpr double kRadToDeg = 57.29577951308232;

void printState(const char *label, const double s[5]) {
  std::cout << label << " v=" << s[0] << " r=" << s[1]
            << " x=" << s[2] << " y=" << s[3]
            << " psi=" << (s[4] * kRadToDeg) << " deg\n";
}

}  // namespace

int main() {
  double u_rudder_rad = 0.0;
  double exitflag = 0.0;
  double state_before[5];
  double state_after[5];

  std::cout << "Memulai satu solve NMPC (fmincon, N=30)...\n";

  simulate_nmpc_kapal_initialize();

  const auto t0 = std::chrono::high_resolution_clock::now();
  nmpc_solve_once(&u_rudder_rad, &exitflag, state_before, state_after);
  const auto t1 = std::chrono::high_resolution_clock::now();

  simulate_nmpc_kapal_terminate();

  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  const double elapsed_s = elapsed_ms / 1000.0;

  std::cout << "\nSOLVE SELESAI\n";
  std::cout << "Waktu Komputasi     : " << elapsed_ms << " ms ("
            << elapsed_s << " detik)\n";
  if (elapsed_ms > 0.0) {
    std::cout << "Estimasi rate       : " << (1000.0 / elapsed_ms) << " Hz\n";
  }
  std::cout << "Exitflag fmincon    : " << exitflag << "\n";
  std::cout << "Rudder (rad)        : " << u_rudder_rad << "\n";
  std::cout << "Rudder (deg)        : " << (u_rudder_rad * kRadToDeg)
            << "\n";

  std::cout << "\n--- State ---\n";
  printState("Sebelum", state_before);
  printState("Sesudah (dt=1s)", state_after);

  return 0;
}

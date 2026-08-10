# Cpp_NMPC_150

Benchmark simulasi NMPC kapal (150 langkah, `dt = 1 s`) di PC Windows/Linux.

Kode solver di `lib/nmpc/` berasal dari MATLAB Coder (`simulate_nmpc_kapal`), hanya file `.cpp` / `.h` yang diperlukan untuk build — tanpa MEX, `main` codegen, atau artefak build MATLAB.

Tambahan minimal untuk build standalone (bukan dari folder codegen asli):

- `lib/nmpc/tmwtypes.h` — typedef MATLAB Coder (tanpa instalasi MATLAB)
- OpenMP (`-fopenmp`) — dipakai oleh solver (`xgeqp3.cpp`)

| Parameter | Nilai |
|-----------|-------|
| Total simulasi | 150 s (150 langkah) |
| Horizon NMPC | 30 langkah (`Tp = 30`, `T_sim = 1`) |
| Sumber asli | `MPC NMPC Agus/.../NMPC/C++/codegen/lib/simulate_nmpc_kapal` |

## Build

Prasyarat: `g++` (MinGW-w64 / MSYS2) dengan dukungan **OpenMP**.

```bat
build.bat
```

Atau manual:

```powershell
g++ -std=c++17 -O2 -I lib/nmpc src/main.cpp lib/nmpc/*.cpp -o nmpc_150.exe
```

## Jalankan

```powershell
.\nmpc_150.exe
```

Output: total waktu komputasi, rata-rata ms per step NMPC, dan RMSE X/Y/Psi (sama konsep `run_nmpc.m` di MATLAB).

**Catatan:** Simulasi penuh bisa memakan beberapa menit; tidak ada progress bar di terminal.

## Struktur

```text
Cpp_NMPC_150/
  src/main.cpp      — entry point + pengukuran waktu (chrono)
  lib/nmpc/         — solver NMPC (codegen, 60 cpp)
  build.bat
  CMakeLists.txt
```

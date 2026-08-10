# Cpp_NMPC_1

Benchmark **satu solve NMPC** di PC — 1× `fmincon` (horizon `N = 30`), terapkan `U_opt(1)` sebagai rudder.

Pasangan simulasi penuh: `Cpp_Files/Cpp_NMPC_150` (150 langkah).

| Parameter | Nilai |
|-----------|-------|
| Solve NMPC | **1×** per run |
| Horizon prediksi | 30 langkah (`Tp = 30 s`, `dt = 1 s`) |
| State awal | sama dengan simulasi MATLAB (x≈100 m, psi=0) |
| Sumber solver | MATLAB Coder → `lib/nmpc/` |

## Build

Prasyarat: `g++` (MinGW-w64 / MSYS2) dengan **OpenMP**.

```bat
build.bat
```

## Jalankan

```powershell
.\nmpc_1.exe
```

Output: waktu solve (ms), `exitflag`, rudder (rad/deg), state sebelum/sesudah satu langkah Euler.

## API solver

```cpp
#include "nmpc_solve_once.h"

nmpc_solve_once(&u_rudder_rad, &exitflag, state_before_dim, state_after_dim);
```

Entry point `simulate_nmpc_kapal()` (150 langkah) **tidak** dipakai di proyek ini; diganti `nmpc_solve_once()` di `simulate_nmpc_kapal.cpp`.

## Struktur

```text
Cpp_NMPC_1/
  src/main.cpp
  lib/nmpc/          — solver + nmpc_solve_once.h
  build.bat
```

## Catatan

- `lib/nmpc/tmwtypes.h` — typedef standalone (tanpa instalasi MATLAB).
- Regenerate dari MATLAB: buat fungsi `nmpc_one_step` lalu codegen; atau ubah `T_sim_total = 1` di `simulate_nmpc_kapal.m`.

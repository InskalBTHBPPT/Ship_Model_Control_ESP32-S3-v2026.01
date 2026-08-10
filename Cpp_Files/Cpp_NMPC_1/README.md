# Cpp_NMPC_1

Benchmark **satu solve NMPC** di PC — 1× `fmincon` (horizon `N = 30`), terapkan `U_opt(1)` sebagai rudder.

Pasangan simulasi penuh: `Cpp_Files/Cpp_NMPC_150` (150 langkah).

| Parameter | Nilai |
|-----------|-------|
| Solve NMPC | **1×** per run |
| Horizon prediksi | 30 langkah (`Tp = 30 s`, `dt = 1 s`) |
| State awal | sama dengan simulasi MATLAB (x≈100 m, psi=0) |
| Sumber solver | MATLAB Coder → `lib/nmpc/` |

## Konstanta

### Kapal & skala nondimensional

| Simbol | Nilai | Keterangan |
|--------|-------|------------|
| `L` | 101.07 m | Panjang kapal |
| `u_0` | 15.4 m/s | Kecepatan maju (surge) |
| `u_0_nd` | 1 | Surge nondimensional |
| `dt` / `T_sim` | 1 s | Langkah waktu |
| `dt_nd` | `T_sim * u_0 / L` ≈ 0.15237 | Langkah waktu nondimensional |
| `Tp` | 30 s | Horizon prediksi |
| `N` | 30 | Jumlah langkah horizon (`Tp / T_sim`) |

### Bobot & batas

| Simbol | Nilai | Keterangan |
|--------|-------|------------|
| `Q` | `diag(10, 1, 1)` | Bobot error `[x, y, ψ]` |
| `R` | 1 | Bobot input rudder |
| `u_limit` | ±35° ≈ ±0.6109 rad | Batas sudut rudder |
| `u_rate_limit` | 5°/s | Batas laju rudder |
| `du_max` | `u_rate_limit * T_sim` ≈ 0.0873 rad | Batas Δu per langkah |
| `r_limit` | 0.0932 rad/s | Batas yaw rate dimensional |
| `r_limit_nd` | `r_limit * L / u_0` ≈ 0.6117 | Batas yaw rate nondimensional |

### Model linear (sudah di-codegen)

```text
A_sys ≈ [[-0.6137, -0.1018],
         [-5.0966, -3.4085]]
B_sys =  [0.01; 1]
```

### State awal & referensi (satu solve di `Cpp_NMPC_1`)

| Simbol | Nilai dimensional | Nilai nondimensional |
|--------|-------------------|----------------------|
| `s0` | `[v, r, x, y, ψ] = [0, 0, 0, 100, 0]` | ≈ `[0, 0, 0, 0.9894, 0]` |
| `u_prev` | 0 rad | rudder sebelumnya |
| `h_ref` | `[x, y, ψ] = [0, 0, 0]` | lintasan lurus di y=0, ψ=0 |

Konversi:

- dimensional → nondimensional: `v/u₀`, `r·L/u₀`, `x/L`, `y/L`, `ψ`
- nondimensional → dimensional: `v·u₀`, `r·u₀/L`, `x·L`, `y·L`, `ψ`

## Variable input

Solver `nmpc_solve_once` memakai state/referensi yang di-hardcode (sama `simulate_nmpc_kapal.m`). Secara konsep, input ke optimasi NMPC:

| Variable | Dimensi | Deskripsi |
|----------|---------|-----------|
| `s_nd` | 5×1 | State sekarang `[v, r, x, y, ψ]` (nondimensional) |
| `u_prev` | skalar | Rudder langkah sebelumnya (rad) |
| `x_ref_seq` | N×1 | Referensi x sepanjang horizon: `(h_ref(1) + t_pred · u₀) / L` |
| `y_ref_seq` | N×1 | Referensi y: `h_ref(2)/L` (nol) |
| `psi_ref_seq` | N×1 | Referensi ψ: `h_ref(3)` (nol) |
| `U0` | N×1 | Tebakan awal = `u_prev` berulang |
| `A_du`, `b_du` | 2N×N, 2N×1 | Kendala laju rudder `‖Δu‖ ≤ du_max` |
| `lb`, `ub` | N×1 | `±u_limit` |

Di `main.cpp`, pemanggilan tidak mengisi input dari luar — semua disiapkan di dalam `nmpc_solve_once()`.

## Pemanggilan dari file lain (as function)

`Cpp_NMPC_1` bisa dipakai sebagai **library NMPC** (bukan hanya benchmark `nmpc_1.exe`). Entry point codegen: `nmpc_solve_once()` di `lib/nmpc/simulate_nmpc_kapal.cpp`.

### Lifecycle

```cpp
#include "nmpc_solve_once.h"
#include "simulate_nmpc_kapal_initialize.h"
#include "simulate_nmpc_kapal_terminate.h"

// Sekali saat startup modul kontrol
simulate_nmpc_kapal_initialize();

// Tiap langkah kontrol (loop waypoint / timer 1 Hz)
double u_apply = 0.0;
double exitflag = 0.0;
double state_before[5];
double state_after[5];
nmpc_solve_once(&u_apply, &exitflag, state_before, state_after);

// Opsional saat shutdown
simulate_nmpc_kapal_terminate();
```

### API saat ini (`nmpc_solve_once`)

```cpp
void nmpc_solve_once(double *u_rudder_rad, double *exitflag,
                     double state_before_dim[5],
                     double state_after_dim[5]);
```

| Arah | Parameter | Tipe | Keterangan |
|------|-----------|------|------------|
| Output | `u_rudder_rad` | `double` | Rudder optimal langkah pertama $U_{\mathrm{opt}}(1)$ (rad) |
| Output | `exitflag` | `double` | Status `fmincon` (`> 0` = sukses) |
| Output | `state_before_dim` | `double[5]` | State sebelum apply: `[v, r, x, y, ψ]` dimensional |
| Output | `state_after_dim` | `double[5]` | State setelah 1 langkah Euler (`dt = 1` s) |

**Catatan:** State dan referensi **belum** bisa diisi dari luar — di dalam fungsi masih hardcode (y ≈ 100 m, `u_prev = 0`, lintasan lurus). Lihat tabel “Parameter yang seharusnya” di bawah untuk integrasi penuh.

### Parameter yang seharusnya ada (konsep integrasi)

Jika `nmpc_solve_once` diperluas jadi fungsi yang dipanggil dari loop kontrol (ESP32, waypoint, simulasi), parameter per langkah MPC:

| Parameter | Tipe | Wajib? | Deskripsi |
|-----------|------|--------|-----------|
| `state` | `double[5]` | Ya | State sekarang `[v, r, x, y, ψ]` (dimensional atau nondimensional — konsisten dengan konversi di dalam) |
| `u_prev` | `double` | Ya | Rudder langkah sebelumnya (rad), untuk kendala laju $\Delta u$ |
| `t` | `double` | Ya* | Waktu simulasi (s), untuk membangun referensi horizon ke depan |
| `h_ref` | `double[3]` | Ya* | Referensi `[x, y, ψ]` (mis. waypoint: y=0, ψ=0) |

\* Jika referensi horizon tidak garis lurus, ganti dengan vektor eksplisit `x_ref[N]`, `y_ref[N]`, `psi_ref[N]`.

**Referensi horizon** (untuk $N = 30$, $dt = 1$ s), sama seperti MATLAB:

- $x_{\mathrm{ref},i} = (h_{\mathrm{ref}}[0] + (t + i \cdot dt) \cdot u_0) / L$
- $y_{\mathrm{ref},i} = h_{\mathrm{ref}}[1] / L$
- $\psi_{\mathrm{ref},i} = h_{\mathrm{ref}}[2]$

### Parameter konfigurasi (sekali init, tidak tiap langkah)

Saat ini tertanam di artefak codegen; mengubahnya butuh regenerate MATLAB Coder kecuali dibungkus struct config:

| Parameter | Nilai default | Fungsi |
|-----------|---------------|--------|
| `L`, `u_0` | 101.07 m, 15.4 m/s | Konversi dimensional ↔ nondimensional |
| `N`, `dt` | 30, 1 s | Horizon dan langkah prediksi |
| `Q`, `R` | `diag(10,1,1)`, 1 | Bobot error posisi/heading dan rudder |
| `u_limit`, `u_rate_limit`, `r_limit` | ±35°, 5°/s, 0.0932 rad/s | Batas rudder, laju rudder, yaw rate |
| `A_sys`, `B_sys` | lihat codegen | Model linier $[v, r]$ |

### Contoh signature fungsi wrapper (rekomendasi)

**Minimal** — caller punya dinamika/RK4 sendiri:

```cpp
bool nmpc_step(
    const double state_dim[5],   // [v, r, x, y, psi]
    double u_prev,               // rad
    double t,                    // s
    const double h_ref[3],       // [x_ref, y_ref, psi_ref]
    double *u_apply,             // output rudder (rad)
    int *exitflag);              // status solver
```

**Dengan update state internal** (seperti `nmpc_solve_once` sekarang):

```cpp
bool nmpc_step(
    const double state_dim[5],
    double u_prev,
    double t,
    const double h_ref[3],
    double *u_apply,
    int *exitflag,
    double state_after_dim[5]);
```

### Alur loop kontrol (receding horizon)

```text
init NMPC (sekali)
  ↓
set state, u_prev, waypoint → h_ref, t
  ↓
nmpc_step(...) → u_apply, exitflag
  ↓
terapkan u_apply ke rudder + update state (RK4 / hardware)
  ↓
u_prev ← u_apply; t ← t + dt
  ↓
ulang tiap T_sim (1 s)
```

### Link ke file

| File | Fungsi |
|------|--------|
| `lib/nmpc/nmpc_solve_once.h` | Deklarasi API |
| `lib/nmpc/simulate_nmpc_kapal.cpp` | Implementasi `nmpc_solve_once` |
| `lib/nmpc/simulate_nmpc_kapal_initialize.h` | Init workspace solver |
| `src/main.cpp` | Contoh benchmark satu solve + timing |

Untuk wrapper dengan parameter input dari luar, perlu modifikasi `simulate_nmpc_kapal.cpp` atau regenerate dari MATLAB fungsi `nmpc_one_step(s_nd, u_prev, t, h_ref)`.

## Formula

### Dinamika kapal (nondimensional)

$$
\begin{bmatrix}\dot v \\ \dot r\end{bmatrix}
= A_{\mathrm{sys}}
\begin{bmatrix}v \\ r\end{bmatrix}
+ B_{\mathrm{sys}}\, u
$$

$$
\dot x = u_{0,\mathrm{nd}}\cos\psi - v\sin\psi,\quad
\dot y = u_{0,\mathrm{nd}}\sin\psi + v\cos\psi,\quad
\dot\psi = r
$$

Propagasi Euler: $s_{k+1} = s_k + dt_{\mathrm{nd}}\, f(s_k, u_k)$.

### Masalah optimasi NMPC

Cari $U = [u_1,\ldots,u_N]$ yang meminimalkan

$$
J(U) = \sum_{i=1}^{N}
\Bigl(
e_i^\top Q\, e_i + R\, u_i^2
\Bigr),\quad
e_i =
\begin{bmatrix}
x_i - x_{\mathrm{ref},i} \\
y_i - y_{\mathrm{ref},i} \\
\psi_i - \psi_{\mathrm{ref},i}
\end{bmatrix}
$$

dengan kendala:

| Jenis | Formula |
|-------|---------|
| Batas rudder | $\|u_i\| \le u_{\mathrm{limit}}$ |
| Laju rudder | $\|u_1 - u_{\mathrm{prev}}\| \le du_{\max}$, $\|u_i - u_{i-1}\| \le du_{\max}$ |
| Yaw rate | $\|r_i\| \le r_{\mathrm{limit,nd}}$ (nonlinear, lewat `nonlcon`) |

Solver: `fmincon` (algoritma SQP, codegen MATLAB).

### Receding horizon (setelah solve)

Hanya $u_{\mathrm{applied}} = U_{\mathrm{opt}}(1)$ yang diterapkan; state di-update satu langkah Euler `dt = 1 s`. Jika `exitflag ≤ 0`, dipakai fallback $U = U_0$ (`u_prev`).

## Output

Rincian API dan parameter integrasi: lihat [Pemanggilan dari file lain (as function)](#pemanggilan-dari-file-lain-as-function).

### Output terminal (`nmpc_1.exe`)

- Waktu komputasi solve (ms / detik) dan estimasi rate (Hz)
- `exitflag` fmincon
- Rudder (rad dan derajat)
- State sebelum / sesudah (`v`, `r`, `x`, `y`, `ψ`)

## Build

Prasyarat: `g++` (MinGW-w64 / MSYS2) dengan **OpenMP**.

```bat
build.bat
```

## Jalankan

```powershell
.\nmpc_1.exe
```

## Struktur

```text
Cpp_NMPC_1/
  src/main.cpp
  lib/nmpc/          — solver + nmpc_solve_once.h
  build.bat
```

## Catatan

- `lib/nmpc/tmwtypes.h` — typedef standalone (tanpa instalasi MATLAB).
- Entry point `simulate_nmpc_kapal()` (150 langkah) **tidak** dipakai di proyek ini; diganti `nmpc_solve_once()` di `simulate_nmpc_kapal.cpp`.
- Regenerate dari MATLAB: buat fungsi `nmpc_one_step` lalu codegen; atau ubah `T_sim_total = 1` di `simulate_nmpc_kapal.m`.
- Sumber rumus/parameter: `MPC NMPC Agus/.../NMPC/C++/kode matlab/simulate_nmpc_kapal.m`.

# Cpp_MPC_150

Simulasi **Linear MPC konvensional** (port dari `MPC/MPC_Biasa_Lurus.m` / Python) — **150 langkah** (`dt=1 s`), horizon `Np=60`.

Tidak memakai MATLAB Coder; solver QP native di `mpc_common/`.

## Build

```bat
build.bat
```

## Jalankan

```powershell
.\mpc_150.exe
```

## Parameter (default)

| Parameter | Nilai |
|-----------|-------|
| `Np` | 60 |
| `Tp` | 60 s |
| Langkah simulasi | 151 titik (`T_sim_total=150`, dt=1 s) |
| State awal Y | 500 m |

## Struktur

```text
Cpp_MPC_150/
  src/main.cpp
  build.bat
../mpc_common/   — model kapal + QP LMPC
```

# mpc_common

Kode bersama untuk **Linear MPC konvensi** (`MPC_Biasa_Lurus.m` / Python).

| File | Fungsi |
|------|--------|
| `mpc_model.hpp/cpp` | Model linier kapal, RK4, nondimensional |
| `mpc_qp.hpp/cpp` | Pembentukan QP (`Np=60`), null-space + solve step |
| `qp_osqp.hpp/cpp` | Solver OSQP pada QP tereduksi (60 variabel) |
| `qp_solver.hpp/cpp` | Cholesky / matriks untuk null-space |
| `third_party/osqp/` | OSQP v0.6.3 + qdldl (vendor) |
| `osqp_flags.bat` | Include path dan daftar sumber OSQP untuk `g++` |

Digunakan oleh `Cpp_MPC_1` dan `Cpp_MPC_150`.

**Solver:** OSQP menyelesaikan QP tereduksi setelah eliminasi kendala dinamika (padanan `quadprog` Python untuk langkah pertama). Simulasi panjang dapat sedikit berbeda dari Python pada fase transisi rudder.

**Setup:** clone `qdldl` ke `third_party/osqp/lin_sys/direct/qdldl/qdldl_sources` jika belum ada.

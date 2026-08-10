# mpc_common

Kode bersama untuk **Linear MPC konvensi** (`MPC_Biasa_Lurus.m` / Python).

| File | Fungsi |
|------|--------|
| `mpc_model.hpp/cpp` | Model linier kapal, RK4, nondimensional |
| `mpc_qp.hpp/cpp` | Pembentukan QP (`Np=60`), null-space, solve step |
| `qp_solver.hpp/cpp` | Solver QP dense (projected gradient + projection) |

Digunakan oleh `Cpp_MPC_1` dan `Cpp_MPC_150`.

**Catatan:** Solver C++ adalah pendekatan iteratif (bukan `quadprog` MATLAB). Hasil rudder langkah pertama mendekati Python (~−5°); untuk presisi identik MATLAB pertimbangkan OSQP / codegen `quadprog`.

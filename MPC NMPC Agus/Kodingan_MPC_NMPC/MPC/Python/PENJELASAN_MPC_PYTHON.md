# Penjelasan MPC Python (`mpc_biasa_lurus.py`)

Port Python dari `MPC/MPC_Biasa_Lurus.m` — Linear MPC konvensional dengan formulasi QP penuh (sama seperti `quadprog` di MATLAB).

## Struktur file

| File | Fungsi |
|------|--------|
| `mpc_model.py` | Model linier kapal `[v, r, x, y, ψ]`, diskretisasi Euler, RK4 plant |
| `mpc_qp.py` | Pembentukan matriks QP (`H`, `A_eq`, `A_ineq`) dan solver `quadprog` |
| `mpc_biasa_lurus.py` | Loop simulasi 180 detik, plot, RMSE |
| `requirements.txt` | Dependensi Python |

Parameter kapal diambil dari `NMPC/Python/nmpc_model.py` (`ShipParams`) agar konsisten dengan port NMPC.

## Cara menjalankan

```bash
cd "MPC NMPC Agus/Kodingan_MPC_NMPC/MPC/Python"
pip install -r requirements.txt
python mpc_biasa_lurus.py
```

Atau dari Python:

```python
from mpc_biasa_lurus import run_simulation
run_simulation(plot=True, log_step_time=True)
```

## Parameter simulasi (sama MATLAB)

| Parameter | Nilai |
|-----------|-------|
| Horizon prediksi `Tp` | 60 s |
| Langkah prediksi `Np` | 60 |
| `dt_pred` | 1 s |
| Langkah simulasi | 1 s |
| Total simulasi | 180 s |
| Bobot `Q` | diag(50, 50, 50) pada state x, y, ψ |
| Bobot `R` | 100 |
| Kondisi awal | `[v, r, x, y, ψ] = [0, 0, 0, 500 m, 0]` |
| Referensi | lintasan lurus `y = 0`, heading `0` |

## Log per langkah waktu `t`

```text
   t (s)  waktu (ms)    iter  status
----------------------------------------
     0.0      688.58       0      ok
    20.0      536.42       0      ok
   ...
```

| Kolom | Arti |
|-------|------|
| **t (s)** | Waktu simulasi |
| **waktu (ms)** | Waktu CPU satu siklus MPC + integrasi plant |
| **iter** | Tidak tersedia dari `quadprog` Python (selalu 0) |
| **status** | `ok` jika QP berhasil, `fail: ...` jika gagal |

## Formulasi QP

Minimasi (setara MATLAB `quadprog(H, f, A_ineq, b_ineq, A_eq, b_eq, lb, ub)`):

```
min  ½ zᵀ H z + fᵀ z
s.t. A_eq z = b_eq          (dinamika + kondisi awal)
     A_ineq z ≤ b_ineq      (batas laju rudder)
     lb ≤ z ≤ ub             (batas yaw rate & sudut rudder)
```

Vektor keputusan: `z = [s₀…s_Np, u₀…u_{Np-1}]` dengan `s_k ∈ ℝ⁵`.

Solver: paket **`quadprog`** (Goldfarb–Idnani), padanan langsung solver MATLAB.

## Hasil uji (Windows, Np=60)

| Metrik | Nilai tipikal |
|--------|---------------|
| RMSE X | ~0 m (referensi x mengikuti waktu) |
| RMSE Y | ~209 m (rata-rata selama konvergensi 500 m → 0 m) |
| Posisi Y akhir | ~0.13 m |
| Waktu total 180 langkah | ~67–80 detik |
| Waktu per langkah | ~350–700 ms |

> RMSE Y tinggi karena dihitung sepanjang 180 detik termasuk fase awal dengan offset 500 m. Posisi akhir mendekati referensi.

## Plot

Sama seperti MATLAB:

1. **Trajektori** kapal vs garis referensi `y = 0`
2. **Heading** aktual vs referensi
3. **Verifikasi batasan**: yaw rate, sudut rudder, laju perubahan rudder

## Perbedaan dengan NMPC Python

| Aspek | MPC (ini) | NMPC |
|-------|-----------|------|
| Model prediksi | Linier (Euler diskrit) | Nonlinier (Euler) |
| Optimizer | QP (`quadprog`) | SLSQP (`scipy`) |
| Variabel optimasi | State + input (365) | Hanya input (60) |
| Horizon | 60 s | 30 s |
| Offset awal Y | 500 m | 100 m |

## Catatan implementasi

- Plant disimulasikan dengan **RK4** pada model linier kontinyu (bukan model diskrit QP), sama seperti MATLAB.
- Batas state tak terbatas (`±inf`) diimplementasi lewat kendala kotak pada solver `quadprog`.
- Jika `quadprog` gagal pada suatu langkah, peringatan dicetak dan input nol dipakai untuk langkah tersebut.

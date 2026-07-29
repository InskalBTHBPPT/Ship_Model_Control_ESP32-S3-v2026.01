# Penjelasan NMPC Python (`nmpc_biasa.py`)

Dokumen ini menjelaskan:

1. Output log per langkah waktu `t` (SLSQP, `mpc_cost`, propagasi horizon)
2. Fungsi tiap **level** komputasi dalam satu siklus NMPC
3. Arti **"terbaik"** dalam optimasi NMPC

Port Python ini setara dengan `NMPC/MATLAB/NMPC_Biasa.m` dan telah divalidasi terhadap hasil MATLAB/C++:

| Metrik   | MATLAB/C++ | Python   |
|----------|------------|----------|
| RMSE X   | 6.5354 m   | 6.5355 m |
| RMSE Y   | 37.2324 m  | 37.2323 m |
| RMSE ψ   | 0.0809 rad | 0.0809 rad |

---

## 1. Log per langkah waktu `t`

Saat menjalankan:

```bash
python nmpc_biasa.py
```

atau:

```python
from nmpc_biasa import run_simulation
run_simulation(plot=True, log_step_time=True)
```

konsol menampilkan statistik **per detik simulasi**:

```text
   t (s)  waktu (ms)   SLSQP   mpc_cost   kendala   prop_tot  status
                                              (N=30)
--------------------------------------------------------------------
     0.0      312.90       9        280       321     18030      OK
     1.0      337.55       9        281       322     18090      OK
   ...
   149.0       74.58       1         31        64      2850      OK
```

### 1.1 Arti setiap kolom

| Kolom        | Sumber di kode                         | Arti |
|--------------|----------------------------------------|------|
| **t (s)**    | Langkah simulasi                       | Waktu simulasi (0, 1, 2, …, 149) |
| **waktu (ms)** | `time.perf_counter()` per loop       | Waktu aktual CPU untuk seluruh siklus `t` (optimasi + update state) |
| **SLSQP**    | `result.nit` dari `scipy.optimize.minimize` | Jumlah iterasi algoritma SLSQP |
| **mpc_cost** | Counter pada fungsi `objective()`      | Berapa kali fungsi biaya `mpc_cost` dievaluasi |
| **kendala**  | Counter pada `nonlinear_ineq()`        | Berapa kali `state_constraints` dievaluasi |
| **prop_tot** | `(mpc_cost + kendala) × N`             | Total propagasi Euler horizon dalam optimizer |
| **status**   | `result.success`                       | `OK` jika SLSQP konvergen, `FAIL` jika tidak |

### 1.2 Rumus propagasi

Horizon prediksi **N = 30** (dari `Tp = 30 s`, `T_sim = 1 s`).

Setiap evaluasi `mpc_cost` atau `state_constraints` mensimulasikan **30 langkah** ke depan (prediksi imajiner).

```text
prop_tot = (mpc_cost + kendala) × N
         = (mpc_cost + kendala) × 30
```

**Contoh `t = 0`:**

```text
(280 + 321) × 30 = 18.030 propagasi
```

**Contoh `t = 149`:**

```text
(31 + 64) × 30 = 2.850 propagasi
```

Di luar optimizer, setiap `t` ada **1 propagasi nyata** (`euler_step` pada plant simulasi). Total 150 propagasi nyata untuk simulasi 150 detik — dicetak di ringkasan akhir.

### 1.3 Ringkasan setelah simulasi

```text
SIMULASI SELESAI
Total Waktu Komputasi: 43.6873 detik
Waktu per langkah t - min: 74.58 ms, max: 701.75 ms, rata-rata: 291.20 ms
SLSQP per t - min: 1, max: 11, rata-rata: 4.3
mpc_cost per t - min: 31, max: 343, rata-rata: 135.6
Propagasi horizon (N=30 per evaluasi) - min: 2850, max: 21870, total seluruh simulasi: 1383330
(+ 150 propagasi nyata kapal, 1x Euler per langkah t di luar optimizer)
```

### 1.4 Data yang dikembalikan fungsi `run_simulation()`

```python
r = run_simulation(plot=False)

r["slsqp_iters"]         # array int, per t
r["mpc_cost_calls"]      # array int, per t
r["constraint_calls"]    # array int, per t
r["total_propagations"]  # array int, per t
r["step_times_s"]        # waktu detik per t
r["horizon_n"]           # 30
```

### 1.5 Mengapa `t` besar → waktu/SLSQP/propagasi cenderung mengecil?

| Fase `t` | SLSQP | mpc_cost | prop_tot | waktu tipikal |
|----------|-------|----------|----------|---------------|
| Awal (0–10) | 8–11 | ~250–280 | ~16.000–18.000 | 300–700 ms |
| Tengah (70–90) | 3–4 | ~95–97 | ~6.750–6.870 | 180–230 ms |
| Akhir (130–149) | 1–2 | ~31–65 | ~2.850–4.920 | 75–160 ms |

**Penyebab utama:**

1. **Warm start:** tebakan awal `U0 = u_prev × ones(N,1)` — saat `t` besar, `u_prev` sudah dekat solusi optimal.
2. **Masalah lebih mudah:** error lateral/heading lebih kecil → landscape cost lebih datar → SLSQP konvergen lebih cepat.
3. **Bukan karena N berubah:** N tetap 30; yang berkurang adalah **jumlah evaluasi** level 2 dan 3.

Lonjakan di tengah (mis. `t = 8` → 701 ms) masih mungkin karena line search atau kendala aktif — tren umum turun, bukan aturan ketat per detik.

### 1.6 Mematikan log per baris

```python
run_simulation(plot=True, log_step_time=False)
```

---

## 2. Fungsi tiap level komputasi

Setiap **1 detik simulasi** (`t`) NMPC menjalankan struktur bersarang berikut.

```text
t = 10 detik simulasi
│
├─ LEVEL 1  → 1× siklus kontrol NMPC (satu baris log)
│
├─ LEVEL 2  → SLSQP: perbaiki tebakan U berulang kali
│   │
│   ├─ LEVEL 3a → mpc_cost: "kalau rudder begini, seberapa buruk?"
│   ├─ LEVEL 3b → kendala: "apakah yaw rate aman?"
│   │   │
│   │   └─ LEVEL 4 → di dalam tiap evaluasi: simulasikan N=30 langkah ke depan
│   │
│   └─ (ulang level 2–4 sampai konvergen atau max 200 iterasi)
│
└─ LEVEL 5 (di luar optimizer) → terapkan u₁ ke kapal nyata (1× Euler)
```

---

### LEVEL 1 — Langkah waktu simulasi (`t`)

| | |
|--|--|
| **Fungsi** | Mengatur **kapan** kontrol diperbarui |
| **Frekuensi** | 150× untuk simulasi 150 detik (interval 1 detik) |
| **Input** | State kapal sekarang `s_nd`, referensi horizon, `u_prev` |
| **Output** | Satu rudder `u_applied` + state kapal langkah berikutnya |
| **Di log** | Satu baris penuh (`t`, `waktu`, `SLSQP`, …) |
| **Di kode** | Loop `for step in range(num_steps)` di `nmpc_biasa.py` |

**Analogi:** Setiap detik, kapten bertanya: *"Rencana kemudi apa yang terbaik untuk 30 detik ke depan?"*

---

### LEVEL 2 — Iterasi SLSQP

| | |
|--|--|
| **Fungsi** | **Mencari** vektor rudder optimal `U = [u₁, u₂, …, u₃₀]` |
| **Solver** | `scipy.optimize.minimize(..., method='SLSQP')` |
| **Setara MATLAB** | `fmincon(..., 'Algorithm', 'sqp')` |
| **Di log** | Kolom **SLSQP** (`result.nit`) |
| **Maksimum** | 200 iterasi (`maxiter: 200`) |

**Tiap iterasi SLSQP kira-kira:**

1. Evaluasi cost di titik saat ini
2. Estimasi gradien numerik (butuh banyak panggilan `mpc_cost`)
3. Line search — coba beberapa kandidat `U`
4. Perbarui `U` jika cost turun
5. Ulangi sampai konvergen (`ftol: 1e-6`) atau gagal

**Analogi:** Mencari lembah terendah sambil buta peta — tiap iterasi SLSQP = *"cek sekitar, geser sedikit, cek lagi"*.

**Penting:** 1 iterasi SLSQP ≠ 1 panggilan `mpc_cost`. Satu iterasi bisa memicu puluhan evaluasi cost + kendala.

---

### LEVEL 3a — Panggilan `mpc_cost`

| | |
|--|--|
| **Fungsi** | **Menilai** satu kandidat rencana rudder `U` |
| **Input** | Vektor `U` (30 sudut rudder), state saat ini, referensi 30 langkah |
| **Output** | Skalar `J` (cost — semakin kecil semakin baik) |
| **Di log** | Kolom **mpc_cost** |
| **File** | `nmpc_constraints.py` → fungsi `mpc_cost()` |

**Isi perhitungan:**

```text
J = Σᵢ [ err_x²·Q₁ + err_y²·Q₂ + err_ψ²·Q₃ ] + Σᵢ [ R · uᵢ² ]
```

dengan `Q = diag(10, 1, 1)`, `R = 1`.

**Analogi:** Mencicipi satu resep — *"kalau bumbu (rudder) begini, rasanya (tracking error) bagaimana?"*

---

### LEVEL 3b — Panggilan `state_constraints` (kendala)

| | |
|--|--|
| **Fungsi** | **Memeriksa** apakah kandidat `U` melanggar batasan yaw rate |
| **Output** | Vektor `c` dengan syarat `c ≤ 0` |
| **Di log** | Kolom **kendala** |
| **File** | `nmpc_constraints.py` → fungsi `state_constraints()` |

SLSQP memanggil ini terpisah dari `mpc_cost` untuk feasibility check dan gradien kendala.

**Analogi:** Cicip resep **dan** cek apakah tidak terlalu "pedas" (yaw rate melebihi batas fisik).

| | mpc_cost | kendala |
|--|----------|---------|
| Tujuan | Seberapa baik tracking? | Apakah aman secara fisik? |
| Output | Skalar `J` | Vektor batasan `c` |
| Propagasi internal | 30 langkah | 30 langkah |

---

### LEVEL 4 — Propagasi horizon (N = 30)

| | |
|--|--|
| **Fungsi** | Mensimulasikan model kapal **30 detik ke depan** di dalam imajinasi optimizer |
| **Per evaluasi** | Selalu **N = 30** langkah Euler |
| **Di log** | Kolom **prop_tot** |
| **File** | Loop `for i in range(N)` di `mpc_cost` dan `state_constraints` |

```python
for i in range(N):
    s = euler_step(s, u[i], dt_nd, A_sys, B_sys, u_0_nd)
    # hitung error atau cek yaw rate
```

**Ini bukan kapal bergerak di dunia nyata** — hanya prediksi untuk menilai kandidat `U`.

**Analogi:** Di pikiran, bayangkan 30 detik ke depan sebelum benar-benar memutar kemudi.

---

### LEVEL 5 — Propagasi nyata plant (di luar optimizer)

| | |
|--|--|
| **Fungsi** | Menerapkan hasil optimasi ke simulasi/plant |
| **Apa yang dilakukan** | Ambil `u₁` dari `U_opt`, lalu `euler_step` **1×** |
| **Frekuensi** | 1 per langkah `t` |
| **Di log** | Tidak per baris; total +150 di ringkasan akhir |

```python
u_applied = U_opt[0]
s_nd = euler_step(s_nd, u_applied, dt_nd, ...)
```

Ini satu-satunya propagasi yang **mengubah** `history_state` simulasi.

**Analogi:** Setelah mencoba banyak resep di pikiran, baru masak **satu langkah** sesuai resep terbaik.

---

### Tabel hubungan level (contoh numerik)

| Level | `t = 0` | `t = 149` | Fungsi singkat |
|-------|---------|-----------|----------------|
| 1 — Langkah `t` | 1× | 1× | Satu siklus kontrol NMPC |
| 2 — SLSQP | 9 | 1 | Cari `U` optimal |
| 3a — mpc_cost | 280 | 31 | Nilai cost |
| 3b — kendala | 321 | 64 | Cek yaw rate |
| 4 — propagasi (×30) | 18.030 | 2.850 | Simulasi prediksi |
| 5 — plant nyata | 1 | 1 | Kapal maju 1 detik |

**Kolom waktu (ms)** = total waktu level 2 + 3 + 4 (level 5 sangat kecil).

---

## 3. Apa arti "terbaik"?

Dalam NMPC, **"terbaik"** punya definisi **matematis**, bukan sekadar intuitif ("paling dekat ke garis").

### 3.1 Definisi formal

Setiap langkah `t`, NMPC mencari vektor:

```text
U = [u₁, u₂, …, u₃₀]
```

**Terbaik** = `U` yang:

```text
meminimalkan J(U)
dengan memenuhi semua batasan (rudder, rate rudder, yaw rate)
```

Itu yang dikerjakan SLSQP setiap detik.

### 3.2 Fungsi biaya J (kriteria "terbaik")

```text
J = Σ [ Q · error² ] + Σ [ R · u² ]
```

| Komponen | Parameter | Arti |
|----------|-----------|------|
| Error x | Q₁ = 10 | Prioritas tinggi — ikuti posisi x referensi |
| Error y | Q₂ = 1 | Koreksi lateral ke garis referensi |
| Error ψ | Q₃ = 1 | Ikuti heading referensi |
| Penalti rudder | R = 1 | Hindari kemudi berlebihan |

**"Terbaik"** = rencana rudder yang:

- membuat prediksi posisi/heading sedekat mungkin ke referensi 30 detik ke depan,
- tanpa memakai rudder lebih besar dari yang perlu,
- dengan error **x** dianggap 10× lebih penting daripada error **y** atau **ψ**.

Mengubah `Q` dan `R` = mengubah **definisi** "terbaik".

### 3.3 Harus memenuhi batasan (feasible)

Rencana tidak dianggap valid meski `J`-nya kecil jika melanggar:

| Batasan | Nilai | Implementasi |
|---------|-------|--------------|
| Sudut rudder | ±35° | `lb`, `ub` |
| Perubahan rudder | ±5°/detik | `du_constraints` → `A_du`, `b_du` |
| Yaw rate | ±0.0932 rad/s | `state_constraints` |

### 3.4 Terbaik untuk 30 detik, dipakai hanya 1 detik (receding horizon)

```text
U_terbaik = [u₁*, u₂*, u₃*, …, u₃₀*]
                ↑
           hanya u₁* yang diterapkan
```

Langkah berikutnya (`t + 1`), optimasi diulang dari awal dengan state baru.

**"Terbaik"** = terbaik untuk **horizon 30 detik saat ini**, bukan terbaik untuk seluruh 150 detik sekaligus.

### 3.5 Optimum lokal, bukan global

SLSQP mencari **optimum lokal** — terbaik di sekitar tebakan awal `U0 = u_prev`.

| Jenis | Arti |
|-------|------|
| Optimum lokal | Terbaik di "lembah" terdekat — yang dicari SLSQP |
| Optimum global | Terbaik di seluruh kemungkinan `U` — **tidak dijamin** |

Warm start (`u_prev`) membantu mendapat solusi bagus lebih cepat.

### 3.6 "Terbaik" per t ≠ RMSE minimum

| | "Terbaik" per `t` | RMSE akhir |
|--|-------------------|------------|
| Definisi | Minimalkan `J` di horizon | √(rata-rata error²) seluruh simulasi |
| Kapan dihitung | Setiap detik | Sekali di akhir |
| Contoh hasil | — | RMSE Y ≈ 37 m |

RMSE Y besar menunjukkan tracking **keseluruhan** belum sempurna — meski setiap langkah sudah "terbaik" menurut `J` saat itu. Perbaikan RMSE biasanya lewat tuning `Q`, `R`, `N`, atau referensi lintasan.

### 3.7 Ringkas satu kalimat per level dan "terbaik"

| Istilah | Arti dalam satu kalimat |
|---------|-------------------------|
| **Terbaik** | `U` yang meminimalkan `J` + memenuhi batasan |
| **Level 1 (`t`)** | Waktunya hitung rudder baru |
| **Level 2 (SLSQP)** | Cari `U` dengan coba-perbaiki berulang |
| **Level 3a (mpc_cost)** | Nilai seberapa buruk satu rencana rudder |
| **Level 3b (kendala)** | Cek apakah rencana melanggar batas yaw rate |
| **Level 4 (N=30)** | Bayangkan kapal 30 detik ke depan (prediksi) |
| **Level 5 (Euler nyata)** | Terapkan rudder pertama, majukan kapal 1 detik |

---

## 4. Struktur file Python

```text
NMPC/Python/
├── nmpc_biasa.py           # Simulasi utama + log per t
├── nmpc_model.py           # Model kapal, Euler, konversi dim/nondim
├── nmpc_constraints.py     # mpc_cost, state_constraints, du_constraints
├── requirements.txt        # numpy, scipy, matplotlib
└── PENJELASAN_NMPC_PYTHON.md   # dokumen ini
```

---

## 5. Referensi kode MATLAB

| Topik | File |
|-------|------|
| Simulasi asli | `NMPC/MATLAB/NMPC_Biasa.m` |
| Port codegen C++ | `NMPC/C++/kode matlab/simulate_nmpc_kapal.m` |
| Validasi C++ | `NMPC/C++/Hasil Matlab.png`, `Hasil C++.png` |

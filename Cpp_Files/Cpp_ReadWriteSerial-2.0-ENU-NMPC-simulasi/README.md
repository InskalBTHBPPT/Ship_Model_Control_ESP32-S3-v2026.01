# Cpp_ReadWriteSerial-2.0-ENU-NMPC-simulasi

Versi **simulasi only** dari `Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta`.

- **Tidak** membuka COM, **tidak** kirim `$HB`, **bukan** untuk kapal.
- Input = file teks berformat sama dengan USB Remote.
- Output = `timestamp,result` (derajat rudder) ke stdout.
- Adaptor NMPC sama: `v=0`, `r` dari `yaw_rate`, `ψ = π/2 + yaw` (IMU **0 = Utara, 270° = Timur**).

Revisi heading sama 2.0 live: dulu `ψ = π/2 − yaw` dan `r_nd` bertanda minus; sekarang `ψ = π/2 + yaw`, `r_nd` ikut `yaw_rate`. Bukan rumus `u`,`v` 1.2.

| Live | Simulasi |
|------|----------|
| `Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta` | folder ini |
| USB CSV + `[WP]` | `scenarios/*.txt` |
| TX ke ESP32 | stdout / file |

---

## Build

```powershell
cd "Cpp_Files\Cpp_ReadWriteSerial-2.0-ENU-NMPC-simulasi"
g++ -std=c++17 -Iinclude -Inmpc src/sim_main.cpp nmpc/nmpc_kapal_waypoint.c nmpc/geo_enu.c nmpc/waypoint_manager.c -o nmpc_sim.exe
```

---

## Penggunaan

```powershell
.\nmpc_sim.exe --scenario scenarios\diam_ke_utara.txt
.\nmpc_sim.exe --scenario scenarios\belok.txt
.\nmpc_sim.exe --scenario scenarios\tanpa_wp.txt --quiet > hasil.csv
```

Simpan hasil:

```powershell
.\nmpc_sim.exe --scenario scenarios\diam_ke_utara.txt --quiet > hasil.csv
```

| Opsi | Keterangan |
|------|------------|
| `--scenario <file>` | Wajib. Format USB. |
| `--r-tran <m>` | Default 3.0 |
| `--quiet` | Hanya `timestamp,result` di stdout (stderr minim) |
| `--help` | Bantuan |

Baris kosong, header `timestamp,...`, dan komentar `#...` (bukan `[WP]`) dilewati.

---

## Skenario bawaan

| File | Isi | `result` yang diharapkan |
|------|-----|--------------------------|
| `scenarios/diam_ke_utara.txt` | Home + WP utara, `yaw=0` (IMU Utara) | \|result\| kecil |
| `scenarios/belok.txt` | WP timur, `yaw=0` (hadap utara), lalu `yaw_rate=30` | \|result\| besar, ±45 |
| `scenarios/tanpa_wp.txt` | CSV saja, satu baris GPS 0 | semua `0.00` |

`yaw=0` (Utara) → `ψ = π/2`. `yaw=90` = Barat, `yaw=270` = Timur.

---

## Format file

```text
[WP] Home: -7.287150, 112.796000
[WP] #1: -7.286750, 112.796000
0.000,-7.287150,112.796000,0.00,0.00,0.00,0.00,0.00
```

Kolom CSV sama Remote: `timestamp,lat,lon,servo1,servo2,yaw,gyro_z,yaw_rate`.  
`gyro_z` dan servo diabaikan.

`yaw` IMU: **0 = Utara, 90 = Barat, 180 = Selatan, 270 = Timur**.

Konversi ke NMPC (0 = Timur, CCW):

```text
ψ_nmpc = wrap(π/2 + deg2rad(yaw))
r_nd   = +deg2rad(yaw_rate) · L / u0
```

---

## Catatan

1. Logika tick ada di `include/nmpc_tick.hpp` (salinan aturan 2.0). Jika 2.0 live diubah, samakan file ini.
2. Heading: **0 = Utara, 270° = Timur**. `ψ = π/2 + yaw`, `r_nd` ikut tanda `yaw_rate` (sama 2.0 live).
3. Tidak ada plant WyNDA; ini **replay terbuka**.
4. Tick pertama vs `nmpc_demo` C bisa dekat (`v=0`); langkah berikutnya C mengisi `v` dari model.

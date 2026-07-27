# Ship Auto Way Maps Points Tracking

Dashboard PySide6 untuk monitoring telemetry kapal model secara real-time, perencanaan waypoint, dan pengiriman konfigurasi ke Remote via User-Side ESP32.

## File utama

| File | Deskripsi |
|------|-----------|
| `Local Monitor Dashboard-beta1.1.1.py` | Versi aktif (beta 1.1.1) |
| `Local Monitor Dashboard-beta1.1.py` | Versi sebelumnya |

## Arsitektur

```
Dashboard  --USB serial-->  User-Side ESP32  --ESP-NOW-->  Remote-Side (kapal)
```

## Tab aplikasi

### Live Data

- Peta tracking posisi kapal
- Indikator live (yaw, heading, rudder, RPM, baterai, speed, WP index)
- Time-series plots
- Logging CSV (nilai tampilan, bukan raw)
- Setup koreksi rudder sensor
- Tampilan route Home → WP (setelah Send to Remote)

### Map Points

- Klik peta untuk menambah waypoint
- **Set Home Point** — Home dari koordinat serial terbaru
- Panel **Send to Remote** — kirim WP + tuning + verifikasi read-back
- Panel **Setup** (di bawah Send to Remote) — dialog algoritma auto + tuning Alg 1
- Tabel waypoint + garis route biru di peta
- Snapshot waypoint ke folder `WayPoints/` (CSV)

### Analyze

- Load CSV log (format display / raw / legacy)
- Peta + 3 plot + panel indikator
- Slider timeline untuk replay data

## Protokol serial (ke User-Side)

### Kirim waypoint

```
$WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...
```

### Kirim tuning

```
$TUNSET,<alg>[,kp,kd,arrive_m,rudder_max]
```

Default alg **2** (stub). Alg 1 membutuhkan 4 parameter.

### Read-back tuning

```
$TUNGET
```

### Respons yang di-handle dashboard

| Pola | Makna |
|------|-------|
| `$WACK,OK,WP` | Waypoint diterima Remote |
| `$WACK,OK,TUN` | Tuning disimpan NVS |
| `$WACK,ERR,...` | Error WP/TUN |
| `$TACK,<alg>,...` | Read-back sukses |
| `$TACK,ERR,...` | Read-back gagal |

### Alur Send to Remote

1. Validasi: connected, Home set, minimal Home + 2 WP
2. `$WPSET` → `$WACK,OK,WP`
3. `$TUNSET` → `$WACK,OK,TUN`
4. `$TUNGET` → bandingkan `$TACK` dengan Setup
5. Status: **Verified OK** / **MISMATCH** / **TIMEOUT**

Timeout per langkah: **3 detik**.

## Telemetry masuk (23 kolom)

Raw fixed-point dari User-Side; dashboard menampilkan nilai fisik:

| Kolom | Skala tampilan |
|-------|----------------|
| timestamp, lat, lon | asli |
| speedMps, yaw, heading, rudder, servo, accel, gyro, rpm, battery | ÷ 100 |
| distance_to_wp | ÷ 10 |
| track_wp_index, mode_auto | integer |

## Setup dialog (Map Points)

- Radio **Alg 1** (waypoint + PD) / **Alg 2** (stub, default)
- Spinbox Alg 1: Kp, Kd, WP arrive (m), Rudder max (°)
- **Read from Remote** — `$TUNGET`, update UI dari `$TACK`

## Menjalankan

```bash
cd Pythonfile/Way_Points_Tracking
python "Local Monitor Dashboard-beta1.1.1.py"
```

### Dependencies

- Python 3.10+
- PySide6
- pyserial
- PyQtWebEngine (peta)

## Folder output

- `WayPoints/` — snapshot CSV waypoint saat Send to Remote
- Log CSV — lokasi sesuai pengaturan tab Live

## Catatan

- Pastikan MAC ESP-NOW Remote ↔ User-Side cocok sebelum uji lapangan
- Connect serial ke port User-Side (115200 baud)
- CH6 RC ≥ 1750 untuk mode Auto di kapal

## Author

Chandra P — Ship Model Control System

## Versi

- **beta 1.1.1** — waypoint send, tuning NVS, verify TACK, Analyze tab

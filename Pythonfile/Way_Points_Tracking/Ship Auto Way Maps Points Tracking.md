# Ship Auto Way Maps Points Tracking

Dokumentasi sistem **Ship Auto Way Maps Points Tracking** — kontrol kapal model ESP32-S3 dengan waypoint, telemetry real-time, NMPC di mini PC, dan dashboard PySide6.

**Versi dokumen:** dashboard 1.6 + Remote/User-05 + Cpp 2.0  
**Last update:** 2026-10

Dokumen dashboard: `README Local Monitor Dashboard-beta1.6.md`

---

## Daftar isi

1. [Ringkasan sistem](#1-ringkasan-sistem)
2. [Komponen & path proyek](#2-komponen--path-proyek)
3. [Arsitektur & alur data](#3-arsitektur--alur-data)
4. [Remote-Side-05](#4-remote-side-05)
5. [User-Side-05](#5-user-side-05)
6. [Dashboard beta 1.6](#6-dashboard-beta-16)
7. [Mini PC — Cpp_ReadWriteSerial-2.0](#7-mini-pc--cpp_readwriteserial-20)
8. [Protokol serial (Dashboard ↔ User-Side)](#8-protokol-serial-dashboard--user-side)
9. [Protokol ESP-NOW](#9-protokol-esp-now)
10. [Telemetry 24 kolom](#10-telemetry-24-kolom)
11. [Yaw, setpoint, u dan v](#11-yaw-setpoint-u-dan-v)
12. [Algoritma auto track](#12-algoritma-auto-track)
13. [Build, upload & menjalankan](#13-build-upload--menjalankan)
14. [Prosedur uji lapangan](#14-prosedur-uji-lapangan)
15. [Troubleshooting](#15-troubleshooting)

---

## 1. Ringkasan sistem

Sistem ini memungkinkan:

- **Monitoring live** posisi, yaw kompas, rudder, RPM, baterai, status Mini PC, serta surge `u` / sway `v` (dihitung di dashboard)
- **Perencanaan waypoint** di peta (Home + hingga 10 waypoint navigasi)
- **Pengiriman waypoint** ke kapal via ESP-NOW (`0xA1`)
- **Kontrol auto alg 2** — rudder dari NMPC mini PC (`timestamp,result`)
- **Kontrol auto alg 1** — waypoint + PD (opsional, compile-time)
- **Shutdown mini PC** dari dashboard (ESP-NOW `0xA2`, tanpa Wi‑Fi laptop↔mini PC)
- **Analisis log CSV** — replay data dengan peta dan plot

Alur end-to-end:

```text
Dashboard beta 1.6 (PySide6)
    │ USB serial 115200
    ▼
User-Side-05  (ESP-Now_ESP32-S3_User-Side-05)
    │ ESP-NOW peer-to-peer
    ▼
Remote-Side-05 (ESP-Now_ESP32-S3_Remote-Side-05) — di kapal
    │ USB Serial 115200
    ▼
Mini PC — Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta
```

---

## 2. Komponen & path proyek

| Komponen | Path (dari root repo) |
|----------|----------------------|
| Dashboard | `Pythonfile/Way_Points_Tracking/Local Monitor Dashboard-beta1.6.py` |
| User-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-05/` |
| Remote-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05/` |
| Mini PC | `Cpp_Files/Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta/` |
| Dokumen ini | `Pythonfile/Way_Points_Tracking/Ship Auto Way Maps Points Tracking.md` |

`Cpp_ReadWriteSerial-2.1-ENU-NMPC-beta` adalah cabang yang menghitung `u`,`v` dari GPS. Stack di dokumen ini memakai **2.0** (`v = 0`, `u0 = 0.6114` m/s).

---

## 3. Arsitektur & alur data

### 3.1 Telemetry (kapal → laptop)

1. Remote baca sensor ~10 Hz → struct `DatatoSend` (64 byte, 24 field)
2. ESP-NOW ke User-Side
3. User-Side cetak CSV 24 kolom ke USB
4. Dashboard parse → Live / log / plot, lalu hitung `u`,`v` lokal

### 3.2 Waypoint (laptop → kapal → mini PC)

1. Dashboard: `$WPSET,...`
2. User-Side → ESP-NOW `0xA1` → Remote simpan RAM + cetak `[WP] ...`
3. User-Side balas `$WACK,OK` / `$WACK,ERR,...`
4. Mini PC 2.0 memakai `[WP] Home` sebagai origin ENU dan `[WP] #n` sebagai target

### 3.3 Rudder NMPC (auto alg 2)

1. Remote (CH6 auto) kirim CSV 8 kolom ke mini PC
2. 2.0 memakai `timestamp`, `lat`, `lon`, `yaw`, `yaw_rate`, dan daftar `[WP]`
3. 2.0 mengabaikan `calc_deg_servo_*` dan `gyro_z`
4. Mini PC kirim `$HB` (~1 Hz) + `timestamp,result` (offset rudder, derajat)
5. Remote set `mini_pc_link` dari heartbeat; pakai `result` sebagai offset rudder

`ψ`, bearing, `u`, dan `v` tidak dikirim balik ke Remote.

### 3.4 Shutdown mini PC

1. Dashboard tombol **Shutdown** (hanya jika Mini PC CONNECTED)
2. `$SHUTDOWN` → User → ESP-NOW `0xA2` → Remote → Serial `$SHUTDOWN`
3. `Cpp_ReadWriteSerial-2.0` jalankan `shutdown /s /t 5`
4. User balas `$SACK,OK` (forward ESP-NOW sukses — bukan konfirmasi OS mati)

---

## 4. Remote-Side-05

**Proyek:** `ESP-Now_ESP32-S3_Remote-Side-05`

- Sensor, actuator, RC PPM, waypoint RAM, telemetry ESP-NOW 24 kolom
- Yaw kapal: raw JY901 (−180…180) → wrap 0…360 → +90° pasang → `360 − yaw` = kompas CW (0 Utara, 90 Timur, 270 Barat)
- USB Serial ke mini PC: CSV 8 kolom, `[WP]`, `$SHUTDOWN`; terima `$HB` + `timestamp,result`
- Default `#define AUTO_TRACK_ALG 2` (mini PC / NMPC)
- Alg 2: `heading_setpoint` = salinan yaw, `heading_error` = 0
- Alg 1: `heading_setpoint` = bearing ke waypoint aktif

Detail: `PlatformIO/.../Remote-Side-05/src/README.md`

---

## 5. User-Side-05

**Proyek:** `ESP-Now_ESP32-S3_User-Side-05`

- Gateway USB ↔ ESP-NOW. Tidak terhubung ke mini PC
- Forward `$WPSET` → `0xA1`, `$SHUTDOWN` → `0xA2`
- CSV 24 kolom ke dashboard. `yaw` dan `heading_setpoint` diteruskan mentah (×100)

Detail: `PlatformIO/.../User-Side-05/src/README.md`

---

## 6. Dashboard beta 1.6

**File:** `Local Monitor Dashboard-beta1.6.py`

- Live: mode, Mini PC CONNECTED/DISCONNECTED, warning auto tanpa mini PC
- Tombol **Shutdown** sebelah status Mini PC (enable jika Connect + `mini_pc_link=1`)
- Map Points: Home + waypoints, **Send Way Points** (`$WPSET` / `$WACK`)
- Live: **u surge**, **v sway** (hitung lokal; bukan dari firmware)
- Logging & Analyze: CSV 24 kolom ditambah `u (m/s)`, `v (m/s)`
- Plot Heading Setpoint pada alg 2 menempel pada yaw

Detail: `README Local Monitor Dashboard-beta1.6.md`

---

## 7. Mini PC — Cpp_ReadWriteSerial-2.0

**Path:** `Cpp_Files/Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta/`

| Arah | Isi |
|------|-----|
| Terima | CSV 8 kolom saat CH6 auto; `[WP] Home` / `[WP] #n`; `$SHUTDOWN` |
| Pakai | `timestamp`, `lat`, `lon`, `yaw`, `yaw_rate`, daftar WP |
| Abaikan | `calc_deg_servo_1/2`, `gyro_z` |
| Kirim | `$HB` tiap 1 s; `timestamp,result` (rudder °, ±45) |

State NMPC: `v = 0`, `u0 = 0.6114` m/s, `ψ = π/2 − yaw`, `r` dari `yaw_rate` (tanda minus). `L = 1.0107` m.

Detail: `Cpp_Files/Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta/README.md`

---

## 8. Protokol serial (Dashboard ↔ User-Side)

| Arah | Format |
|------|--------|
| User → PC | CSV 24 kolom telemetry |
| PC → User | `$WPSET,<home_lat>,<home_lon>,<count>,...` |
| User → PC | `$WACK,OK` / `$WACK,ERR,<reason>` |
| PC → User | `$SHUTDOWN` |
| User → PC | `$SACK,OK` / `$SACK,ERR,<reason>` |

Baud: **115200**.

Serial Remote ↔ mini PC terpisah (bukan lewat User-Side):

```text
timestamp,lat,lon,calc_deg_servo_1,calc_deg_servo_2,yaw,gyro_z,yaw_rate
[WP] Home: lat, lon
[WP] #1: lat, lon
$HB
timestamp,result
$SHUTDOWN
```

---

## 9. Protokol ESP-NOW

| msg_type | Payload | Arah | Fungsi |
|----------|---------|------|--------|
| (telemetry) | `DatatoSend` 64 B | Remote → User | Telemetry 24 field |
| `0xA1` | `waypoints_payload` ~180 B | User → Remote | Waypoint + home |
| `0xA2` | `pc_command_payload` 4 B | User → Remote | Perintah mini PC (`cmd=1` shutdown) |

**Catatan:** `0xA2` di versi 05 = shutdown mini PC (bukan tuning NVS dokumen lama).

---

## 10. Telemetry 24 kolom

Urutan sama di Remote `DatatoSend`, User CSV, dan dashboard:

1 timestamp, 2 lat, 3 lon, 4 speedMps×100, 5–6 servo×100, 7 yaw×100,  
8 hdg_sp×100, 9 hdg_err×100, 10 rudder_cmd×100, 11 track_wp_index,  
12 distance_to_wp×10, 13–18 IMU×100, 19–20 RPM, 21–22 battery×100,  
23 mode_auto, **24 mini_pc_link**

`mode_auto`: 0 Manual, 1 Auto PD, 2 Auto Mini PC.

Log dashboard menambah `u (m/s)` dan `v (m/s)` setelah `speedMps`. Kedua kolom itu tidak ada di firmware.

---

## 11. Yaw, setpoint, u dan v

**Yaw** hanya diubah di Remote:

```text
raw (−180…180) → wrap 0…360 → +90° → 360 − yaw
```

Hasil di CSV: 0 = Utara, 90 = Timur, 270 = Barat. User-Side dan dashboard hanya ÷100.

**Heading setpoint** bukan mode zigzag. Pada alg 2 isinya salinan yaw. Bearing ke waypoint hanya diisi jika firmware di-compile dengan `AUTO_TRACK_ALG 1`.

**`u`, `v` di dashboard 1.6** (kompas CW):

```text
u = ẋ sinψ + ẏ cosψ
v = ẋ cosψ − ẏ sinψ
```

2.0 tidak memakai rumus ini.

---

## 12. Algoritma auto track

Dipilih compile-time di Remote (`AUTO_TRACK_ALG`):

| Nilai | Perilaku |
|-------|----------|
| 1 | Waypoint haversine + PD rudder. `heading_setpoint` = bearing |
| 2 (default) | Rudder dari mini PC `timestamp,result`. Setpoint = yaw |

CH6 ≥ 1750 = Auto; jika alg 2 dan `mini_pc_link=0` → rudder netral + warning.

CH6 manual ↔ auto tidak mereset indeks waypoint. Reset ke WP1 hanya lewat **Send Way Points** baru atau restart program 2.0.

---

## 13. Build, upload & menjalankan

```bash
# Firmware
cd PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05
pio run --target upload
cd ../ESP-Now_ESP32-S3_User-Side-05
pio run --target upload

# Mini PC 2.0
cd Cpp_Files/Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta
g++ -std=c++17 -Iinclude -Inmpc src/main.cpp src/serial_port.cpp nmpc/nmpc_kapal_waypoint.c nmpc/geo_enu.c nmpc/waypoint_manager.c -o read_write_serial.exe
.\read_write_serial.exe --port COMx --baud 115200 --rudder-mode nmpc --print all

# Dashboard
cd Pythonfile/Way_Points_Tracking
python "Local Monitor Dashboard-beta1.6.py"
```

Sesuaikan MAC ESP-NOW di kedua `main.cpp` dan port COM.

---

## 14. Prosedur uji lapangan

1. Flash **Remote-05** (rantai yaw CW) + **User-05** berpasangan
2. Jalankan **read_write_serial.exe** 2.0 di mini PC (auto-start opsional)
3. Connect dashboard beta **1.6** ke User-Side
4. Verifikasi Live: telemetry + Mini PC **CONNECTED**
5. Map Points → Set Home + ≥1 WP → **Send Way Points** → `$WACK,OK`; cek `[WP]` di stderr/stdout 2.0
6. RC CH6 Auto → pantau rudder dari `timestamp,result`
7. (Opsional) **Shutdown** → konfirmasi → `$SACK,OK` → mini PC mati ~5 s

---

## 15. Troubleshooting

| Gejala | Tindakan |
|--------|----------|
| Mini PC DISCONNECTED | Cek USB Remote↔PC, jalankan exe 2.0, baud 115200 |
| `$WACK` TIMEOUT | MAC ESP-NOW, Remote power, jarak |
| Auto Mini PC tidak gerak | CH6 high, `mini_pc_link=1`, timestamp CSV cocok dengan balasan |
| Heading setpoint = yaw | Normal pada alg 2. Bearing hanya ada di alg 1 |
| `u`,`v` aneh | Remote belum di-flash yaw CW, atau log lama (skala IMU) |
| Shutdown tombol abu-abu | Harus Connect + CONNECTED |
| `$SACK,OK` tapi PC tidak mati | Pastikan exe **2.0** yang menangani `$SHUTDOWN` |
| Telemetry 23 kolom | Flash User/Remote-05; dashboard 1.6 tetap terima 23/24 |

---

## Diagram alur

```text
┌──────────────────┐  $WPSET / $SHUTDOWN  ┌──────────────┐  0xA1 / 0xA2  ┌───────────────┐
│ Dashboard beta   │ ───────────────────► │ User-Side-05 │ ────────────► │ Remote-Side-05│
│ 1.6              │ ◄─────────────────── │              │ ◄──────────── │ yaw kompas CW │
└──────────────────┘  CSV24 / $WACK/$SACK └──────────────┘  telemetry 24 └───────┬───────┘
                                                                                  │ USB
                                                                                  ▼
                                                                          ┌───────────────┐
                                                                          │ Mini PC 2.0   │
                                                                          │ NMPC          │
                                                                          │ CSV8 / [WP] / │
                                                                          │ $HB, result   │
                                                                          └───────────────┘
```

---

*Dokumen ini: Ship Auto Way Maps Points Tracking — dashboard 1.6, Remote/User-05, Cpp_ReadWriteSerial-2.0-ENU-NMPC-beta*

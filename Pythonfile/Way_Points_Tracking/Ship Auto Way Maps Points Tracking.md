# Ship Auto Way Maps Points Tracking

Dokumentasi sistem **Ship Auto Way Maps Points Tracking** — kontrol kapal model ESP32-S3 dengan waypoint, telemetry real-time, mini PC rudder, dan dashboard PySide6.

**Versi dokumen:** beta 1.5 | **Author:** Chandra P — Ship Model Control System  
**Last update:** 2026-08

---

## Daftar isi

1. [Ringkasan sistem](#1-ringkasan-sistem)
2. [Komponen & path proyek](#2-komponen--path-proyek)
3. [Arsitektur & alur data](#3-arsitektur--alur-data)
4. [Remote-Side-05](#4-remote-side-05)
5. [User-Side-05](#5-user-side-05)
6. [Dashboard beta 1.5](#6-dashboard-beta-15)
7. [Mini PC — Cpp_ReadWriteSerial-1.0](#7-mini-pc--cpp_readwriteserial-10)
8. [Protokol serial (Dashboard ↔ User-Side)](#8-protokol-serial-dashboard--user-side)
9. [Protokol ESP-NOW](#9-protokol-esp-now)
10. [Telemetry 24 kolom](#10-telemetry-24-kolom)
11. [Algoritma auto track](#11-algoritma-auto-track)
12. [Build, upload & menjalankan](#12-build-upload--menjalankan)
13. [Prosedur uji lapangan](#13-prosedur-uji-lapangan)
14. [Troubleshooting](#14-troubleshooting)

---

## 1. Ringkasan sistem

Sistem ini memungkinkan:

- **Monitoring live** posisi, heading, rudder, RPM, baterai, status Mini PC
- **Perencanaan waypoint** di peta (Home + hingga 10 waypoint navigasi)
- **Pengiriman waypoint** ke kapal via ESP-NOW (`0xA1`)
- **Kontrol auto alg 2** — rudder dari mini PC (`timestamp,result`)
- **Kontrol auto alg 1** — waypoint + PD (opsional, compile-time)
- **Shutdown mini PC** dari dashboard (ESP-NOW `0xA2`, tanpa Wi‑Fi laptop↔mini PC)
- **Analisis log CSV** — replay data dengan peta dan plot

Alur end-to-end:

```text
Dashboard beta 1.5 (PySide6)
    │ USB serial 115200
    ▼
User-Side-05  (ESP-Now_ESP32-S3_User-Side-05)
    │ ESP-NOW peer-to-peer
    ▼
Remote-Side-05 (ESP-Now_ESP32-S3_Remote-Side-05) — di kapal
    │ USB Serial 115200
    ▼
Mini PC — Cpp_ReadWriteSerial-1.0
```

---

## 2. Komponen & path proyek

| Komponen | Path (dari root repo) |
|----------|----------------------|
| Dashboard | `Pythonfile/Way_Points_Tracking/Local Monitor Dashboard-beta1.5.py` |
| User-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-05/` |
| Remote-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05/` |
| Mini PC | `Cpp_Files/Cpp_ReadWriteSerial-1.0/` |
| Dokumen ini | `Pythonfile/Way_Points_Tracking/Ship Auto Way Maps Points Tracking.md` |

Versi sebelumnya (referensi): User/Remote-04, Dashboard beta 1.4, `Cpp_ReadWriteSerial` (tanpa shutdown).

---

## 3. Arsitektur & alur data

### 3.1 Telemetry (kapal → PC)

1. Remote baca sensor ~10 Hz → struct `DatatoSend` (64 byte, 24 field)
2. ESP-NOW ke User-Side
3. User-Side cetak CSV 24 kolom ke USB
4. Dashboard parse → Live / log / plot

### 3.2 Waypoint (PC → kapal → mini PC)

1. Dashboard: `$WPSET,...`
2. User-Side → ESP-NOW `0xA1` → Remote simpan RAM + cetak `[WP] ...`
3. User-Side balas `$WACK,OK` / `$WACK,ERR,...`
4. Mini PC (`--print all|wp`) menampilkan `[WP]`

### 3.3 Rudder mini PC (auto alg 2)

1. Remote (CH6 auto) kirim CSV 8 kolom ke mini PC
2. Mini PC kirim `$HB` (~1 Hz) + `timestamp,result`
3. Remote set `mini_pc_link` dari heartbeat; pakai `result` sebagai offset rudder

### 3.4 Shutdown mini PC

1. Dashboard tombol **Shutdown** (hanya jika Mini PC CONNECTED)
2. `$SHUTDOWN` → User → ESP-NOW `0xA2` → Remote → Serial `$SHUTDOWN`
3. `Cpp_ReadWriteSerial-1.0` jalankan `shutdown /s /t 5`
4. User balas `$SACK,OK` (forward ESP-NOW sukses — bukan konfirmasi OS mati)

---

## 4. Remote-Side-05

**Proyek:** `ESP-Now_ESP32-S3_Remote-Side-05` (dari Remote-Side-04)

- Sensor, actuator, RC PPM, waypoint RAM, telemetry ESP-NOW 24 kolom
- USB Serial ke mini PC: CSV 8 kolom, `[WP]`, `$SHUTDOWN`; terima `$HB` + `timestamp,result`
- Default `#define AUTO_TRACK_ALG 2` (mini PC)

Detail: `PlatformIO/.../Remote-Side-05/src/README.md`

---

## 5. User-Side-05

**Proyek:** `ESP-Now_ESP32-S3_User-Side-05` (dari User-Side-04)

- Gateway USB ↔ ESP-NOW
- Forward `$WPSET` → `0xA1`, `$SHUTDOWN` → `0xA2`
- CSV 24 kolom ke dashboard

Detail: `PlatformIO/.../User-Side-05/src/README.md`

---

## 6. Dashboard beta 1.5

**File:** `Local Monitor Dashboard-beta1.5.py` (dari beta 1.4)

- Live: mode, Mini PC CONNECTED/DISCONNECTED, warning auto tanpa mini PC
- Tombol **Shutdown** sebelah status Mini PC (enable jika Connect + `mini_pc_link=1`)
- Map Points: Home + waypoints, **Send Way Points** (`$WPSET` / `$WACK`)
- Logging & Analyze CSV 24 kolom

---

## 7. Mini PC — Cpp_ReadWriteSerial-1.0

**Path:** `Cpp_Files/Cpp_ReadWriteSerial-1.0/` (dari `Cpp_ReadWriteSerial`)

| Fitur | Keterangan |
|-------|------------|
| `$HB` | Heartbeat ke Remote ~1 Hz |
| CSV 8 kolom | Baca + hitung rudder (`--rudder-mode`) + tulis `timestamp,result` |
| `[WP]` | Print ke stdout (`--print all\|wp`) |
| `$SHUTDOWN` | Matikan OS Windows/Linux |
| `--print` | `all` / `csv` / `wp` |

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

---

## 11. Algoritma auto track

Dipilih compile-time di Remote (`AUTO_TRACK_ALG`):

| Nilai | Perilaku |
|-------|----------|
| 1 | Waypoint haversine + PD rudder |
| 2 (default) | Rudder dari mini PC `timestamp,result` |

CH6 ≥ 1750 = Auto; jika alg 2 dan `mini_pc_link=0` → rudder netral + warning.

---

## 12. Build, upload & menjalankan

```bash
# Firmware
cd PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05
pio run --target upload
cd ../ESP-Now_ESP32-S3_User-Side-05
pio run --target upload

# Mini PC
cd Cpp_Files/Cpp_ReadWriteSerial-1.0
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
.\read_write_serial.exe --port COMx --baud 115200 --rudder-mode yawrate2 --print all

# Dashboard
cd Pythonfile/Way_Points_Tracking
python "Local Monitor Dashboard-beta1.5.py"
```

Sesuaikan MAC ESP-NOW di kedua `main.cpp` dan port COM.

---

## 13. Prosedur uji lapangan

1. Flash **Remote-05** + **User-05** berpasangan
2. Jalankan **Cpp_ReadWriteSerial-1.0** di mini PC (auto-start opsional)
3. Connect dashboard beta **1.5** ke User-Side
4. Verifikasi Live: telemetry + Mini PC **CONNECTED**
5. Map Points → Set Home + ≥2 WP → **Send Way Points** → `$WACK,OK`; cek `[WP]` di mini PC
6. RC CH6 Auto → pantau rudder dari mini PC
7. (Opsional) **Shutdown** → konfirmasi → `$SACK,OK` → mini PC mati ~5 s

---

## 14. Troubleshooting

| Gejala | Tindakan |
|--------|----------|
| Mini PC DISCONNECTED | Cek USB Remote↔PC, jalankan exe 1.0, baud 115200 |
| `$WACK` TIMEOUT | MAC ESP-NOW, Remote power, jarak |
| Auto Mini PC tidak gerak | CH6 high, `mini_pc_link=1`, timestamp match |
| Shutdown tombol abu-abu | Harus Connect + CONNECTED |
| `$SACK,OK` tapi PC tidak mati | Pastikan exe **1.0** (bukan folder lama tanpa `$SHUTDOWN`) |
| Telemetry 23 kolom | Flash User/Remote-05; dashboard 1.5 tetap terima 23/24 |

---

## Diagram alur (versi 05 / 1.5)

```text
┌──────────────────┐  $WPSET / $SHUTDOWN  ┌──────────────┐  0xA1 / 0xA2  ┌───────────────┐
│ Dashboard beta   │ ───────────────────► │ User-Side-05 │ ────────────► │ Remote-Side-05│
│ 1.5              │ ◄─────────────────── │              │ ◄──────────── │               │
└──────────────────┘  CSV24 / $WACK/$SACK └──────────────┘  telemetry 24 └───────┬───────┘
                                                                                  │ USB
                                                                                  ▼
                                                                          ┌───────────────┐
                                                                          │ Mini PC       │
                                                                          │ ReadWrite     │
                                                                          │ Serial-1.0    │
                                                                          │ CSV / [WP] /  │
                                                                          │ $SHUTDOWN     │
                                                                          └───────────────┘
```

---

*Dokumen ini: Ship Auto Way Maps Points Tracking — beta 1.5 (Remote/User-05 + Cpp_ReadWriteSerial-1.0)*

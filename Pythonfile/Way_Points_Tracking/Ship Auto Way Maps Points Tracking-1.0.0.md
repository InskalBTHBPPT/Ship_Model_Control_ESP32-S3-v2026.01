# Ship Auto Way Maps Points Tracking

Dokumentasi sistem **Ship Auto Way Maps Points Tracking** — kontrol kapal model ESP32-S3 dengan waypoint, telemetry real-time, NMPC di mini PC, dan dashboard PySide6.

**Versi dokumen:** dashboard 1.8 + User-05 + Remote-05.2 + Cpp 2.3  
**Last update:** 2026-10-10

Dokumen dashboard: `README Local Monitor Dashboard-1.8.md`

---

## Daftar isi

1. [Ringkasan sistem](#1-ringkasan-sistem)
2. [Komponen & path proyek](#2-komponen--path-proyek)
3. [Arsitektur & alur data](#3-arsitektur--alur-data)
4. [Remote-Side-05.2](#4-remote-side-052)
5. [User-Side-05](#5-user-side-05)
6. [Dashboard 1.8](#6-dashboard-18)
7. [Mini PC — Cpp_ReadWriteSerial-2.3](#7-mini-pc--cpp_readwriteserial-23)
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
- **Kontrol auto alg 2** — rudder dan bearing dari NMPC mini PC (`timestamp,result,bearing`)
- **Baterai rendah** — Remote-05.2 mengirim `$RTL` setelah 10 detik di bawah 10,8 V saat auto; 2.3 mengarahkan ke Home. Dashboard 1.8 hanya alarm
- **Kontrol auto alg 1** — waypoint + PD (opsional, compile-time)
- **Propeller** — manual dari CH3/CH5; saat auto menahan nilai terakhir sebelum pindah mode
- **Shutdown mini PC** dari dashboard (ESP-NOW `0xA2`, tanpa Wi‑Fi laptop↔mini PC)
- **Analisis log CSV** — replay data dengan peta dan plot
- **Replay 3D dan prediksi lintasan** — jejak uji di bingkai ENU; garis merah dari model NMPC 2.3 (Home, yaw awal timur)

Alur end-to-end:

```text
Dashboard 1.8 (PySide6)
    │ USB serial 115200
    ▼
User-Side-05  (ESP-Now_ESP32-S3_User-Side-05)
    │ ESP-NOW peer-to-peer
    ▼
Remote-Side-05.2 (ESP-Now_ESP32-S3_Remote-Side-05.2) — di kapal
    │ USB Serial 115200
    ▼
Mini PC — Cpp_ReadWriteSerial-2.3-ENU-NMPC
```

---

## 2. Komponen & path proyek

| Peran | Proyek | Path (dari root repo) |
|-------|--------|------------------------|
| Dashboard | `Local Monitor Dashboard-1.8.py` | `Pythonfile/Way_Points_Tracking/` |
| Laptop, jembatan ESP-NOW | `ESP-Now_ESP32-S3_User-Side-05` | `PlatformIO/Way_Points_Tracking/` |
| Kapal | `ESP-Now_ESP32-S3_Remote-Side-05.2` | `PlatformIO/Way_Points_Tracking/` |
| Mini PC | `Cpp_ReadWriteSerial-2.3-ENU-NMPC` | `Cpp_Files/` |
| Dokumen ini | `Ship Auto Way Maps Points Tracking.md` | `Pythonfile/Way_Points_Tracking/` |

2.3 turunan **2.2** (dan 2.2 turunan 2.0, bukan 2.1): `v = 0`, `u0 = 0.6114` m/s. 2.3 menambah `$RTL`: target NMPC pindah ke Home. `Cpp_ReadWriteSerial-2.1-ENU-NMPC-beta` menghitung `u`,`v` dari GPS dan **bukan** pasangan kapal ini.

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
4. Mini PC 2.3 memakai `[WP] Home` sebagai origin ENU dan `[WP] #n` sebagai target

### 3.3 Rudder NMPC (auto alg 2)

1. Remote (CH6 auto) kirim CSV 8 kolom ke mini PC
2. 2.3 memakai `timestamp`, `lat`, `lon`, `yaw`, `yaw_rate`, dan daftar `[WP]`
3. 2.3 mengabaikan `calc_deg_servo_*` dan `gyro_z`
4. Mini PC kirim `$HB` (~1 Hz). Jika GPS dan waypoint siap, balasan `timestamp,result,bearing`; jika tidak, `timestamp,result` saja
5. Remote set `mini_pc_link` dari heartbeat, memakai `result` sebagai offset rudder, dan — bila kolom ketiga ada — menyalin `bearing` ke `heading_setpoint`
6. Jika Remote sudah mengunci baterai rendah, ia mengirim `$RTL`. 2.3 mengganti target menjadi Home dan bearing mengikuti Home

`u` dan `v` tidak dikirim balik ke Remote. Bearing dihitung di mini PC (`wrap360(90° − θ)`), skala kompas CW.

### 3.4 Shutdown mini PC

1. Dashboard tombol **Shutdown** (hanya jika Mini PC CONNECTED)
2. `$SHUTDOWN` → User → ESP-NOW `0xA2` → Remote → Serial `$SHUTDOWN`
3. `Cpp_ReadWriteSerial-2.3` jalankan `shutdown /s /t 5`
4. User balas `$SACK,OK` (forward ESP-NOW sukses — bukan konfirmasi OS mati)

---

## 4. Remote-Side-05.2

**Proyek:** `ESP-Now_ESP32-S3_Remote-Side-05.2`  
Turunan Remote-Side-05.1. Pasangan mini PC: 2.3.

- Sensor, actuator, RC PPM, waypoint RAM, telemetry ESP-NOW 24 kolom
- Yaw kapal: raw JY901 (−180…180) → wrap 0…360 → +90° pasang → `360 − yaw` = kompas CW (0 Utara, 90 Timur, 270 Barat)
- USB Serial ke mini PC: CSV 8 kolom, `[WP]`, `$SHUTDOWN`; terima `$HB` + `timestamp,result` atau `timestamp,result,bearing`
- Default `#define AUTO_TRACK_ALG 2` (mini PC / NMPC)
- Alg 2, ada kolom bearing: `heading_setpoint` = bearing, `heading_error` = bearing − yaw
- Alg 2, tanpa kolom ketiga: `heading_setpoint` = salinan yaw, `heading_error` = 0
- Alg 1: `heading_setpoint` = bearing haversine ke waypoint aktif (dihitung di Remote)
- Propeller CH3 (kecepatan) dan CH5 (arah): saat manual mengikuti stik. Saat CH6 pindah ke auto, PWM menahan nilai tick manual terakhir. Gerakan CH3/CH5 selama auto diabaikan sampai mode kembali manual. Jika kapal dinyalakan sudah di auto, yang ditahan adalah pembacaan tick auto pertama
- Baterai: `battery_1` atau `battery_2` di bawah 10,8 V selama 10 detik terus-menerus saat CH6 auto → kunci pulang Home dan kirim `$RTL` tiap 1 detik. Kunci tidak lepas jika tegangan naik. Propeller tetap pada kunci CH3/CH5

Detail: `PlatformIO/.../Remote-Side-05.2/src/README.md`

---

## 5. User-Side-05

**Proyek:** `ESP-Now_ESP32-S3_User-Side-05`

- Gateway USB ↔ ESP-NOW. Tidak terhubung ke mini PC
- Forward `$WPSET` → `0xA1`, `$SHUTDOWN` → `0xA2`
- CSV 24 kolom ke dashboard. `yaw` dan `heading_setpoint` diteruskan mentah (×100)
- Tidak perlu versi baru untuk bearing maupun `$RTL`: kolom `heading_setpoint` sudah ada, dan `$RTL` hanya lewat USB Remote ↔ mini PC

Detail: `PlatformIO/.../User-Side-05/src/README.md`

---

## 6. Dashboard 1.8

**File:** `Local Monitor Dashboard-1.8.py`

- Live: mode, Mini PC CONNECTED/DISCONNECTED, warning auto tanpa mini PC
- Tombol **Shutdown** sebelah status Mini PC (enable jika Connect + `mini_pc_link=1`)
- Map Points: Home + waypoints, **Send Way Points** (`$WPSET` / `$WACK`)
- Live: **u surge**, **v sway** (hitung lokal; bukan dari firmware)
- Logging & Analyze: CSV telemetry ditambah `u (m/s)`, `v (m/s)`
- Peta Live, Map Points, dan Analyze memakai simbol yang sama: Home kotak hijau (tanpa lingkaran), waypoint bintang bernomor + lingkaran 3 m, garis rencana oranye putus-putus antar waypoint, kapal belah ketupat hijau
- Analyze: **Load Log CSV** (jejak biru) dan **Load Waypoints**. Segitiga hijau = auto mulai, segitiga merah = auto selesai, belah ketupat oranye = log berakhir masih auto
- Heading: Live merah putus-putus 5 m; Map Points oranye solid 5 m; Analyze merah putus-putus pada slider, plus checkbox **Heading Line**
- Tab **3D**: replay log (jejak hitam) dengan Play. **Prediksi** menggambar lintasan NMPC sebagai garis merah di scene yang sama. Play tidak memutar prediksi
- Prediksi memakai `predict_track.exe` (Home, yaw awal 90° timur, `u0 = 0.6114`, `v = 0`)
- Plot Heading Setpoint pada alg 2 mengikuti bearing dari 2.3 (waypoint, atau Home setelah `$RTL`). Saat manual, setpoint sama dengan yaw
- Alarm Live: jika `battery_1` atau `battery_2` < 10,8 V, label berkedip dan bunyi berulang. Tombol Diamkan alarm mematikan bunyi saja. Dashboard tidak mengirim `$RTL`

Detail: `README Local Monitor Dashboard-1.8.md`

---

## 7. Mini PC — Cpp_ReadWriteSerial-2.3

**Path:** `Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/`

| Arah | Isi |
|------|-----|
| Terima | CSV 8 kolom saat CH6 auto; `[WP] Home` / `[WP] #n`; `$SHUTDOWN` |
| Pakai | `timestamp`, `lat`, `lon`, `yaw`, `yaw_rate`, daftar WP |
| Abaikan | `calc_deg_servo_1/2`, `gyro_z` |
| Kirim | `$HB` tiap 1 s; `timestamp,result` atau `timestamp,result,bearing` |

`result` = offset rudder (°), dibatasi solver ±45. `bearing` = haluan kompas ke target (0 Utara, 90 Timur). Saat `$RTL`, targetnya Home. Kolom ketiga ada bila GPS fix dan ada target (waypoint, atau Home setelah `$RTL`). Di dalam `r_tran` target, rudder = 0.

`$RTL` dari Remote mengunci target ke Home sampai program 2.3 di-restart. Waypoint tidak dilanjutkan. Tanpa `[WP] Home`, rudder netral.

State NMPC: `v = 0`, `u0 = 0.6114` m/s, `ψ = π/2 − yaw`, `r` dari `yaw_rate` (tanda minus). `L = 1.0107` m.

Detail: `Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC/README.md`

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
timestamp,result,bearing
$RTL
$SHUTDOWN
```

---

## 9. Protokol ESP-NOW

| msg_type | Payload | Arah | Fungsi |
|----------|---------|------|--------|
| (telemetry) | `DatatoSend` 64 B | Remote → User | Telemetry 24 field |
| `0xA1` | `waypoints_payload` ~180 B | User → Remote | Waypoint + home |
| `0xA2` | `pc_command_payload` 4 B | User → Remote | Perintah mini PC (`cmd=1` shutdown) |

**Catatan:** `0xA2` di versi 05 / 05.1 = shutdown mini PC (bukan tuning NVS dokumen lama).

---

## 10. Telemetry 24 kolom

Urutan sama di Remote `DatatoSend`, User CSV, dan parser dashboard:

1 timestamp, 2 lat, 3 lon, 4 speedMps×100, 5–6 servo×100, 7 yaw×100,  
8 hdg_sp×100, 9 hdg_err×100, 10 rudder_cmd×100, 11 track_wp_index,  
12 distance_to_wp×10, 13–18 IMU×100, 19–20 RPM, 21–22 battery×100,  
23 mode_auto, **24 mini_pc_link**

| Field | Arti |
|-------|------|
| `mode_auto` | 0 Manual, 1 Auto PD (alg 1), 2 Auto Mini PC (alg 2, default) |
| `heading_setpoint` | Manual: sama dengan yaw. Alg 2 + bearing 2.3: bearing kompas (WP atau Home). Alg 1: bearing haversine |
| `track_wp_index` | Diisi alg 1: 0 tidak menjejak, 1…N nomor WP, 255 home. Manual dan alg 2 selalu 0 (indeks WP aktif ada di mini PC) |
| `mini_pc_link` | 1 jika Remote menerima `$HB` dalam 3 detik; 0 jika putus. Dashboard CONNECTED membaca kolom ini |
| RPM | Hasil ukur encoder, bukan perintah. Perintah propeller tetap dari RC (lihat bagian 4) |

Log dashboard menambah `u (m/s)` dan `v (m/s)` setelah `speedMps`. Di file log, `mode_auto` menjadi kolom 25 dan `mini_pc_link` kolom 26. Kedua kolom `u`,`v` tidak ada di firmware.

---

## 11. Yaw, setpoint, u dan v

**Yaw** hanya diubah di Remote:

```text
raw (−180…180) → wrap 0…360 → +90° → 360 − yaw
```

Hasil di CSV: 0 = Utara, 90 = Timur, 270 = Barat. User-Side dan dashboard hanya ÷100.

**Heading setpoint** bukan mode zigzag.

| Kondisi | Isi |
|---------|-----|
| CH6 manual | sama dengan yaw, error 0 |
| Alg 2 + baris `timestamp,result,bearing` | bearing kompas dari 2.3 |
| Alg 2 tanpa kolom ketiga | salinan yaw |
| Alg 1 | bearing haversine di Remote |

**`u`, `v` di dashboard 1.8** (kompas CW, rumus sama dengan 1.6):

```text
u = ẋ sinψ + ẏ cosψ
v = ẋ cosψ − ẏ sinψ
```

2.3 tidak memakai rumus ini (`v = 0`, `u0 = 0.6114`).

---

## 12. Algoritma auto track

Dipilih compile-time di Remote (`AUTO_TRACK_ALG`):

| Nilai | Perilaku |
|-------|----------|
| 1 | Waypoint haversine + PD rudder. `heading_setpoint` = bearing |
| 2 (default) | Rudder dari mini PC `result`. `heading_setpoint` dari kolom `bearing` bila ada |

CH6 ≥ 1750 = Auto. Jika alg 2 dan `mini_pc_link=0` → rudder netral + warning. Propeller saat auto tetap pada kunci CH3/CH5, tidak ikut stik.

CH6 manual ↔ auto tidak mereset indeks waypoint di mini PC. Reset ke WP1 hanya lewat **Send Way Points** baru atau restart program 2.3. `$RTL` yang sudah terkunci tidak dilepas oleh Send Way Points; lepasnya saat program 2.3 di-restart.

---

## 13. Build, upload & menjalankan

```bash
# Firmware
cd PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05.2
pio run --target upload
cd ../ESP-Now_ESP32-S3_User-Side-05
pio run --target upload

# Mini PC 2.3
cd Cpp_Files/Cpp_ReadWriteSerial-2.3-ENU-NMPC
g++ -std=c++17 -Iinclude -Inmpc src/main.cpp src/serial_port.cpp nmpc/nmpc_kapal_waypoint.c nmpc/geo_enu.c nmpc/waypoint_manager.c -o read_write_serial.exe
.\read_write_serial.exe --port COMx --baud 115200 --rudder-mode nmpc --print all

# Dashboard
cd Pythonfile/Way_Points_Tracking
python "Local Monitor Dashboard-1.8.py"
```

Port COM mini PC ada di `start_read_write_serial.bat` (ubah sendiri). Sesuaikan MAC ESP-NOW di kedua `main.cpp`.

---

## 14. Prosedur uji lapangan

1. Flash **Remote-05.2** + **User-05** berpasangan
2. Jalankan **read_write_serial.exe** 2.3 di mini PC (auto-start opsional)
3. Connect dashboard **1.8** ke User-Side
4. Verifikasi Live: telemetry + Mini PC **CONNECTED**
5. Map Points → Set Home + ≥1 WP → **Send Way Points** → `$WACK,OK`; cek `[WP]` di stderr/stdout 2.3
6. Set kecepatan dan arah propeller di CH3/CH5 saat masih manual, lalu RC CH6 Auto. Propeller menahan nilai itu; pantau rudder dari `result` dan heading setpoint dari `bearing`
7. (Opsional) **Shutdown** → konfirmasi → `$SACK,OK` → mini PC mati ~5 s

---

## 15. Troubleshooting

| Gejala | Tindakan |
|--------|----------|
| Mini PC DISCONNECTED | Cek USB Remote↔PC, jalankan exe 2.3, baud 115200. Dashboard Connect saja tidak mengisi `mini_pc_link` |
| `$WACK` TIMEOUT | MAC ESP-NOW, Remote power, jarak |
| Auto Mini PC tidak gerak | CH6 high, `mini_pc_link=1`, timestamp CSV cocok dengan balasan |
| Heading setpoint = yaw saat auto | 2.3 belum mengirim kolom bearing (GPS atau waypoint belum siap), atau Remote yang ter-flash masih 05 |
| Propeller tidak ikut CH3/CH5 | CH6 masih auto: kunci PWM. Kembalikan ke manual |
| `u`,`v` aneh | Remote belum di-flash yaw CW, atau log lama (skala IMU) |
| Shutdown tombol abu-abu | Harus Connect + CONNECTED |
| `$SACK,OK` tapi PC tidak mati | Pastikan exe **2.3** yang menangani `$SHUTDOWN` |
| Telemetry 23 kolom | Flash User-05 / Remote-05.2; dashboard 1.8 tetap terima 23/24 |
| Tidak pulang saat baterai rendah | CH6 harus auto, tegangan < 10,8 V selama 10 detik terus, dan `[WP] Home` sudah terkirim. Cek baris `$RTL` di serial mini PC |

---

## Diagram alur

```text
┌──────────────────┐  $WPSET / $SHUTDOWN  ┌──────────────┐  0xA1 / 0xA2  ┌────────────────┐
│ Dashboard 1.8    │ ───────────────────► │ User-Side-05 │ ────────────► │ Remote-Side-05.2│
│                  │ ◄─────────────────── │              │ ◄──────────── │ yaw kompas CW  │
└──────────────────┘  CSV24 / $WACK/$SACK └──────────────┘  telemetry 24 └───────┬────────┘
                                                                                  │ USB
                                                                                  ▼
                                                                          ┌────────────────┐
                                                                          │ Mini PC 2.3    │
                                                                          │ NMPC           │
                                                                          │ CSV8 / [WP] /  │
                                                                          │ $HB, result,   │
                                                                          │ bearing        │
                                                                          └────────────────┘
```

---

*Dokumen ini: Ship Auto Way Maps Points Tracking — dashboard 1.8, User-Side-05, Remote-Side-05.2, Cpp_ReadWriteSerial-2.3-ENU-NMPC*

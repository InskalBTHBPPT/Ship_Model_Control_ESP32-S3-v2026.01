# Ship Auto Way Maps Points Tracking

Dokumentasi sistem **Ship Auto Way Maps Points Tracking** — kontrol kapal model ESP32-S3 dengan waypoint, tuning algoritma auto, telemetry real-time, dan dashboard PySide6.

**Versi dokumen:** beta 1.2 | **Author:** Chandra P — Ship Model Control System

---

## Daftar isi

1. [Ringkasan sistem](#1-ringkasan-sistem)
2. [Komponen & path proyek](#2-komponen--path-proyek)
3. [Arsitektur & alur data](#3-arsitektur--alur-data)
4. [Remote-Side (firmware kapal)](#4-remote-side-firmware-kapal)
5. [User-Side (gateway USB)](#5-user-side-gateway-usb)
6. [Dashboard (PC)](#6-dashboard-pc)
7. [Protokol serial (Dashboard ↔ User-Side)](#7-protokol-serial-dashboard--user-side)
8. [Protokol ESP-NOW (User-Side ↔ Remote-Side)](#8-protokol-esp-now-user-side--remote-side)
9. [Telemetry 23 kolom](#9-telemetry-23-kolom)
10. [Algoritma auto track](#10-algoritma-auto-track)
11. [NVS tuning (Remote)](#11-nvs-tuning-remote)
12. [Konfigurasi MAC ESP-NOW](#12-konfigurasi-mac-esp-now)
13. [Build, upload & menjalankan](#13-build-upload--menjalankan)
14. [Prosedur uji lapangan](#14-prosedur-uji-lapangan)
15. [Troubleshooting](#15-troubleshooting)

---

## 1. Ringkasan sistem

Sistem ini memungkinkan:

- **Monitoring live** posisi, heading, rudder, RPM, baterai dari kapal model
- **Perencanaan waypoint** di peta (Home + hingga 10 waypoint navigasi)
- **Pengiriman waypoint & tuning** ke kapal via ESP-NOW (disimpan di NVS Remote)
- **Verifikasi read-back** parameter tuning (`$TUNGET` / `$TACK`)
- **Auto track Alg 1** — navigasi waypoint dengan kontrol PD heading
- **Auto track Alg 2** — stub (default, rudder netral)
- **Analisis log CSV** — replay data dengan peta dan plot

Alur end-to-end:

```
Dashboard (PySide6)
    │ USB serial 115200, ASCII + CSV
    ▼
User-Side ESP32-S3  (ESP-Now_ESP32-S3_User-Side-02)
    │ ESP-NOW peer-to-peer
    ▼
Remote-Side ESP32-S3 (ESP-Now_ESP32-S3_Remote-Side-02) — di kapal
```

---

## 2. Komponen & path proyek

| Komponen | Path (dari root repo) | File utama |
|----------|----------------------|------------|
| Dashboard | `Pythonfile/Way_Points_Tracking/` | `Local Monitor Dashboard-beta1.2.py` |
| User-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-02/` | `src/main.cpp` |
| Remote-Side | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-02/` | `src/main.cpp` |

Dokumentasi ini disalin ke setiap folder proyek firmware sebagai referensi yang sama.

Salinan: `ESP-Now_ESP32-S3_Remote-Side-02/`, `ESP-Now_ESP32-S3_User-Side-02/`

---

## 3. Arsitektur & alur data

### 3.1 Telemetry (kapal → PC)

1. Remote membaca sensor ~10 Hz, membentuk struct `DatatoSend`
2. Remote mengirim struct via ESP-NOW ke User-Side
3. User-Side mencetak CSV 23-kolom ke USB serial
4. Dashboard `poll_serial()` mem-parse, menampilkan nilai fisik, log CSV

### 3.2 Waypoint & tuning (PC → kapal)

1. Dashboard validasi Home + waypoint, kirim `$WPSET`
2. User-Side parse, forward ESP-NOW `0xA1` ke Remote
3. Remote simpan waypoint di RAM, ACK `0xC1` → User-Side → `$WACK,OK,WP`
4. Dashboard kirim `$TUNSET` → Remote simpan NVS → `$WACK,OK,TUN`
5. Dashboard kirim `$TUNGET` → Remote balas `0xA3` → `$TACK,...` → verifikasi

### 3.3 Timeout

| Lapisan | Timeout | Catatan |
|---------|---------|---------|
| User-Side menunggu Remote ACK/TACK | 2.5 s | `ACK_TIMEOUT_MS` |
| Dashboard menunggu respons serial | 3 s | per langkah WP / TUN / TUNGET |

---

## 4. Remote-Side (firmware kapal)

**Proyek:** `ESP-Now_ESP32-S3_Remote-Side-02`

Firmware di kapal: sensor, actuator, kontrol rudder/propeller, waypoint tracking, NVS tuning, telemetry ESP-NOW.

### 4.1 Hardware

| Perangkat | Interface | Pin / catatan |
|-----------|-----------|---------------|
| MCU | ESP32-S3 DevKitC1-N16R8 | 16 MB Flash, 8 MB PSRAM |
| RC FS-iA6B | PPM | GPIO 4 |
| Rudder servo | PWM 50 Hz | GPIO 5; feedback ADC GPIO 8, 3 |
| Propeller speed/dir | PWM (Servo lib) | GPIO 6, 7 |
| Encoder prop 1/2 | Interrupt | GPIO 9, 10 |
| GNSS u-blox | Serial1 | GPIO 17 RX, 18 TX; 115200, 10 Hz |
| IMU HWT905TTL / JY901 | Serial2 | GPIO 15 RX, 16 TX; 57600 |
| Baterai 1 | ADC | GPIO 1 (divider ×5) |
| Baterai 2 | ADC | GPIO 2 (divider ×5) |

### 4.2 Mapping channel RC

| Channel | Fungsi |
|---------|--------|
| CH1 | Rudder manual (map −40° … +40°) |
| CH3 | Kecepatan propeller |
| CH5 | Arah propeller |
| CH6 | Auto/Manual — **≥ 1750 = Auto**, &lt; 1750 = Manual |

### 4.3 Mode operasi

| Mode | RC CH6 | Algoritma NVS | `mode_auto` | Rudder |
|------|--------|---------------|-------------|--------|
| Manual | &lt; 1750 | — | 0 | CH1 langsung |
| Auto Alg 1 | ≥ 1750 | alg = 1 | 1 | PD waypoint tracking |
| Auto Alg 2 | ≥ 1750 | alg = 2 (default) | 2 | Netral (stub) |

### 4.4 Penerimaan ESP-NOW

| msg_type | Handler | Aksi |
|----------|---------|------|
| `0xA1` | `handleWaypointsPayload` | Simpan waypoint RAM, reset `g_active_wp_index`, ACK WP |
| `0xA2` | `handleTrackConfigSet` | Validasi, apply runtime, simpan NVS, ACK TUN |
| `0xB1` | `sendTrackConfigResponse` | Balas config aktif via `0xA3` |

### 4.5 Struct waypoint (`waypoints_payload`, ~180 byte)

```
uint8_t  msg_type      = 0xA1
uint8_t  home_valid    = 1 jika Home valid
uint8_t  wp_count      = 0..10 (waypoint navigasi, tidak termasuk Home)
uint8_t  reserved
double   home_lat, home_lon
double   wp_lat[10], wp_lon[10]
```

Home disimpan terpisah; `wp_count` hanya menghitung waypoint klik di peta.

### 4.6 Dependencies PlatformIO

ESP32Servo, TinyGPSPlus, JY901 (lib lokal), Preferences, ESP-NOW, WiFi.

---

## 5. User-Side (gateway USB)

**Proyek:** `ESP-Now_ESP32-S3_User-Side-02`

Bridge: perintah ASCII dari PC ↔ binary ESP-NOW ke Remote. **Tidak** mengirim `$WACK,OK` sebelum Remote membalas `0xC1`.

### 5.1 Peran

- Parse baris serial (`$WPSET`, `$TUNSET`, `$TUNGET`)
- Forward payload ESP-NOW ke Remote
- Tunggu ACK (`0xC1`) atau tuning response (`0xA3`)
- Relay telemetry struct Remote sebagai CSV ke USB

### 5.2 Loop utama

- Baca serial baris-per-baris (buffer, max line length)
- `checkAckTimeout()` — emit `$WACK,ERR,...TIMEOUT` atau `$TACK,ERR,TIMEOUT`
- Saat telemetry diterima: `printTelemetryCsv()` langsung

### 5.3 Struct telemetry (`receivedfromremoteside`)

Identik field dengan `DatatoSend` Remote (64 byte). User-Side tidak mengubah skala — dashboard yang decode.

---

## 6. Dashboard (PC)

**File:** `Local Monitor Dashboard-beta1.2.py` (PySide6)

### 6.1 Tab aplikasi

| Tab | Fungsi utama |
|-----|--------------|
| **Map Points** | Klik peta → waypoint; Set Home; tabel marker; Send to Remote; panel Setup |
| **Live Data** | Peta tracking; indikator; 3 plot time-series; log CSV; route WP; koreksi rudder |
| **Analize Data** | Load CSV; peta replay; plot; indikator; slider timeline |

### 6.2 Map Points — panel kanan (urutan)

1. **Live position** — lat/lon/heading dari serial terbaru
2. **Set Home Point** — Home dari posisi live (butuh connect)
3. **Marker Points** — tabel 11 baris (Home + 10 WP max tampilan)
4. **Send to Remote** — info jumlah point, tombol kirim, status WP/TUN/verify
5. **Setup** — tombol **Open Setup** → dialog alg + tuning

### 6.3 Setup dialog

- **Alg 1** — waypoint + PD; spinbox Kp, Kd, arrive (m), rudder max (°)
- **Alg 2** — stub (default); tanpa parameter
- **Read from Remote** — `$TUNGET`, update UI dari `$TACK`

Nilai dialog disimpan lokal; commit ke kapal hanya via **Send to Remote**.

### 6.4 Send to Remote — state machine

| Fase | Kirim | Respons sukses | Status UI |
|------|-------|----------------|-----------|
| `wp` | `$WPSET,...` | `$WACK,OK,WP` | Lanjut TUN |
| `tun` | `$TUNSET,...` | `$WACK,OK,TUN` | Lanjut TUNGET |
| `tunget` | `$TUNGET` | `$TACK,...` | Verified OK / MISMATCH |

Validasi sebelum kirim:

- Serial connected
- `home_point_coords` sudah di-set
- Minimal **Home + 2 waypoint** (total ≥ 3 point)
- Semua koordinat dalam range valid

Snapshot waypoint otomatis ke `WayPoints/DDMMYYYY_HHMM_WayPoints.csv`.

### 6.5 Log CSV (tab Live)

Header `TELEMETRY_LOG_HEADER` — nilai **tampilan** (bukan raw fixed-point), sama seperti panel Live.

### 6.6 Analyze — format CSV didukung

| Format | Deteksi |
|--------|---------|
| `display_v23` | Kolom `speedMps (m/s)`, `heading_setpoint (°)` |
| `raw_v23` | Kolom `heading_setpoint (raw)` |
| `legacy` | Format lama |

### 6.7 Dependencies Python

Python 3.10+, PySide6, pyserial, PyQtWebEngine.

```bash
cd Pythonfile/Way_Points_Tracking
python "Local Monitor Dashboard-beta1.2.py"
```

---

## 7. Protokol serial (Dashboard ↔ User-Side)

Semua perintah diakhiri `\n`. Baud: **115200**.

### 7.1 `$WPSET` — set waypoint

```
$WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>
```

| Field | Aturan |
|-------|--------|
| `home_lat`, `home_lon` | double, ±90° / ±180° |
| `wp_count` | 0–10 |
| waypoint | pasangan lat, lon per WP |

Contoh (Home + 2 WP):

```
$WPSET,-7.281500,112.798900,2,-7.282000,112.799500,-7.283000,112.800000
```

### 7.2 `$TUNSET` — set tuning

```
$TUNSET,<alg>[,<kp>,<kd>,<arrive_m>,<rudder_max>]
```

| alg | Parameter | Contoh |
|-----|-----------|--------|
| 1 | 4 float wajib | `$TUNSET,1,1.0000,0.0500,3.00,40.00` |
| 2 | tidak ada | `$TUNSET,2` |

### 7.3 `$TUNGET` — read-back tuning dari NVS Remote

```
$TUNGET
```

### 7.4 Respons `$WACK`

| Pola | Makna |
|------|-------|
| `$WACK,OK,WP` | Waypoint diterima Remote |
| `$WACK,OK,TUN` | Tuning disimpan NVS |
| `$WACK,ERR,<kind>,<reason>[,extra]` | Gagal |

### 7.5 Respons `$TACK`

| Pola | Makna |
|------|-------|
| `$TACK,<alg>,<kp>,<kd>,<arrive>,<rudmax>` | Read-back sukses |
| `$TACK,ERR,<reason>` | Gagal |

### 7.6 Tabel error User-Side (validasi lokal)

**Waypoint (`kind=WP`):**

| Reason | Penyebab |
|--------|----------|
| `FORMAT` | Token tidak cukup |
| `HOME_LAT`, `HOME_LON` | Home tidak numerik |
| `COUNT_NOT_INT` | wp_count bukan integer |
| `COUNT_RANGE` | wp_count &lt; 0 atau &gt; 10 |
| `COUNT_MISMATCH` | jumlah token ≠ 3 + 2×wp_count |
| `LAT_RANGE,home` / `LON_RANGE,home` | Home out of range |
| `WP_LAT,n` / `WP_LON,n` | WP tidak numerik |
| `LAT_RANGE,n` / `LON_RANGE,n` | WP out of range |
| `SEND_FAIL` | `esp_now_send` gagal |
| `TIMEOUT` | Remote tidak ACK dalam 2.5 s |
| `LINE_TOO_LONG` | Baris serial terlalu panjang |

**Tuning (`kind=TUN`):**

| Reason | Penyebab |
|--------|----------|
| `FORMAT` | Token kosong |
| `ALG` | alg tidak integer |
| `ALG_RANGE` | alg bukan 1 atau 2 |
| `PARAM_COUNT` | alg=1 tapi param ≠ 4 |
| `KP`, `KD`, `ARRIVE`, `RUDMAX` | float tidak valid |
| `SEND_FAIL`, `TIMEOUT` | sama seperti WP |

**TACK:**

| Reason | Penyebab |
|--------|----------|
| `SEND_FAIL` | esp_now_send gagal |
| `TIMEOUT` | Remote tidak balas 0xA3 |

### 7.7 Error ACK dari Remote (via `$WACK,ERR,TUN,<code>`)

| err_code | Makna |
|----------|-------|
| 1 (`VALIDATE`) | Parameter tuning out of range |
| 2 (`NVS`) | Gagal simpan Preferences |

---

## 8. Protokol ESP-NOW (User-Side ↔ Remote-Side)

| msg_type | Arah | Struct | Ukuran |
|----------|------|--------|--------|
| `0xA1` | User → Remote | `waypoints_payload` | ~180 B |
| `0xA2` | User → Remote | `track_config_payload` | 20 B |
| `0xB1` | User → Remote | `tun_get_request` | 1 B |
| `0xA3` | Remote → User | `track_config_payload` | 20 B |
| `0xC1` | Remote → User | `remote_ack_payload` | 4 B |
| telemetry | Remote → User | `DatatoSend` / `receivedfromremoteside` | 64 B |

### `track_config_payload`

```
msg_type, active_alg, param_count, reserved,
params[0]=Kp, params[1]=Kd, params[2]=arrive_m, params[3]=rudder_max_deg
```

### `remote_ack_payload`

```
msg_type=0xC1, ack_kind (1=WP, 2=TUN), status (0=OK, 1=ERR), err_code
```

---

## 9. Telemetry 23 kolom

Urutan CSV dari User-Side (raw fixed-point):

```
timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,
yaw,heading_setpoint,heading_error,rudder_cmd,track_wp_index,distance_to_wp,
accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,
rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
```

| # | Field | Tipe Remote | Skala dashboard | Unit tampilan |
|---|-------|-------------|-----------------|---------------|
| 1 | timestamp | double | asli | s |
| 2 | latitude | double | asli | ° |
| 3 | longitude | double | asli | ° |
| 4 | speedMps | uint16 | ÷100 | m/s |
| 5 | Calc_deg_servo_1 | int16 | ÷100 | ° |
| 6 | Calc_deg_servo_2 | int16 | ÷100 | ° |
| 7 | yaw | uint16 | ÷100 | ° (0–360) |
| 8 | heading_setpoint | uint16 | ÷100 | ° |
| 9 | heading_error | int16 | ÷100 | ° |
| 10 | rudder_cmd | int16 | ÷100 | ° offset |
| 11 | track_wp_index | uint8 | asli | 0=—, 1..N=WP, 255=Home |
| 12 | distance_to_wp | uint16 | ÷10 | m |
| 13–15 | accel_x/y/z | int16 | ÷100 | g |
| 16–18 | gyro_x/y/z | int16 | ÷100 | deg/s |
| 19–20 | rpm_prop_1/2 | uint16 | ÷100 | rpm |
| 21–22 | battery_1/2 | uint16 | ÷100 | V |
| 23 | mode_auto | uint8 | asli | 0/1/2 |

Update rate: ~10 Hz (loop Remote 100 ms).

---

## 10. Algoritma auto track

### 10.1 Alg 1 — waypoint + PD

**Persyaratan:** GPS valid, waypoint sudah diterima (`$WPSET` sukses), CH6 ≥ 1750.

**Urutan target:**

1. Jika `wp_count > 0`: navigasi ke WP1 → WP2 → … (index `g_active_wp_index`)
2. Jika `wp_count == 0` dan `home_valid`: target Home (`track_wp_index = 255`)
3. Saat jarak ke WP aktif &lt; `arrive_m`, advance ke WP berikutnya

**Kontrol:**

```
bearing = bearingDeg(pos, target)
heading_error = wrap(bearing - yaw, ±180°)
rudder_offset = Kp × heading_error − Kd × gyro_z
rudder_offset = clamp(rudder_offset, ±rudder_max)
```

**Telemetry navigasi:** heading_setpoint = bearing, heading_error, track_wp_index, distance_to_wp.

### 10.2 Alg 2 — stub (default)

Rudder netral; heading_setpoint, heading_error, track_wp_index, distance_to_wp = 0.

### 10.3 Parameter tuning Alg 1

| Parameter | Default | Range validasi Remote |
|-----------|---------|----------------------|
| Kp | 1.0 | 0 – 10 |
| Kd | 0.05 | 0 – 2 |
| WP arrive (m) | 3.0 | 0.5 – 50 |
| Rudder max (°) | 40.0 | 1 – 45 |

---

## 11. NVS tuning (Remote)

| Item | Nilai |
|------|-------|
| Namespace | `wptrack` |
| Magic | `0xA24150` (`TUN_CFG_MAGIC`) |
| Keys | `magic`, `alg`, `kp`, `kd`, `arrive`, `rudmax` |

Load saat `setup()` via `loadTrackConfigFromNvs()`. Jika magic tidak cocok → default Alg 2.

Waypoint **tidak** disimpan NVS (hanya RAM sampai power cycle atau `$WPSET` baru).

---

## 12. Konfigurasi MAC ESP-NOW

**PENTING:** MAC harus saling cocok di kedua firmware.

**Remote** `src/main.cpp`:

```cpp
uint8_t user_side_Address[] = {0x80, 0xb5, 0x4e, 0xc1, 0xd5, 0xac};
```

**User-Side** `src/main.cpp`:

```cpp
uint8_t remote_side_Address[] = {0x10, 0x20, 0xba, 0x4c, 0x53, 0xfc};
```

Cek MAC perangkat: upload firmware yang print MAC di `setup()`, atau tool WiFi scanner. Sesuaikan pasangan di kedua file sebelum uji lapangan.

---

## 13. Build, upload & menjalankan

### Remote-Side

```bash
cd PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-02
pio run
pio run --target upload
pio device monitor
```

### User-Side

```bash
cd PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-02
pio run
pio run --target upload
pio device monitor
```

Sesuaikan `upload_port` / `monitor_port` di `platformio.ini` tiap proyek.

### Dashboard

```bash
cd Pythonfile/Way_Points_Tracking
python "Local Monitor Dashboard-beta1.2.py"
```

---

## 14. Prosedur uji lapangan

1. **Flash** Remote-Side ke ESP di kapal, User-Side ke ESP gateway USB
2. **Verifikasi MAC** ESP-NOW cocok di kedua `main.cpp`
3. **Power** kapal — tunggu GPS fix (monitor Remote optional)
4. **Connect** dashboard ke port User-Side (115200)
5. Tab **Map Points** → tunggu lat/lon live update
6. **Set Home Point** dari posisi aktual
7. Klik **2+ waypoint** di peta
8. Panel **Setup** → pilih alg & param (default Alg 2)
9. **Send to Remote** → status harus **Verified OK**
10. Opsional: **Read from Remote** di Setup untuk cek NVS
11. **RC CH6 ≥ 1750** → mode Auto di kapal
12. Tab **Live Data** → pantau track_wp_index, distance_to_wp, rudder_cmd

---

## 15. Troubleshooting

| Gejala | Kemungkinan penyebab | Tindakan |
|--------|---------------------|----------|
| TIMEOUT (wp/tun) | MAC salah, Remote off, jarak ESP-NOW | Cek MAC, power, jarak &lt; ~100 m LOS |
| MISMATCH setelah TUNGET | NVS beda dari Setup | Read from Remote; kirim ulang TUNSET |
| Verified OK tapi auto tidak jalan | CH6 manual, GPS invalid, alg=2 stub | CH6 high, tunggu GPS, set Alg 1 |
| Telemetry tidak masuk | User-Side port salah, Remote tidak kirim | Cek COM port, MAC, Remote loop |
| $WACK,ERR,WP,COUNT_MISMATCH | Format $WPSET salah | Cek jumlah koordinat di payload |
| $WACK,ERR,TUN,1 atau 2 | Validasi/NVS Remote | Cek range param; flash ulang Remote |
| Waypoint tidak bergerak | wp_count=0 atau GPS invalid | Kirim ulang WPSET; cek GNSS |
| track_wp_index selalu 0 | Mode manual atau Alg 2 | CH6 Auto + Alg 1 + waypoint loaded |

---

## Lampiran — diagram alur Send to Remote

```
[Dashboard] Send to Remote
     │
     ├─► $WPSET ──► [User-Side] ──ESP-NOW 0xA1──► [Remote] RAM waypoint
     │                      ▲                           │
     │                      └── 0xC1 ACK WP ────────────┘
     │                 $WACK,OK,WP
     │
     ├─► $TUNSET ──► [User-Side] ──ESP-NOW 0xA2──► [Remote] NVS + runtime
     │                      ▲                           │
     │                      └── 0xC1 ACK TUN ──────────┘
     │                 $WACK,OK,TUN
     │
     ├─► $TUNGET ──► [User-Side] ──ESP-NOW 0xB1──► [Remote]
     │                      ▲                           │
     │                      └── 0xA3 config ─────────────┘
     │                 $TACK,alg,kp,kd,arrive,rudmax
     │
     └─► Bandingkan $TACK vs Setup → Verified OK / MISMATCH
```

---

*Dokumen ini: Ship Auto Way Maps Points Tracking — beta 1.2*

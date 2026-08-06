# ESP-Now_ESP32-S3_Remote-Side-05

Firmware sisi kapal (Remote-Side) untuk sistem **Way Points Tracking**.

Clone dari **Remote-Side-04** dengan tambahan forward perintah **`$SHUTDOWN`** ke mini PC (ESP-NOW `0xA2`).

Mengumpulkan data sensor dan actuator, menjalankan kontrol rudder/propeller, menerima waypoint dari User-Side via ESP-NOW, lalu mengirim telemetry **24 kolom** ke User-Side @ 10 Hz.

**Pasangan:**
| Komponen | Path |
|----------|------|
| User-Side | `ESP-Now_ESP32-S3_User-Side-05` |
| Dashboard | `Local Monitor Dashboard-beta1.5.py` |
| Mini PC | `Cpp_Files/Cpp_ReadWriteSerial-1.0` |

---

## Ringkasan Fitur

- Pembacaan PPM dari receiver RC (FS-iA6B)
- Kontrol rudder: manual (CH1) atau auto mini PC (default alg 2)
- Kontrol propeller speed/direction via LEDC (CH3, CH5)
- GNSS u-blox @ 115200 baud, 10 Hz (UBX)
- IMU HWT905TTL / JY901 @ 115200 baud
- RPM motor propeller (2× rotary encoder)
- Monitoring tegangan baterai (2 channel ADC)
- **Terima waypoint** dari User-Side (`waypoints_payload`, msg `0xA1`)
- **Echo waypoint ke mini PC** — baris `[WP] ...` di USB Serial (sama port dengan CSV)
- **Forward `$SHUTDOWN`** ke mini PC saat ESP-NOW `0xA2` diterima
- **Kirim telemetry** `DatatoSend` (64 byte) ke User-Side via ESP-NOW
- **Debug CSV** ke mini PC @ 10 Hz (8 kolom, hanya saat RC auto)
- **Mini PC serial:** terima `$HB` + `timestamp,result` (rudder deg)

---

## Protokol Mini PC (USB Serial 115200)

| Arah | Format | Keterangan |
|------|--------|------------|
| ESP32 → PC | CSV 8 kolom | Hanya saat CH6 auto |
| ESP32 → PC | `[WP] ...` | Saat waypoint `0xA1` diterima; dibaca/print oleh `Cpp_ReadWriteSerial-1.0` (`--print all\|wp`) |
| ESP32 → PC | `$SHUTDOWN` | Saat perintah `0xA2` SHUTDOWN; mini PC menjalankan shutdown OS |
| PC → ESP32 | `$HB` | Heartbeat ~1 Hz (manual/auto) |
| PC → ESP32 | `timestamp,result` | `result` = rudder offset (°), timestamp harus sama dengan baris CSV input |

Field telemetry ESP-NOW tambahan: `mini_pc_link` (kolom 24) — `1` jika heartbeat OK (< 3 s).

**Alur waypoint ke mini PC:**

```text
Dashboard ($WPSET) → User-Side-05 → ESP-NOW 0xA1 → Remote-Side-05
  → simpan g_lastWaypoints + printWaypoints() → USB Serial [WP]
  → Cpp_ReadWriteSerial-1.0 (stdout, filter --print)
```

**Alur shutdown mini PC (tanpa Wi‑Fi laptop↔mini PC):**

```text
Dashboard ($SHUTDOWN) → User-Side-05 → ESP-NOW 0xA2 → Remote-Side-05
  → Serial.println("$SHUTDOWN") → Cpp_ReadWriteSerial-1.0 → shutdown OS
```

User-Side membalas dashboard dengan `$SACK,OK` / `$SACK,ERR,...` (sukses forward ESP-NOW).

---

## Mode Kontrol

Dipilih via **CH6** receiver RC:

| CH6 | Mode | Perilaku |
|-----|------|----------|
| `< 1750` | Manual | Rudder dari CH1 (−40° … +40° offset netral) |
| `≥ 1750` | Auto | Algoritma dipilih compile-time `AUTO_TRACK_ALG` |

### `AUTO_TRACK_ALG` (ubah di `main.cpp` sebelum upload)

| Nilai | Fungsi |
|-------|--------|
| `1` | **Waypoint + PD** (opsional) — bearing haversine ke WP aktif |
| `2` | **Mini PC** (default) — rudder dari `timestamp,result` serial |

Parameter (default):

```cpp
#define AUTO_TRACK_ALG 2
#define MINI_PC_HB_TIMEOUT_MS 3000
#define RUDDER_CMD_MAX 40.0f
```

Auto alg 2: jika `mini_pc_link=0` saat RC auto → rudder netral + `[WARN]` serial. Tidak fallback ke PD.

---

## Mapping Channel RC

| Channel | Fungsi |
|---------|--------|
| CH1 | Rudder (mode manual) |
| CH3 | Kecepatan propeller |
| CH5 | Arah propeller |
| CH6 | Auto (`≥1750`) / Manual (`<1750`) |

PPM mentah 600–1600 µs (FS-iA6B) → dimapping ke 1000–2000 µs.

---

## Hardware

| Komponen | Keterangan |
|----------|------------|
| MCU | ESP32-S3 DevKitC1-N16R8 |
| Receiver RC | FS-iA6B (PPM, GPIO 4) |
| Servo rudder | LEDC 50 Hz, 12-bit (GPIO 5) |
| Motor propeller ×2 | LEDC speed/dir (GPIO 6, 7) + encoder (GPIO 9, 10) |
| GNSS | u-blox, Serial1 GPIO 17/18 |
| IMU | HWT905TTL, Serial2 GPIO 15/16 |
| ADC feedback | GPIO 8 (servo 1), GPIO 3 (servo 2) |
| ADC baterai | GPIO 1 (sistem), GPIO 2 (motor) |

> Gunakan **ADC1** saja — ADC2 tidak stabil saat WiFi/ESP-NOW aktif.

---

## Pin Configuration

### Input
- **GPIO 4** — PPM (FS-iA6B)
- **GPIO 9** — Encoder motor propeller 1
- **GPIO 10** — Encoder motor propeller 2
- **GPIO 8** — ADC feedback servo 1 (ADC1_CH7)
- **GPIO 3** — ADC feedback servo 2 (ADC1_CH2)
- **GPIO 1** — ADC baterai 1 (ADC1_CH0)
- **GPIO 2** — ADC baterai 2 (ADC1_CH1)

### Output (LEDC)
- **GPIO 5** — Servo rudder (channel 0)
- **GPIO 6** — Propeller speed (channel 2)
- **GPIO 7** — Propeller direction (channel 1)

### Serial
- **Serial1 (TX17/RX18)** — GNSS (9600 → 115200 otomatis)
- **Serial2 (TX15/RX16)** — IMU @ 115200 baud

---

## ESP-NOW

### MAC User-Side (peer)

Ubah di `main.cpp` sesuai board User-Side Anda:

```cpp
uint8_t user_side_Address[] = {0x80, 0xb5, 0x4e, 0xc1, 0xd5, 0xac};
```

### Terima: `waypoints_payload` (User-Side → Remote)

| Field | Tipe | Keterangan |
|-------|------|------------|
| `msg_type` | `uint8_t` | `0xA1` |
| `home_valid` | `uint8_t` | 0/1 |
| `wp_count` | `uint8_t` | 0…10 |
| `reserved` | `uint8_t` | padding |
| `home_lat`, `home_lon` | `double` | titik home |
| `wp_lat[10]`, `wp_lon[10]` | `double` | daftar waypoint |

Total: **180 byte**. Diterima di callback `OnDataRecv`; isi dicetak ke Serial untuk debug `[WP]`.

### Kirim: `DatatoSend` (Remote → User-Side)

Dikirim setiap **100 ms** (`sizeof` = 64 byte):

```cpp
struct DatatoSend {
  double timestamp;           // detik sejak boot
  double latitude;            // derajat
  double longitude;           // derajat
  uint16_t speedMps;          // m/s × 100
  int16_t Calc_deg_servo_1;   // ° × 100 (feedback ADC)
  int16_t Calc_deg_servo_2;   // ° × 100
  uint16_t yaw;               // ° × 100 (0–360)
  uint16_t heading_setpoint;  // bearing ke WP aktif, ° × 100
  int16_t  heading_error;     // setpoint − yaw, ° × 100 (±180)
  int16_t  rudder_cmd;        // perintah rudder offset netral, ° × 100 (±40)
  uint8_t  track_wp_index;    // 0=idle, 1..N=WP#, 255=home
  uint16_t distance_to_wp;    // meter × 10
  int16_t accel_x, accel_y, accel_z;  // g × 100
  int16_t gyro_x, gyro_y, gyro_z;     // deg/s × 100
  uint16_t rpm_prop_1;        // RPM (nilai langsung, bukan ×100)
  uint16_t rpm_prop_2;
  uint16_t battery_1;         // V × 100
  uint16_t battery_2;         // V × 100
  uint8_t mode_auto;          // 0=manual, 1=auto alg1, 2=auto alg2 stub
};
```

**`mode_auto`:**

| Nilai | Arti |
|-------|------|
| 0 | Manual |
| 1 | Auto alg 1 (PD waypoint) |
| 2 | Auto alg 2 (stub) |

---

## Debug Serial (CSV @ 10 Hz)

Header dicetak sekali saat boot:

```text
timestamp,lat,lon,calc_deg_servo_1,calc_deg_servo_2,yaw,gyro_z,yaw_rate
```

Contoh baris data:

```text
24.783,0.000000,0.000000,-5.67,-20.57,0.00,0.00,0.00
```

| Kolom | Sumber |
|-------|--------|
| `timestamp` | `millis()/1000` |
| `lat`, `lon` | GNSS |
| `calc_deg_servo_1/2` | ADC feedback (°) |
| `yaw` | IMU (°) |
| `gyro_z` | IMU (°/s) |
| `yaw_rate` | Δyaw/Δt lokal (°/s), **tidak** masuk struct ESP-NOW |

Baud monitor: **115200** (`platformio.ini`).

Format CSV dipakai oleh `Cpp_Files/Cpp_ReadSerial` dan `Cpp_ReadWriteSerial` di PC.

### Echo waypoint `[WP]` (event-driven)

Dipanggil dari `OnDataRecv` → `printWaypoints()` setiap kali paket `waypoints_payload` valid diterima (bukan periodik 10 Hz). Contoh:

```text
[WP] Bytes received from User-Side: 180
[WP] msg_type=0xA1 home_valid=1 count=3
[WP] Home: -6.200000, 106.800000
[WP] #1: -6.201000, 106.801000
```

Mini PC (`Cpp_ReadWriteSerial`) mencetak ulang baris yang diawali `[WP]` ke stdout (opsi `--print all` atau `wp`).

---

## Kalibrasi & Rumus

### Feedback servo (ADC → derajat)
- Servo 1: `deg = (mV × 0.0595) − 98.848`
- Servo 2: `deg = (mV × 0.0594) − 98.801`

### Servo rudder
- Netral: 90° (duty 307, 1.5 ms @ 50 Hz)
- Offset perintah: ±40° (`RUDDER_CMD_MAX`)

### Baterai
- `V = (ADC_mV / 1000) × 5` (pembagi tegangan 5×)

### RPM
- Moving average 10 sampel, interval 100 ms, PPR = 1
- `RPM = avg_pulses_per_loop × 600`

### Auto-track PD
```
err = wrap180(bearing_to_wp − yaw)
rudder_cmd = Kp × err − Kd × gyro_z
```

---

## Build & Upload

Path proyek:

```text
PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05
```

```bash
pio run
pio run --target upload
pio device monitor
```

Sesuaikan `upload_port` / `monitor_port` di `platformio.ini` (default: `COM14`).

### Dependencies (`platformio.ini`)
- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo)
- [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)
- JY901 (library lokal di `lib/JY901`)

---

## Alur Operasi

1. Power ON → inisialisasi PPM, ADC, LEDC, GNSS (re-baud 115200, 10 Hz), IMU, ESP-NOW
2. User-Side kirim waypoint via ESP-NOW → Remote simpan + cetak `[WP]` ke mini PC
3. Operator pilih Manual/Auto lewat CH6
4. Loop 10 Hz: baca sensor → kontrol rudder/propeller → isi `dataToSend` → `esp_now_send`
5. Serial CSV debug ke mini PC saat RC auto; `$HB` / `timestamp,result` dari `Cpp_ReadWriteSerial-1.0`
6. Opsional: dashboard Shutdown → `$SHUTDOWN` ke mini PC

---

## Troubleshooting

| Masalah | Cek |
|---------|-----|
| GPS `lat/lon = 0` | Antena, outdoor, tunggu fix; koneksi Serial1 |
| IMU tidak update | Baud 115200, koneksi Serial2 GPIO 15/16 |
| Auto tidak gerak | CH6 ≥1750, GPS valid, waypoint sudah diterima (`[WP]` di serial) |
| ESP-NOW gagal | MAC User-Side benar, jarak, mode WIFI_STA |
| RPM 0 | Koneksi encoder GPIO 9/10, motor berputar |
| `$SHUTDOWN` tidak sampai mini PC | Flash pasangan 05; `Cpp_ReadWriteSerial-1.0` jalan; cek `$SACK,OK` di dashboard |

---

## Catatan

1. Interval utama: **100 ms (10 Hz)**
2. `AUTO_TRACK_ALG` dipilih **compile-time**, bukan runtime
3. Auto alg 2 default = mini PC (`timestamp,result`); alg 1 = waypoint PD
4. `msg_type 0xA2` = perintah mini PC (shutdown), **bukan** tuning NVS lama
5. Struct `DatatoSend` 64 byte / 24 field harus identik dengan User-Side-05

---

## Author & Versi

- **Author:** Chandra P — Ship Model Control System
- **Version:** 1.0 (Remote-Side-05) — dari Remote-Side-04
- **Last update:** 2026-08
- **Board:** ESP32-S3 DevKitC1-N16R8
- **Framework:** Arduino / PlatformIO

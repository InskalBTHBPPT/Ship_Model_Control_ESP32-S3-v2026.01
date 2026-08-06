# ESP-Now_ESP32-S3_Remote-Side-03

Firmware sisi kapal (Remote-Side) untuk sistem **Way Points Tracking**.

Mengumpulkan data sensor dan actuator, menjalankan kontrol rudder/propeller, menerima waypoint dari User-Side via ESP-NOW, lalu mengirim telemetry **23 kolom** ke User-Side @ 10 Hz.

Clone dari `ESP_Now_Send_Ver2025_revJan2026` / `Remote-Side-01`, dengan penambahan auto-track waypoint + debug CSV serial.

**Pasangan User-Side:** `ESP-Now_ESP32-S3_User-Side-01` (struct telemetry harus identik).

---

## Ringkasan Fitur

- Pembacaan PPM dari receiver RC (FS-iA6B)
- Kontrol rudder: manual (CH1) atau auto waypoint + PD heading
- Kontrol propeller speed/direction via LEDC (CH3, CH5)
- GNSS u-blox @ 115200 baud, 10 Hz (UBX)
- IMU HWT905TTL / JY901 @ 115200 baud
- RPM motor propeller (2× rotary encoder)
- Monitoring tegangan baterai (2 channel ADC)
- **Terima waypoint** dari User-Side (`waypoints_payload`, msg `0xA1`)
- **Kirim telemetry** `DatatoSend` (64 byte) ke User-Side via ESP-NOW
- **Debug CSV** ke Serial Monitor @ 10 Hz (8 kolom)

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
| `1` | **Waypoint + PD** — bearing haversine ke WP aktif, rudder = `Kp·err − Kd·gyro_z` |
| `2` | **Stub** — rudder netral, telemetry navigasi nol |

Parameter auto-track (default):

```cpp
#define AUTO_TRACK_ALG 1
#define WP_ARRIVE_M    3.0f    // jarak (m) advance ke WP berikutnya
#define AUTO_TRACK_KP  1.0f    // heading error (deg) → rudder offset (deg)
#define AUTO_TRACK_KD  0.05f   // damping gyro_z (deg/s)
#define RUDDER_CMD_MAX 40.0f   // max offset rudder (deg)
```

Auto-track membutuhkan: GPS valid + waypoint diterima dari User-Side. Tanpa itu, rudder netral.

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

Format ini dipakai oleh `Cpp_Files/Cpp_ReadSerial` dan `Cpp_ReadWriteSerial` di PC.

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
PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-03
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
2. User-Side kirim waypoint via ESP-NOW
3. Operator pilih Manual/Auto lewat CH6
4. Loop 10 Hz: baca sensor → kontrol rudder/propeller → isi `dataToSend` → `esp_now_send`
5. Serial CSV debug ke PC (opsional logging)

---

## Troubleshooting

| Masalah | Cek |
|---------|-----|
| GPS `lat/lon = 0` | Antena, outdoor, tunggu fix; koneksi Serial1 |
| IMU tidak update | Baud 115200, koneksi Serial2 GPIO 15/16 |
| Auto tidak gerak | CH6 ≥1750, GPS valid, waypoint sudah diterima (`[WP]` di serial) |
| ESP-NOW gagal | MAC User-Side benar, jarak, mode WIFI_STA |
| RPM 0 | Koneksi encoder GPIO 9/10, motor berputar |

---

## Catatan

1. Interval utama: **100 ms (10 Hz)**
2. `AUTO_TRACK_ALG` dipilih **compile-time**, bukan runtime
3. Auto alg 2 (NMPC/dll.) masih stub — siap untuk pengembangan lanjut
4. Penerimaan baris `timestamp,result` dari PC (**ReadWriteSerial**) di firmware ESP32: **belum diimplementasi (pending)**

---

## Author & Versi

- **Author:** Chandra P — Ship Model Control System
- **Version:** 1.0 (Remote-Side-03)
- **Last update:** 2026
- **Board:** ESP32-S3 DevKitC1-N16R8
- **Framework:** Arduino / PlatformIO

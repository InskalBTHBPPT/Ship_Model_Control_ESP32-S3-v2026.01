# ESP-Now_ESP32-S3_Remote-Side-01

Firmware sisi kapal (Remote-Side) untuk sistem kontrol kapal model ESP32-S3. Mengumpulkan data sensor dan actuator, menjalankan kontrol rudder/propeller, waypoint tracking, lalu mengirim telemetry 23-kolom ke User-Side via ESP-NOW.

Clone/evolusi dari `ESP_Now_Send_Ver2025_revJan2026`.

## Arsitektur

```
Dashboard (PC)  --USB serial-->  User-Side ESP32  --ESP-NOW-->  Remote-Side ESP32 (kapal)
```

Remote-Side menerima waypoint dan parameter tuning dari User-Side, menyimpan tuning di NVS, dan menjalankan algoritma auto track saat mode RC = Auto.

## Fitur utama

- Pembacaan PPM dari receiver RC (FS-iA6B)
- Kontrol servo rudder (manual + auto)
- Kontrol motor propeller (PWM speed + direction)
- GNSS u-blox (Serial1, 10 Hz)
- IMU HWT905TTL / JY901 (Serial2)
- RPM propeller via rotary encoder
- Monitoring baterai (ADC)
- **Waypoint tracking** via ESP-NOW `0xA1`
- **Runtime tuning** algoritma auto (NVS namespace `wptrack`)
- **Auto Alg 1**: waypoint + PD heading control
- **Auto Alg 2**: stub (rudder netral, default)

## Mode kontrol

| Mode | Kondisi RC CH6 | `mode_auto` telemetry |
|------|----------------|------------------------|
| Manual | CH6 < 1750 | 0 |
| Auto Alg 1 | CH6 ≥ 1750, alg=1 | 1 |
| Auto Alg 2 | CH6 ≥ 1750, alg=2 | 2 |

### Auto Alg 1 — waypoint + PD

- Bearing ke waypoint aktif → heading setpoint
- `rudder_cmd = Kp × heading_error − Kd × gyro_z`
- Advance waypoint saat jarak < `arrive_m`
- Urutan target: Home (index 255) → WP1 → WP2 → ...

### Auto Alg 2 — stub

Rudder netral; telemetry navigasi nol. Placeholder untuk algoritma lanjutan.

## Protokol ESP-NOW (terima dari User-Side)

| msg_type | Arah | Payload | Deskripsi |
|----------|------|---------|-----------|
| `0xA1` | User → Remote | `waypoints_payload` | Set Home + waypoint navigasi |
| `0xA2` | User → Remote | `track_config_payload` | Set alg + parameter tuning |
| `0xB1` | User → Remote | `tun_get_request` | Request read-back config |
| `0xA3` | Remote → User | `track_config_payload` | Balasan read-back tuning |
| `0xC1` | Remote → User | `remote_ack_payload` | ACK setelah WP/TUN sukses/gagal |

### Struktur `waypoints_payload` (180 byte)

```
msg_type, home_valid, wp_count, reserved,
home_lat, home_lon,
wp_lat[10], wp_lon[10]
```

Max 10 waypoint navigasi (`WP_MAX_COUNT`).

### Struktur `track_config_payload`

```
msg_type, active_alg, param_count, reserved,
params[4]  → [Kp, Kd, arrive_m, rudder_max_deg]
```

Alg 1 membutuhkan 4 parameter; Alg 2 tanpa parameter.

### Validasi tuning Alg 1

| Parameter | Default | Range |
|-----------|---------|-------|
| Kp | 1.0 | 0 – 10 |
| Kd | 0.05 | 0 – 2 |
| WP arrive (m) | 3.0 | 0.5 – 50 |
| Rudder max (°) | 40.0 | 1 – 45 |

### NVS

Namespace: `wptrack`, magic `0xA24150`.

Keys: `magic`, `alg`, `kp`, `kd`, `arrive`, `rudmax`.

Default saat NVS kosong: Alg 2, nilai default di tabel di atas.

## Telemetry ESP-NOW (kirim ke User-Side)

Struktur `DatatoSend`, update ~10 Hz (100 ms):

| Field | Tipe | Skala | Catatan |
|-------|------|-------|---------|
| timestamp | double | s | millis/1000 |
| latitude, longitude | double | ° | GNSS |
| speedMps | uint16 | ×100 | m/s |
| Calc_deg_servo_1/2 | int16 | ×100 | ° feedback rudder |
| yaw | uint16 | ×100 | 0–360° |
| heading_setpoint | uint16 | ×100 | bearing ke WP |
| heading_error | int16 | ×100 | wrap ±180° |
| rudder_cmd | int16 | ×100 | offset dari netral |
| track_wp_index | uint8 | — | 0=idle, 1..N=WP, 255=Home |
| distance_to_wp | uint16 | ×10 | meter |
| accel_x/y/z | int16 | ×100 | g |
| gyro_x/y/z | int16 | ×100 | deg/s |
| rpm_prop_1/2 | uint16 | ×100 | RPM |
| battery_1/2 | uint16 | ×100 | V |
| mode_auto | uint8 | — | 0/1/2 |

## Hardware

- **MCU**: ESP32-S3 DevKitC1-N16R8
- **RC**: FS-iA6B (PPM, GPIO 4)
- **Rudder servo**: PWM GPIO 5, feedback ADC GPIO 8/3
- **Propeller**: PWM GPIO 6/7, encoder GPIO 9/10
- **GNSS**: Serial1 GPIO 17/18
- **IMU**: Serial2 GPIO 15/16
- **Baterai**: ADC GPIO 1/2

### Mapping channel RC

- **CH1**: Rudder (manual)
- **CH3**: Propeller speed
- **CH5**: Propeller direction
- **CH6**: Auto/Manual (≥1750 = Auto)

## Konfigurasi MAC ESP-NOW

Ubah MAC User-Side di `src/main.cpp`:

```cpp
uint8_t user_side_Address[] = {0x80, 0xb5, 0x4e, 0xc1, 0xd5, 0xac};
```

MAC Remote harus cocok dengan `remote_side_Address` di firmware User-Side.

## Build & upload

```bash
cd PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-01
pio run
pio run --target upload
pio device monitor
```

Sesuaikan `upload_port` / `monitor_port` di `platformio.ini`.

## Dependencies

- ESP32Servo, TinyGPSPlus, JY901 (lokal), Preferences, ESP-NOW, WiFi

## Troubleshooting

- **ESP-NOW tidak terkirim**: cek MAC address, jarak, mode WIFI_STA
- **Waypoint tidak dipakai**: pastikan `$WPSET` sukses (`$WACK,OK,WP`) dari dashboard
- **Auto tidak jalan**: CH6 ≥ 1750, GPS valid, alg aktif di NVS
- **GPS tidak fix**: tunggu fix, cek Serial1 dan baud 115200

## Author

Chandra P — Ship Model Control System

## Versi

- **Version**: 1.1 (waypoint + NVS tuning)
- **Board**: ESP32-S3 DevKitC1-N16R8
- **Framework**: Arduino (PlatformIO)

# ESP-Now_ESP32-S3_User-Side-01

Gateway USB-serial antara PC (dashboard) dan kapal (Remote-Side) via ESP-NOW.

User-Side menerima perintah ASCII dari dashboard, meneruskan waypoint/tuning ke Remote via ESP-NOW, menunggu ACK dari Remote, lalu membalas ke PC. Telemetry dari Remote diteruskan sebagai CSV 23-kolom ke serial USB.

## Arsitektur

```
PC (Dashboard)  --USB 115200-->  User-Side  --ESP-NOW-->  Remote-Side (kapal)
                     CSV telemetry          waypoint / tuning / ACK
```

## Protokol PC → User-Side (ASCII, baris `\n`)

### Set waypoint

```
$WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>
```

- `wp_count`: 0–10 (jumlah waypoint navigasi, bukan termasuk Home)
- Minimal validasi: lat ±90°, lon ±180°, jumlah token = 3 + 2×wp_count

### Set tuning algoritma

```
$TUNSET,<alg>[,<kp>,<kd>,<arrive_m>,<rudder_max>]
```

| alg | Parameter | Contoh |
|-----|-----------|--------|
| 1 | 4 float wajib | `$TUNSET,1,1.0000,0.0500,3.00,40.00` |
| 2 | tanpa param | `$TUNSET,2` |

### Read-back tuning dari Remote (NVS)

```
$TUNGET
```

## Protokol User-Side → PC

### ACK waypoint / tuning

```
$WACK,OK,WP
$WACK,OK,TUN
$WACK,ERR,<kind>,<reason>[,<extra>]
```

`kind`: `WP` atau `TUN`.

Contoh error: `$WACK,ERR,WP,COUNT_MISMATCH`, `$WACK,ERR,TUN,KP`, `$WACK,ERR,WP,TIMEOUT`.

Timeout menunggu ACK dari Remote: **2.5 s** (`ACK_TIMEOUT_MS`).

### Balasan read-back tuning

```
$TACK,<alg>,<kp>,<kd>,<arrive_m>,<rudder_max>
$TACK,ERR,<reason>
```

Contoh sukses: `$TACK,2,1.0000,0.0500,3.00,40.00`

## Protokol ESP-NOW (User-Side ↔ Remote)

| msg_type | Arah | Fungsi |
|----------|------|--------|
| `0xA1` | User → Remote | `waypoints_payload` |
| `0xA2` | User → Remote | `track_config_payload` (set tuning) |
| `0xB1` | User → Remote | `tun_get_request` |
| `0xA3` | Remote → User | `track_config_payload` (response) |
| `0xC1` | Remote → User | `remote_ack_payload` |
| telemetry | Remote → User | `receivedfromremoteside` (64 byte) |

User-Side **tidak** mengirim `$WACK,OK` langsung setelah `esp_now_send`; ACK hanya setelah Remote membalas `0xC1`.

## Telemetry serial (23 kolom, raw fixed-point)

Diteruskan dari Remote tanpa konversi skala:

```
timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,
yaw,heading_setpoint,heading_error,rudder_cmd,track_wp_index,distance_to_wp,
accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,
rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
```

Dashboard membagi field numerik (kecuali timestamp, lat, lon, track_wp_index, mode_auto) dengan 100; `distance_to_wp` dibagi 10.

## Konfigurasi MAC ESP-NOW

Ubah MAC Remote di `src/main.cpp`:

```cpp
uint8_t remote_side_Address[] = {0x10, 0x20, 0xba, 0x4c, 0x53, 0xfc};
```

Harus cocok dengan `user_side_Address` di firmware Remote-Side.

## Alur dashboard “Send to Remote”

1. `$WPSET` → tunggu `$WACK,OK,WP`
2. `$TUNSET,...` → tunggu `$WACK,OK,TUN`
3. `$TUNGET` → bandingkan `$TACK` dengan nilai Setup (Verified)

## Build & upload

```bash
cd PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-01
pio run
pio run --target upload
pio device monitor
```

Serial monitor: 115200 baud. Sesuaikan `upload_port` di `platformio.ini`.

## Troubleshooting

- **TIMEOUT pada WP/TUN**: Remote tidak merespons — cek MAC, power, jarak ESP-NOW
- **Telemetry kosong**: Remote tidak mengirim atau MAC salah
- **$TACK,ERR,TIMEOUT**: Remote tidak membalas `0xA3` dalam 2.5 s

## Author

Chandra P — Ship Model Control System

## Versi

- **Version**: 1.0 (gateway WP/TUN/TACK)
- **Board**: ESP32-S3 DevKitC1-N16R8
- **Framework**: Arduino (PlatformIO)

# ESP-Now_ESP32-S3_Remote-Side-01

Firmware sisi kapal (Remote-Side) — sensor, actuator, waypoint tracking, NVS tuning, telemetry ESP-NOW.

## Dokumentasi lengkap

**[Ship Auto Way Maps Points Tracking.md](Ship%20Auto%20Way%20Maps%20Points%20Tracking.md)** — dokumentasi sistem end-to-end (Remote, User-Side, Dashboard, protokol, uji lapangan).

Salinan sama ada di:

- `Pythonfile/Way_Points_Tracking/Ship Auto Way Maps Points Tracking.md`
- `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-01/Ship Auto Way Maps Points Tracking.md`

## Ringkasan cepat

| Item | Detail |
|------|--------|
| Board | ESP32-S3 DevKitC1-N16R8 |
| Telemetry | ~10 Hz, struct `DatatoSend` → User-Side |
| Waypoint | Terima ESP-NOW `0xA1`, max 10 WP + Home |
| Tuning | NVS `wptrack`, default Alg 2 |
| Auto Alg 1 | Waypoint + PD (`Kp`, `Kd`, `arrive_m`, `rudder_max`) |
| MAC peer | `user_side_Address` di `src/main.cpp` |

## Build

```bash
pio run
pio run --target upload
pio device monitor
```

Path: `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-01`

## Author

Chandra P — Ship Model Control System | v1.1

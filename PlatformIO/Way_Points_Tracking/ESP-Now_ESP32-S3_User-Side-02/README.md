# ESP-Now_ESP32-S3_User-Side-02

Gateway USB-serial ↔ ESP-NOW antara PC (dashboard) dan kapal (Remote-Side).

## Dokumentasi lengkap

**[Ship Auto Way Maps Points Tracking.md](Ship%20Auto%20Way%20Maps%20Points%20Tracking.md)** — dokumentasi sistem end-to-end (Remote, User-Side, Dashboard, protokol, uji lapangan).

Salinan sama ada di:

- `Pythonfile/Way_Points_Tracking/Ship Auto Way Maps Points Tracking.md`
- `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-02/Ship Auto Way Maps Points Tracking.md`

## Ringkasan cepat

| Item | Detail |
|------|--------|
| Board | ESP32-S3 DevKitC1-N16R8 |
| Serial | 115200 baud, perintah `$WPSET` / `$TUNSET` / `$TUNGET` |
| Balasan | `$WACK,...`, `$TACK,...`, CSV telemetry 23 kolom |
| ACK timeout | 2.5 s menunggu Remote |
| MAC peer | `remote_side_Address` di `src/main.cpp` |

Perintah PC tidak di-ACK langsung — tunggu Remote (`0xC1` / `0xA3`) dulu.

## Build

```bash
pio run
pio run --target upload
pio device monitor
```

Path: `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-02`

## Author

Chandra P — Ship Model Control System | v1.2

# ESP-Now_ESP32-S3_User-Side-05

Firmware **gateway USB ↔ ESP-NOW** di sisi darat (User-Side) untuk sistem **Way Points Tracking**.

Clone dari **User-Side-04** dengan tambahan perintah **`$SHUTDOWN`** (ESP-NOW `pc_command_payload` / `0xA2`).

| Pasangan | Path |
|----------|------|
| Remote-Side | `ESP-Now_ESP32-S3_Remote-Side-05` (struct 24 field / 64 byte identik) |
| Dashboard | `Local Monitor Dashboard-beta1.5.py` |
| Mini PC | `Cpp_Files/Cpp_ReadWriteSerial-1.0` (via Remote USB Serial) |

---

## Peran

| Arah | Fungsi |
|------|--------|
| Remote → User → PC | Telemetry CSV **24 kolom** @ ~10 Hz (Serial 115200) |
| PC → User → Remote | `$WPSET,...` → ESP-NOW `waypoints_payload` (`0xA1`) |
| PC → User → Remote | `$SHUTDOWN` → ESP-NOW `pc_command_payload` (`0xA2`) |

User-Side **tidak** terhubung ke mini PC. Remote yang menulis `[WP]` / `$SHUTDOWN` ke USB Serial mini PC.

---

## Protokol Serial (PC ↔ User-Side)

### Telemetry (User → PC)

CSV 24 kolom (fixed-point), termasuk `mode_auto` dan `mini_pc_link`.

### Waypoint

```text
$WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>
$WACK,OK
$WACK,ERR,<reason>
```

### Shutdown mini PC

```text
$SHUTDOWN
$SACK,OK
$SACK,ERR,<reason>
```

`$SACK,OK` = paket `0xA2` sudah dikirim ESP-NOW (bukan konfirmasi OS sudah mati).

---

## Alur

**Send Way Points**

```text
Dashboard ($WPSET) → User-Side-05 → ESP-NOW 0xA1 → Remote-Side-05
  → [WP] di USB Serial → Cpp_ReadWriteSerial-1.0 (--print all|wp)
```

**Shutdown Mini PC** (tanpa Wi‑Fi laptop↔mini PC)

```text
Dashboard (tombol Live, hanya jika mini_pc_link=1)
  → $SHUTDOWN → User-Side-05 → ESP-NOW 0xA2 → Remote-Side-05
  → Serial "$SHUTDOWN" → Cpp_ReadWriteSerial-1.0 → shutdown OS
```

`$WPSET` / `$SHUTDOWN` tidak mengganggu stream telemetry 24 kolom.

---

## Build & Upload

```text
PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-05
```

```bash
pio run
pio run --target upload
pio device monitor
```

Sesuaikan MAC `remote_side_Address` di `main.cpp` dan port di `platformio.ini`.

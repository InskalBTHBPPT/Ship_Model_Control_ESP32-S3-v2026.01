# ESP-Now_ESP32-S3_User-Side-04

Firmware **gateway USB ↔ ESP-NOW** di sisi darat (User-Side) untuk sistem **Way Points Tracking**.

**Pasangan Remote-Side:** `ESP-Now_ESP32-S3_Remote-Side-04` (struct telemetry 24 field / 64 byte harus identik).

**Dashboard PC:** `Pythonfile/Way_Points_Tracking/Local Monitor Dashboard-beta1.4.py`

---

## Peran

| Arah | Fungsi |
|------|--------|
| Remote → User → PC | Telemetry CSV **24 kolom** @ ~10 Hz (Serial 115200) |
| PC → User → Remote | Perintah `$WPSET,...` → ESP-NOW `waypoints_payload` (`msg_type` `0xA1`) |

User-Side **tidak** terhubung ke mini PC di kapal. Setelah Remote menerima waypoint, Remote yang mencetak `[WP] ...` ke USB Serial mini PC (`Cpp_ReadWriteSerial`).

---

## Protokol Serial (PC ↔ User-Side)

### Telemetry (User → PC)

Satu baris CSV 24 kolom (fixed-point, sama urutan `DatatoSend` / dashboard beta 1.4). Termasuk `mode_auto` dan `mini_pc_link`.

### Kirim waypoint (PC → User)

```text
$WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>
```

### Balasan (User → PC)

```text
$WACK,OK
$WACK,ERR,<reason>
```

---

## Alur Send Way Points

```text
Dashboard (Send Way Points / $WPSET)
  → User-Side-04 (parse + esp_now_send 0xA1)
  → Remote-Side-04 (simpan + print [WP] ke mini PC)
  → Cpp_ReadWriteSerial (--print all|wp) menampilkan [WP] di terminal mini PC
```

Pengiriman `$WPSET` **tidak** mengganggu stream telemetry 24 kolom ke dashboard.

---

## Build & Upload

```text
PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-04
```

```bash
pio run
pio run --target upload
pio device monitor
```

Sesuaikan MAC `remote_side_Address` di `main.cpp` dan port di `platformio.ini`.

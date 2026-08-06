# Changelog

Catatan perubahan utama antar versi firmware dan dashboard **Way Points Tracking**.

---

## [Remote-Side-04] — dari `ESP-Now_ESP32-S3_Remote-Side-03`

### Ringkasan

Remote-Side-04 menambahkan integrasi **mini PC** (USB Serial) untuk kontrol rudder otomatis, memperluas telemetry ESP-NOW menjadi **24 kolom**, dan mengubah algoritma auto default dari waypoint PD ke perintah rudder dari mini PC.

### Telemetry ESP-NOW

| Aspek | Remote-Side-03 | Remote-Side-04 |
|-------|----------------|----------------|
| Jumlah field | 23 | **24** |
| Ukuran struct | 64 byte | 64 byte (tetap) |
| Field baru | — | `mini_pc_link` (0 = offline, 1 = heartbeat OK) |
| `mode_auto` | 0 manual, 1 alg1 PD, 2 stub | 0 manual, 1 alg1 PD, 2 **mini PC** |

### Kontrol auto (`AUTO_TRACK_ALG`)

| Alg | Remote-Side-03 | Remote-Side-04 |
|-----|----------------|----------------|
| **1** | Waypoint haversine + PD rudder (default) | Sama (opsional) |
| **2** | Stub kosong — rudder netral | **Rudder dari mini PC** via `timestamp,result` |
| Default | `#define AUTO_TRACK_ALG 1` | `#define AUTO_TRACK_ALG 2` |

### Protokol mini PC (USB Serial 115200)

| Arah | Format | Keterangan |
|------|--------|------------|
| ESP32 → PC | CSV 8 kolom @ 10 Hz | Hanya saat CH6 mode **auto** |
| ESP32 → PC | `[WP] ...` | Echo waypoint saat terima paket `0xA1` dari User-Side |
| PC → ESP32 | `$HB` | Heartbeat (~1 Hz); timeout 3 detik → `mini_pc_link = 0` |
| PC → ESP32 | `timestamp,result` | `result` = offset rudder (°); timestamp harus cocok dengan baris CSV input |

**Auto alg 2:** jika `mini_pc_link = 0` saat RC auto → rudder netral + peringatan `[WARN]` serial. Tidak ada fallback ke PD.

### Perubahan kode utama (`src/main.cpp`)

- Tambah `pollMiniPcSerial()`, `processMiniPcLine()`, `updateMiniPcLinkField()`
- Tambah state: `g_lastHbMs`, `g_miniPcRxLine`, `g_lastCsvTxTs`, `g_matchedResultTs`, `g_matchedRudderDeg`
- `auto_track_2()` diimplementasi penuh (bukan stub)
- CSV debug serial dipindah: hanya dikirim saat mode auto; `check_mode_auto_manual()` dipanggil sebelum cetak CSV
- `printWaypoints()` didokumentasikan untuk rantai ke `Cpp_ReadWriteSerial`

### Dokumentasi

- `src/README.md` diperbarui: protokol mini PC, alur waypoint, pasangan User-Side-04
- `platformio.ini`: komentar proyek diperbarui

---

## [User-Side-04] — dari `ESP-Now_ESP32-S3_User-Side-03`

### Ringkasan

User-Side-04 menyesuaikan struct telemetry dengan Remote-Side-04 (**24 kolom**) dan mendokumentasikan rantai waypoint ke mini PC di kapal.

### Telemetry serial (User → PC)

| Aspek | User-Side-03 | User-Side-04 |
|-------|--------------|--------------|
| Kolom CSV | 23 | **24** |
| Field baru | — | `mini_pc_link` (kolom ke-24) |
| Struct | `receivedfromremoteside` 23 field | + `uint8_t mini_pc_link` |

### Protokol waypoint (tidak berubah fungsional)

- PC → User: `$WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...`
- User → PC: `$WACK,OK` / `$WACK,ERR,<reason>`
- User → Remote: ESP-NOW `waypoints_payload` (`msg_type 0xA1`)

**Catatan baru:** setelah Remote menerima `0xA1`, Remote yang mencetak `[WP] ...` ke USB Serial mini PC. User-Side tidak berkomunikasi langsung dengan mini PC.

### Perubahan kode utama (`src/main.cpp`)

- Struct `receivedfromremoteside` + field `mini_pc_link`
- Output CSV: `mode_auto` + koma + `mini_pc_link` (bukan `println` di `mode_auto` saja)
- Banner startup: `ESP32-S3 User-Side-04`
- Komentar header: dokumentasi rantai waypoint → mini PC

### Dokumentasi

- **`src/README.md` baru** — peran gateway, protokol 24 kolom, alur Send Way Points
- `platformio.ini`: komentar proyek diperbarui

### Pasangan firmware

| Komponen | Versi |
|----------|-------|
| Remote-Side | `ESP-Now_ESP32-S3_Remote-Side-04` |
| User-Side | `ESP-Now_ESP32-S3_User-Side-04` |
| Dashboard | `Local Monitor Dashboard-beta1.4.py` |

---

## [Dashboard beta 1.4] — dari `Local Monitor Dashboard-beta1.3.py`

### Ringkasan

Dashboard beta 1.4 mendukung telemetry **24 kolom** dari User-Side-04, menampilkan status koneksi mini PC, dan memperjelas alur Send Way Points ke kapal/mini PC.

### Format telemetry

| Aspek | beta 1.3 | beta 1.4 |
|-------|----------|----------|
| `TELEMETRY_COL_COUNT` | 23 | **24** |
| Kolom baru | — | `mini_pc_link` (0/1) |
| Header log CSV | 23 kolom | + `mini_pc_link` |
| Parser | Hanya 23 kolom | **23 atau 24** kolom (backward compatible) |

### UI Live panel

- Label **Mini PC**: `CONNECTED` (hijau) / `DISCONNECTED` (merah)
- Label peringatan: `⚠ Auto aktif — Mini PC tidak terhubung` saat `mode_auto = 2` dan `mini_pc_link = 0`
- Mode auto: teks diperbarui — `Auto Alg 1 (PD)`, `Auto Mini PC`

### Send Way Points (tab Map Points)

- Protokol `$WPSET` tidak berubah
- Dokumentasi diperjelas:
  - Dashboard → User-Side → ESP-NOW `0xA1` → Remote
  - Remote mencetak `[WP] ...` ke USB Serial mini PC
  - `Cpp_ReadWriteSerial` menampilkan baris `[WP]` (`--print all|wp`)
  - Dashboard **tidak** berkomunikasi langsung ke mini PC

### Fungsi yang diubah

- `_build_telemetry_log_line()` — parameter `mini_pc_link`
- `update_live_indicators()` — parameter + tampilan status mini PC
- `poll_serial()` — parse kolom 24, fallback `mini_pc_link = 0` untuk data 23 kolom
- Judul window: `Ship Model Local Dashboard — beta 1.4`

---

## Diagram alur (versi 04)

```text
┌─────────────┐   $WPSET    ┌──────────────┐  ESP-NOW 0xA1  ┌───────────────┐
│  Dashboard  │ ──────────► │ User-Side-04 │ ─────────────► │ Remote-Side-04│
│  beta 1.4   │ ◄────────── │              │ ◄───────────── │               │
└─────────────┘  CSV 24 kol └──────────────┘  telemetry 24  └───────┬───────┘
                                                                      │ USB Serial
                                                                      ▼
                                                              ┌───────────────┐
                                                              │   Mini PC     │
                                                              │ Cpp_ReadWrite │
                                                              │ Serial        │
                                                              │  CSV 8 kolom  │
                                                              │  $HB + result │
                                                              └───────────────┘
```

---

## Migrasi dari versi 03 → 04

1. Flash **Remote-Side-04** dan **User-Side-04** berpasangan (struct 64 byte harus identik).
2. Gunakan **Dashboard beta 1.4** (parser 24 kolom).
3. Jalankan **Cpp_ReadWriteSerial** di mini PC saat mode auto alg 2:
   - Kirim `$HB` periodik
   - Baca CSV 8 kolom, kirim balik `timestamp,result`
4. Waypoint: kirim dari dashboard; verifikasi baris `[WP]` di terminal mini PC.

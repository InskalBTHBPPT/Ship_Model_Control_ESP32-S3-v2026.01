# Changelog

Catatan perubahan utama antar versi firmware dan dashboard **Way Points Tracking**.

---

## [Remote-Side-05] — dari `ESP-Now_ESP32-S3_Remote-Side-04`

### Ringkasan

Remote-Side-05 menambahkan forward perintah **shutdown mini PC** lewat ESP-NOW (`msg_type 0xA2`) ke USB Serial, tanpa mengubah telemetry 24 kolom atau protokol waypoint `0xA1`.

### Protokol baru

| Arah | Format | Keterangan |
|------|--------|------------|
| User → Remote | `pc_command_payload` (4 byte, `0xA2`, `cmd=1`) | Perintah shutdown |
| Remote → Mini PC | `$SHUTDOWN` | Baris ASCII di USB Serial |

### Perubahan kode utama (`src/main.cpp`)

- `#define PC_CMD_MSG_TYPE 0xA2`, `PC_CMD_SHUTDOWN`, struct `pc_command_payload`
- `OnDataRecv`: handle paket 4 byte `0xA2` → `Serial.println("$SHUTDOWN")`
- Header/doc: pasangan User-Side-05 + `Cpp_ReadWriteSerial-1.0`

### Dokumentasi

- `src/README.md`: alur shutdown, pasangan 05 / dashboard 1.5 / Cpp 1.0

### Tidak berubah dari 04

- Telemetry 24 field / 64 byte, CSV 8 kolom, `$HB` + `timestamp,result`, `[WP]` echo, `AUTO_TRACK_ALG` default 2

---

## [User-Side-05] — dari `ESP-Now_ESP32-S3_User-Side-04`

### Ringkasan

User-Side-05 (dari **User-Side-04**) menambahkan perintah serial **`$SHUTDOWN`** yang di-forward sebagai ESP-NOW `0xA2` ke Remote, dengan balasan **`$SACK`**.

### Protokol baru

| Arah | Format |
|------|--------|
| PC → User | `$SHUTDOWN` |
| User → PC | `$SACK,OK` / `$SACK,ERR,<reason>` |
| User → Remote | `pc_command_payload` (`0xA2`) |

`$SACK,OK` = sukses kirim ESP-NOW (bukan konfirmasi OS mini PC sudah mati).

### Perubahan kode utama (`src/main.cpp`)

- Struct `pc_command_payload` + `processShutdownCommand()`
- `processSerialLine`: cabang `$SHUTDOWN` sebelum `$WPSET`
- Banner: `ESP32-S3 User-Side-05`

### Dokumentasi

- `src/README.md` diperbarui untuk pasangan 05 / beta 1.5 / Cpp 1.0

### Tidak berubah dari 04

- CSV telemetry 24 kolom, `$WPSET` / `$WACK`, struct `receivedfromremoteside`

### Pasangan firmware

| Komponen | Versi |
|----------|-------|
| Remote-Side | `ESP-Now_ESP32-S3_Remote-Side-05` |
| User-Side | `ESP-Now_ESP32-S3_User-Side-05` |
| Dashboard | `Local Monitor Dashboard-beta1.5.py` |
| Mini PC | `Cpp_ReadWriteSerial-1.0` |

---

## [Dashboard beta 1.5] — dari `Local Monitor Dashboard-beta1.4.py`

### Ringkasan

Beta 1.5 menambahkan **Shutdown Mini PC** di tab Live (sebelah status Mini PC), lewat rantai ESP-NOW tanpa Wi‑Fi laptop↔mini PC.

### UI Live

- Tombol **Shutdown** di baris indikator Mini PC
- Enable hanya jika: serial **Connect** + `mini_pc_link == 1` (CONNECTED)
- Dialog konfirmasi sebelum kirim
- Reset state saat disconnect

### Protokol

- Kirim: `$SHUTDOWN\n`
- Terima: `$SACK,OK` / `$SACK,ERR,...` (timeout ~2 s)
- Tidak mengubah `$WPSET` / `$WACK` / parser telemetry 24 kolom

### Pasangan

- User-Side-05, Remote-Side-05, `Cpp_ReadWriteSerial-1.0`
- Judul window: `Ship Model Local Dashboard — beta 1.5`

---

## [Cpp_ReadWriteSerial-1.0] — dari `Cpp_Files/Cpp_ReadWriteSerial`

### Ringkasan

Clone folder dengan handler **`$SHUTDOWN`**: menutup port serial lalu menjalankan shutdown OS (Windows: `shutdown /s /t 5`).

### Perilaku baru

| Input serial | Aksi |
|--------------|------|
| `$SHUTDOWN` | Print ke stdout, `shutdown` OS, keluar loop |

### Tetap dari versi sebelumnya

- `$HB`, CSV 8 kolom, `[WP]` print (`--print all|csv|wp|none`), `timestamp,result`, `--rudder-mode`

### Tambahan CLI (1.0)

- `--print none` — tidak ada output ke stdout (rudder / `$HB` / `$SHUTDOWN` OS tetap jalan)

### Pasangan

- Remote-Side-05 (sumber `$SHUTDOWN` / CSV / `[WP]`)
- Dashboard beta 1.5 (tombol Shutdown)

---

## Diagram alur (versi 05 / 1.5 / 1.0)

```text
┌──────────────────┐  $WPSET / $SHUTDOWN  ┌──────────────┐  0xA1 / 0xA2  ┌───────────────┐
│ Dashboard beta   │ ───────────────────► │ User-Side-05 │ ────────────► │ Remote-Side-05│
│ 1.5              │ ◄─────────────────── │              │ ◄──────────── │               │
└──────────────────┘  CSV24 /$WACK/$SACK  └──────────────┘  telemetry 24 └───────┬───────┘
                                                                                  │ USB
                                                                                  ▼
                                                                          ┌───────────────┐
                                                                          │ Mini PC       │
                                                                          │ ReadWrite     │
                                                                          │ Serial-1.0    │
                                                                          └───────────────┘
```

---

## Migrasi dari versi 04 → 05

1. Flash **Remote-Side-05** dan **User-Side-05** berpasangan.
2. Gunakan **Dashboard beta 1.5**.
3. Di mini PC jalankan **`Cpp_ReadWriteSerial-1.0`** (bukan folder lama tanpa `$SHUTDOWN`).
4. Uji: Mini PC CONNECTED → tombol Shutdown → `$SACK,OK` → OS mati ~5 detik.
5. Waypoint / telemetry / rudder auto tetap seperti alur 04.

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

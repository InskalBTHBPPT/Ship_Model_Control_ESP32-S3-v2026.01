# Cpp_ReadWriteSerial-1.0

Versi **1.0** dari mini-PC serial bridge — clone dari `Cpp_Files/Cpp_ReadWriteSerial` dengan tambahan perintah **`$SHUTDOWN`**.

Program C++ untuk **membaca** data dari **ESP-Now_ESP32-S3_Remote-Side-05** via USB serial, mengirim heartbeat `$HB`, lalu **menulis balik** baris `timestamp,result` (rudder deg). Dipakai sebagai mini PC di kapal.

| Pasangan | Path |
|----------|------|
| Firmware Remote | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_Remote-Side-05` |
| Firmware User | `PlatformIO/Way_Points_Tracking/ESP-Now_ESP32-S3_User-Side-05` |
| Dashboard | `Pythonfile/Way_Points_Tracking/Local Monitor Dashboard-beta1.5.py` |
| Versi sebelumnya | `Cpp_Files/Cpp_ReadWriteSerial` (tanpa `$SHUTDOWN`) |

## Format input (dari ESP32 Remote)

### 1) Telemetry CSV 8 kolom (hanya saat RC auto / CH6)

```text
timestamp,lat,lon,calc_deg_servo_1,calc_deg_servo_2,yaw,gyro_z,yaw_rate
24.783,0.000000,0.000000,-5.67,-20.57,0.00,0.00,0.00
```

### 2) Waypoint echo `[WP] ...` (saat dashboard kirim `$WPSET`)

Remote mencetak ke USB Serial setelah menerima `waypoints_payload` (`0xA1`):

```text
[WP] Bytes received from User-Side: 180
[WP] msg_type=0xA1 home_valid=1 count=3
[WP] Home: -6.xxxxxx, 106.xxxxxx
[WP] #1: ...
```

### 3) Shutdown `$SHUTDOWN` (saat dashboard tekan Shutdown)

```text
Dashboard ($SHUTDOWN) → User-Side-05 → ESP-NOW 0xA2 → Remote-Side-05
  → Serial "$SHUTDOWN" → program ini → shutdown OS (~5 detik)
```

**Baud rate default:** `115200`

## Perilaku program

| Data | Tujuan | Tampil di terminal? |
|------|--------|---------------------|
| Baris CSV asli (8 kolom) | **stdout** | Ya jika `--print all` atau `csv` |
| Baris waypoint `[WP] ...` | **stdout** | Ya jika `--print all` atau `wp` |
| Baris `$SHUTDOWN` | **stdout** + aksi OS | Stdout jika bukan `--print none`; aksi OS selalu |
| Baris `timestamp,result` | **serial TX** (ke ESP32) | **Tidak** |
| Pesan info/error | **stderr** | Ya |

`--print` hanya memfilter **stdout** untuk CSV/[WP]/`$SHUTDOWN`]. Hitung rudder + `$HB` + tulis serial tetap jalan. `$SHUTDOWN` **selalu** memicu shutdown OS. `--print none` = tidak ada output ke stdout.

### Contoh stdout (`--print all`)

```text
24.783,0.000000,0.000000,-5.67,-20.57,0.00,0.00,0.00
[WP] msg_type=0xA1 home_valid=1 count=3
[WP] Home: -6.200000, 106.800000
$SHUTDOWN
```

Mode default: `--rudder-mode zero`. Uji integrasi: `--rudder-mode yawrate2` → `result = clamp(yaw_rate × 2, ±10°)`.

---

## Build

```powershell
cd "Cpp_Files\Cpp_ReadWriteSerial-1.0"
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
# atau static (tanpa DLL):
g++ -std=c++17 -static -static-libgcc -static-libstdc++ -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
```

---

## Penggunaan

```powershell
cd "Cpp_Files\Cpp_ReadWriteSerial-1.0"
.\read_write_serial.exe --port COM16 --baud 115200 --rudder-mode yawrate2 --print all
.\read_write_serial.exe --port COM16 --print csv
.\read_write_serial.exe --port COM16 --print wp
.\read_write_serial.exe --port COM16 --print none
```

### Opsi CLI

| Opsi | Default | Keterangan |
|------|---------|------------|
| `--port` | `COM16` | Port serial |
| `--baud` | `115200` | Baud rate |
| `--timeout` | `1000` | Timeout baca (ms) |
| `--print` | `all` | `all` / `csv` / `wp` / `none` |
| `--rudder-mode` | `zero` | `zero`, `yawrate2`, `demo` |
| `--op` / `--field-a` / `--field-b` | — | Hanya mode `demo` |
| `--help` / `-h` | — | Tampilkan bantuan |

---

## Deploy & auto-start

DLL (build dynamic): `libgcc_s_seh-1.dll`, `libgomp-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll` — satu folder dengan exe.

Auto-start: lihat [`startup_guide.md`](startup_guide.md). Edit `COM_PORT` / `PRINT_MODE` di `start_read_write_serial.bat`, shortcut ke `shell:startup`.

---

## Catatan

1. Port COM hanya satu aplikasi — tutup Serial Monitor sebelum jalan.
2. CSV hanya saat **RC auto**; `[WP]` saat waypoint diterima; `$SHUTDOWN` saat tombol Shutdown (butuh `mini_pc_link=CONNECTED`).
3. User Windows harus punya hak `shutdown`.
4. Setelah mati, mini PC **tidak** bisa dihidupkan dari dashboard.

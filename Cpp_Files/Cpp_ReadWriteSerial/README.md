# Cpp_ReadWriteSerial

Program C++ untuk **membaca** data dari **ESP-Now_ESP32-S3_Remote-Side-04** via USB serial, mengirim heartbeat `$HB`, lalu **menulis balik** baris `timestamp,result` (rudder deg). Dipakai sebagai mini PC di kapal.

Pasangan terkait:
- Firmware: `PlatformIO/.../ESP-Now_ESP32-S3_Remote-Side-04`
- Dashboard (shore): `Pythonfile/.../Local Monitor Dashboard-beta1.4.py` → User-Side → ESP-NOW → Remote → (baris `[WP]`) → mini PC

## Format input (dari ESP32)

### 1) Telemetry CSV 8 kolom (hanya saat RC auto / CH6)

```text
timestamp,lat,lon,calc_deg_servo_1,calc_deg_servo_2,yaw,gyro_z,yaw_rate
24.783,0.000000,0.000000,-5.67,-20.57,0.00,0.00,0.00
```

### 2) Waypoint echo `[WP] ...` (saat dashboard kirim `$WPSET`)

Remote mencetak ke USB Serial yang sama setelah menerima `waypoints_payload` (`0xA1`), misalnya:

```text
[WP] Bytes received from User-Side: 180
[WP] msg_type=0xA1 home_valid=1 count=3
[WP] Home: -6.xxxxxx, 106.xxxxxx
[WP] #1: ...
```

**Baud rate default:** `115200`

## Perilaku program

| Data | Tujuan | Tampil di terminal? |
|------|--------|---------------------|
| Baris CSV asli (8 kolom) | **stdout** | Ya jika `--print all` atau `csv` |
| Baris waypoint `[WP] ...` dari Remote | **stdout** | Ya jika `--print all` atau `wp` |
| Baris `timestamp,result` | **serial TX** (ke ESP32) | **Tidak** |
| Pesan info/error | **stderr** | Ya |

`--print` hanya memfilter **stdout**. Hitung rudder + heartbeat + tulis serial tetap berjalan.

### Contoh

**Terminal (stdout), `--print all`:**
```text
24.783,0.000000,0.000000,-5.67,-20.57,0.00,0.00,0.00
[WP] msg_type=0xA1 home_valid=1 count=3
[WP] Home: -6.200000, 106.800000
```

**Dikirim ke serial (tidak tampil di terminal):**
```text
24.783,14.90
24.883,15.74
```

Mode default: `--rudder-mode zero` (result = 0°). Untuk uji integrasi mini PC: `--rudder-mode yawrate2` → `result = clamp(yaw_rate × 2, ±10°)`.

---

## Build

### Opsi A — g++ (disarankan)

**Windows (dynamic link, butuh DLL saat deploy):**
```powershell
cd "Cpp_Files\Cpp_ReadWriteSerial"
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
```

**Windows (static link, tanpa DLL — disarankan untuk Beelink / mini PC):**
```powershell
g++ -std=c++17 -static -static-libgcc -static-libstdc++ -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
```

**Linux:**
```bash
sudo apt install build-essential
cd Cpp_Files/Cpp_ReadWriteSerial
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial
```

### Opsi B — CMake

```powershell
cmake -S . -B build
cmake --build build --config Release
```

---

## Penggunaan

```powershell
# Default: COM16, --rudder-mode zero, --print all
.\read_write_serial.exe

# Port custom
.\read_write_serial.exe --port COM16 --baud 115200

# Uji integrasi rudder dari yaw_rate
.\read_write_serial.exe --port COM16 --baud 115200 --rudder-mode yawrate2

# Filter stdout
.\read_write_serial.exe --port COM16 --print all
.\read_write_serial.exe --port COM16 --print csv
.\read_write_serial.exe --port COM16 --print wp

# Mode demo (math antar field)
.\read_write_serial.exe --rudder-mode demo --op add --field-a yaw --field-b gyro_z
```

```bash
./read_write_serial --port /dev/ttyUSB0
```

Simpan log CSV asli ke file (tanpa baris result / info):

```powershell
.\read_write_serial.exe --port COM16 --print csv 2>nul > telemetry.csv
```

### Opsi CLI

| Opsi | Default | Keterangan |
|------|---------|------------|
| `--port` | `COM16` / `/dev/ttyUSB0` | Port serial |
| `--baud` | `115200` | Baud rate |
| `--timeout` | `1000` | Timeout baca baris (ms) |
| `--print` | `all` | `all` (CSV+WP), `csv`, `wp` |
| `--rudder-mode` | `zero` | `zero`, `yawrate2`, `demo` |
| `--op` | `sub` | `add`, `sub`, `mul`, `div` (hanya mode `demo`) |
| `--field-a` | `calc_deg_servo_1` | Field operand pertama (mode `demo`) |
| `--field-b` | `calc_deg_servo_2` | Field operand kedua (mode `demo`) |

Field yang didukung: `timestamp`, `lat`, `lon`, `calc_deg_servo_1`, `calc_deg_servo_2`, `yaw`, `gyro_z`, `yaw_rate`

---

## Deploy ke PC lain (Windows)

Build dengan g++ MinGW menghasilkan exe yang **bergantung runtime DLL**. Di laptop pengembang DLL bisa sudah ada di PATH; di mini PC (mis. Beelink T5) exe bisa gagal diam-diam atau tidak jalan tanpa file berikut **di folder yang sama** dengan `read_write_serial.exe`:

| File |
|------|
| `libgcc_s_seh-1.dll` |
| `libgomp-1.dll` |
| `libstdc++-6.dll` |
| `libwinpthread-1.dll` |

Salin dari folder MinGW, misalnya `C:\msys64\ucrt64\bin\` (sesuaikan instalasi g++ Anda).

**Alternatif:** build ulang dengan flag static (lihat Opsi A di atas) — cukup satu file `read_write_serial.exe`, tanpa DLL.

---

## Auto-start di Windows (mini PC)

Lihat panduan lengkap: [`startup_guide.md`](startup_guide.md).

Ringkas: edit `COM_PORT` / `PRINT_MODE` di `start_read_write_serial.bat`, lalu pasang **shortcut** ke file itu di `shell:startup` (atau Task Scheduler At log on).

---

## Catatan

1. Tutup Serial Monitor PlatformIO sebelum menjalankan program — port COM hanya satu aplikasi.
2. CSV dari Remote hanya keluar saat **mode RC auto**; baris `[WP]` muncul saat waypoint diterima (manual/auto); heartbeat `$HB` dikirim terus.
3. Program terkait baca-saja: `Cpp_Files/Cpp_ReadSerial`.

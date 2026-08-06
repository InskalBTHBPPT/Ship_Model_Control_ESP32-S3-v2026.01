# Cpp_ReadWriteSerial

Program C++ untuk **membaca** telemetry CSV dari **ESP-Now_ESP32-S3_Remote-Side-04**, mengirim heartbeat `$HB`, lalu **menulis balik** baris `timestamp,result` (rudder deg) ke port serial yang sama. Dipakai sebagai mini PC di kapal.

## Format input (dari ESP32)

Hanya format CSV 8 kolom berikut:

```text
timestamp,lat,lon,calc_deg_servo_1,calc_deg_servo_2,yaw,gyro_z,yaw_rate
24.783,0.000000,0.000000,-5.67,-20.57,0.00,0.00,0.00
24.883,0.000000,0.000000,-4.89,-20.63,0.00,0.00,0.00
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

**Terminal (stdout):**
```text
24.783,0.000000,0.000000,-5.67,-20.57,0.00,0.00,0.00
24.883,0.000000,0.000000,-4.89,-20.63,0.00,0.00,0.00
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
# Default: COM16, operasi sub(calc_deg_servo_1, calc_deg_servo_2)
.\read_write_serial.exe

# Port custom
.\read_write_serial.exe --port COM16 --baud 115200

# Operasi lain
.\read_write_serial.exe --op add --field-a yaw --field-b gyro_z
.\read_write_serial.exe --op mul --field-a yaw_rate --field-b calc_deg_servo_1
cd "Cpp_Files\Cpp_ReadWriteSerial"
.\read_write_serial.exe --port COM16 --baud 115200 --rudder-mode yawrate2

# Filter stdout
.\read_write_serial.exe --port COM16 --print all
.\read_write_serial.exe --port COM16 --print csv
.\read_write_serial.exe --port COM16 --print wp
```

```bash
./read_write_serial --port /dev/ttyUSB0
```

Simpan log CSV asli ke file (tanpa baris result):

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

Gunakan `start_read_write_serial.bat` (edit `COM_PORT` di dalam file). Lalu pilih salah satu cara:

### Opsi 1 — Task Scheduler (disarankan)

Jalankan PowerShell **sebagai Administrator** (sesuaikan path):

```powershell
$dir = "D:\Pengujian\Ship_Model_Control_ESP32-S3 v2026.01\Cpp_Files\Cpp_ReadWriteSerial"
$action = New-ScheduledTaskAction -Execute "$dir\start_read_write_serial.bat" -WorkingDirectory $dir
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -RestartCount 999 -RestartInterval (New-TimeSpan -Minutes 1)
Register-ScheduledTask -TaskName "ShipModel_ReadWriteSerial" -Action $action -Trigger $trigger -Settings $settings -Force
```

Task Scheduler juga bisa lewat GUI: **Task Scheduler** → Create Task → trigger **At log on** → action **Start a program** → program: `start_read_write_serial.bat`, **Start in**: folder `Cpp_ReadWriteSerial`.

### Opsi 2 — Folder Startup (paling sederhana)

1. `Win+R` → ketik `shell:startup` → Enter
2. Buat **shortcut** ke `start_read_write_serial.bat`
3. Reboot / log off-on

### Tips

- Tetapkan **nomor COM tetap** di Device Manager (Properties USB serial → Port Settings → Advanced) agar `COM_PORT` tidak berubah setelah reboot.
- Batch menunggu 15 detik setelah boot supaya USB serial sempat terdeteksi.
- Jika program crash, batch otomatis restart setelah 5 detik.

---

## Catatan

1. Tutup Serial Monitor PlatformIO sebelum menjalankan program — port COM hanya satu aplikasi.
2. CSV dari Remote hanya keluar saat **mode RC auto**; heartbeat `$HB` dikirim terus (manual/auto).
3. Program terkait: `Cpp_Files/Cpp_ReadSerial` (baca saja, tanpa write).

# Cpp_ReadWriteSerial

Program C++ untuk **membaca** telemetry CSV dari **ESP-Now_ESP32-S3_Remote-Side-03**, melakukan operasi matematika ringan, lalu **menulis balik** baris `timestamp,result` ke port serial yang sama.

> **Penting:** Firmware ESP32 saat ini **belum** membaca baris `timestamp,result` — fitur terima di ESP32 **ditunda (pending)**. Program tetap menulis ke serial TX sesuai permintaan.

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
| Baris CSV asli (8 kolom) | **stdout** | Ya |
| Baris `timestamp,result` | **serial TX** (ke ESP32) | **Tidak** |
| Pesan info/error | **stderr** | Ya |

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

Default operasi: `calc_deg_servo_1 - calc_deg_servo_2` → `(-5.67) - (-20.57) = 14.90`

---

## Build

### Opsi A — g++ (disarankan)

**Windows:**
```powershell
cd "Cpp_Files\Cpp_ReadWriteSerial"
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_write_serial.exe
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
```

```bash
./read_write_serial --port /dev/ttyUSB0
```

Simpan log CSV asli ke file (tanpa baris result):

```powershell
.\read_write_serial.exe --port COM16 2>nul > telemetry.csv
```

### Opsi CLI

| Opsi | Default | Keterangan |
|------|---------|------------|
| `--port` | `COM16` / `/dev/ttyUSB0` | Port serial |
| `--baud` | `115200` | Baud rate |
| `--timeout` | `1000` | Timeout baca baris (ms) |
| `--op` | `sub` | `add`, `sub`, `mul`, `div` |
| `--field-a` | `calc_deg_servo_1` | Field operand pertama |
| `--field-b` | `calc_deg_servo_2` | Field operand kedua |

Field yang didukung: `timestamp`, `lat`, `lon`, `calc_deg_servo_1`, `calc_deg_servo_2`, `yaw`, `gyro_z`, `yaw_rate`

---

## Catatan

1. Tutup Serial Monitor PlatformIO sebelum menjalankan program — port COM hanya satu aplikasi.
2. Baris `timestamp,result` yang ditulis ke serial **bisa** masuk ke UART ESP32; parser di firmware belum ada (pending).
3. Program terkait: `Cpp_Files/Cpp_ReadSerial` (baca saja, tanpa write).

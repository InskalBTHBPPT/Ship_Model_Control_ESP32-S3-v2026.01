# Cpp_ReadSerial

Program C++ ringan untuk membaca data serial dari firmware **ESP-Now_ESP32-S3_Remote-Side-03** (atau varian dengan format CSV yang sama).

## Format data yang didukung

Program ini **hanya** untuk format CSV berikut (8 kolom, pemisah koma, satu baris per sampel @ ~10 Hz):

```text
timestamp,lat,lon,calc_deg_servo_1,calc_deg_servo_2,yaw,gyro_z,yaw_rate
9.685,0.000000,0.000000,-62.37,-67.67,0.00,0.00,0.00
9.785,0.000000,0.000000,-62.73,-67.55,0.00,0.00,0.00
9.885,0.000000,0.000000,-62.73,-67.79,0.00,0.00,0.00
```

| Kolom | Satuan | Keterangan |
|-------|--------|------------|
| `timestamp` | detik | Waktu sejak boot ESP32 |
| `lat` | derajat | Lintang GPS |
| `lon` | derajat | Bujur GPS |
| `calc_deg_servo_1` | derajat | Feedback sudut servo 1 |
| `calc_deg_servo_2` | derajat | Feedback sudut servo 2 |
| `yaw` | derajat | Heading IMU (0–360) |
| `gyro_z` | derajat/s | Laju sudut dari gyro Z |
| `yaw_rate` | derajat/s | Turunan yaw (delta yaw / dt) |

Baris header `timestamp,lat,lon,...` otomatis dilewati. Format lain **tidak** didukung.

**Baud rate default:** `115200` (sesuai `platformio.ini` Remote-Side-03).

---

## Struktur folder

```text
Cpp_Files/Cpp_ReadSerial/
  CMakeLists.txt
  README.md
  include/
    serial_port.hpp
    telemetry_parser.hpp
  src/
    main.cpp
    serial_port.cpp
```

Tidak ada library eksternal — serial port memakai API native Windows (`CreateFile`) dan Linux (`termios`).

---

## Build

### Windows (CMake + MSVC atau MinGW)

```powershell
cd "Cpp_Files\Cpp_ReadSerial"
cmake -S . -B build
cmake --build build --config Release
```

Executable:

```text
build\Release\read_serial.exe    # MSVC
build\read_serial.exe            # MinGW/Ninja
```

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake
cd Cpp_Files/Cpp_ReadSerial
cmake -S . -B build
cmake --build build -j
```

Executable: `build/read_serial`

---

## Penggunaan

### Windows

```powershell
# Default port COM16
.\build\Release\read_serial.exe

# Port dan simpan ke CSV
.\build\Release\read_serial.exe --port COM16 --baud 115200 --output telemetry.csv
```

Cek port di Device Manager atau:

```powershell
mode
```

### Linux

```bash
# Tambahkan user ke grup dialout (sekali saja, lalu logout/login)
sudo usermod -aG dialout $USER

# Cek port
ls /dev/ttyUSB* /dev/ttyACM*

# Jalankan
./build/read_serial --port /dev/ttyUSB0 --baud 115200 --output telemetry.csv
```

### Opsi CLI

| Opsi | Default | Keterangan |
|------|---------|------------|
| `--port` | `COM16` (Win) / `/dev/ttyUSB0` (Linux) | Nama port serial |
| `--baud` | `115200` | Baud rate |
| `--output` | (kosong) | Simpan baris valid ke file CSV |
| `--timeout` | `1000` | Timeout baca satu baris (ms) |
| `--help` | — | Bantuan |

Hentikan program dengan **Ctrl+C**.

---

## Contoh output

```text
[INFO] Membaca port COM16 @ 115200 baud
[INFO] Tekan Ctrl+C untuk berhenti
t=9.685 lat=0 lon=0 srv1=-62.37 srv2=-67.67 yaw=0 gyro_z=0 yaw_rate=0
t=9.785 lat=0 lon=0 srv1=-62.73 srv2=-67.55 yaw=0 gyro_z=0 yaw_rate=0
```

Jika `--output telemetry.csv` dipakai, baris mentah CSV ikut disimpan (tanpa header ganda).

---

## Catatan

1. Tutup Serial Monitor PlatformIO / Arduino IDE sebelum menjalankan program ini — port COM hanya bisa dipakai satu aplikasi.
2. `lat=0, lon=0` biasanya berarti GPS belum fix; parser tetap menerima baris tersebut.
3. Program ini untuk **pembacaan/logging** di PC, bukan kontrol kapal.

---

## Build cepat tanpa CMake (opsional)

### Windows (MinGW g++)

```powershell
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_serial.exe
```

### Linux

```bash
g++ -std=c++17 -Iinclude src/main.cpp src/serial_port.cpp -o read_serial
```

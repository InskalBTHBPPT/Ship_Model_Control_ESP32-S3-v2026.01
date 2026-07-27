# Ship Model Control ESP32-S3 - Sistem Kontrol Kapal Model

Sistem kontrol kapal model lengkap menggunakan ESP32-S3 dengan komunikasi ESP-NOW dan dashboard monitoring real-time. Sistem ini terdiri dari tiga komponen utama: **Sender (pada kapal)**, **Receiver (di darat)**, dan **Dashboard Monitoring (GUI Python)**.

## 📋 Deskripsi Proyek

Sistem ini mengimplementasikan kontrol kapal model otonom dengan kemampuan:
- **Kontrol Rudder**: Mode manual dan auto (turning, zigzag)
- **Kontrol Propeller**: 2x motor dengan monitoring RPM
- **Sensing**: GPS/GNSS, IMU (Roll/Pitch/Yaw), monitoring baterai
- **Komunikasi**: ESP-NOW peer-to-peer untuk transmisi data wireless
- **Monitoring**: Dashboard GUI real-time dengan peta, grafik, dan logging CSV

## 🏗️ Arsitektur Sistem

```
┌─────────────────────────────────────────────────────────┐
│                   KAPAL MODEL (Sender)                   │
│  ┌──────────────────────────────────────────────────┐   │
│  │          ESP32-S3 (DevKitC1-N16R8)              │   │
│  │                                                  │   │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐      │   │
│  │  │  GNSS    │  │   AHRS   │  │ Receiver │      │   │
│  │  │  Module  │  │ HWT905TTL│  │   RC     │      │   │
│  │  └──────────┘  └──────────┘  └──────────┘      │   │
│  │                                                  │   │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐      │   │
│  │  │  Servo   │  │  Motor   │  │  Encoder │      │   │
│  │  │  Rudder  │  │ Propeller│  │   RPM    │      │   │
│  │  └──────────┘  └──────────┘  └──────────┘      │   │
│  │                                                  │   │
│  │            ESP-NOW Broadcast (10 Hz)            │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
                         │
                         │ ESP-NOW (100-200m range)
                         ▼
┌─────────────────────────────────────────────────────────┐
│                  DARAT (Receiver)                        │
│  ┌──────────────────────────────────────────────────┐   │
│  │          ESP32-S3 (Receiver)                     │   │
│  │                                                  │   │
│  │         Receive Data via ESP-NOW                │   │
│  │                                                  │   │
│  │         Serial Output (USB)                     │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
                         │
                         │ Serial (115200 baud)
                         ▼
┌─────────────────────────────────────────────────────────┐
│              DASHBOARD MONITORING (PC)                   │
│  ┌──────────────────────────────────────────────────┐   │
│  │        Local Monitor Dashboard (Python)          │   │
│  │                                                  │   │
│  │  • Interactive Map (Folium)                     │   │
│  │  • Real-time Plots (PyQtGraph)                  │   │
│  │  • Live Indicators                              │   │
│  │  • CSV Logging                                  │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

## 🧩 Komponen Sistem

### 1. ESP-NOW Sender (Pada Kapal)
**Lokasi**: `Platform_IO/ESP_Now_Send_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/`

**Fungsi**:
- Membaca sinyal PPM dari receiver RC (FS-iA6B)
- Kontrol servo rudder dengan berbagai mode
- Kontrol motor propeller dengan PWM
- Membaca data GNSS (GPS) dengan update rate 10 Hz
- Membaca data IMU (HWT905TTL) untuk roll, pitch, yaw
- Mengukur RPM motor propeller menggunakan rotary encoder
- Monitoring tegangan baterai (2x baterai)
- Mengirim data via ESP-NOW ke receiver setiap 100ms

**Hardware**:
- ESP32-S3 (DevKitC1-N16R8)
- Receiver RC: FS-iA6B (PPM output)
- Servo Rudder (PWM 50Hz)
- 2x Motor Propeller dengan rotary encoder
- GNSS Module (u-blox, UART 115200 baud, 10 Hz)
- IMU: HWT905TTL (UART 57600 baud)
- ADC Feedback untuk servo
- Voltage Divider untuk monitoring baterai

**Dokumentasi Lengkap**: Lihat [`README.md`](../Platform_IO/ESP_Now_Send_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/src/README.md)

### 2. ESP-NOW Receiver (Di Darat)
**Lokasi**: `Platform_IO/ESP_Now_Receive_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/`

**Fungsi**:
- Menerima data via ESP-NOW dari sender
- Konversi data dari fixed-point (× 100) kembali ke float
- Menampilkan data dalam format CSV melalui Serial Monitor
- Tidak memerlukan hardware tambahan selain ESP32-S3

**Hardware**:
- ESP32-S3 (DevKitC1-N16R8) atau ESP32 lainnya
- Koneksi USB untuk Serial Monitor

**Dokumentasi Lengkap**: Lihat [`README.md`](../Platform_IO/ESP_Now_Receive_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/src/README.md)

### 3. Local Monitor Dashboard (PC)
**Lokasi**: `Pythonfile/`

**Fungsi**:
- Menerima data dari ESP32-S3 receiver via Serial (USB)
- **Tab "Live Data"**: Monitoring real-time dengan peta interaktif, grafik time series, dan live indicators
- **Tab "Analize Data"**: Analisis data rekaman dengan timeline slider, marker interaktif, dan kontrol peta
- Logging data ke file CSV untuk analisis
- Load dan analisis data CSV yang sudah direkam

**Software Requirements**:
- Python 3.8+ (disarankan 3.9+)
- PySide6, folium, pyqtgraph, pyserial

**Dokumentasi Lengkap**: Lihat [`Local Monitor Dashboard README.md`](../Pythonfile/Local%20Monitor%20Dashboard%20README.md)

## 📦 Struktur Data

### Format Data ESP-NOW

Data dikirim dari sender ke receiver dalam struktur `DatatoSend`:

```cpp
struct DatatoSend {
  double timestamp;          // Detik sejak boot (millis()/1000.0)
  double latitude;           // Latitude GPS
  double longitude;          // Longitude GPS
  uint16_t speedMps;         // Kecepatan (m/s × 100)
  int16_t Calc_deg_servo_1;  // Sudut servo 1 (derajat × 100)
  int16_t Calc_deg_servo_2;  // Sudut servo 2 (derajat × 100)
  int16_t roll;              // Roll angle (derajat × 100)
  int16_t pitch;             // Pitch angle (derajat × 100)
  uint16_t yaw;              // Yaw angle (derajat × 100, 0-360°)
  int16_t zigzag_yaw;        // Zigzag yaw offset (derajat × 100)
  uint16_t rpm_prop_1;       // RPM motor propeller 1
  uint16_t rpm_prop_2;       // RPM motor propeller 2
  uint16_t battery_1;        // Tegangan baterai 1 (V × 100)
  uint16_t battery_2;        // Tegangan baterai 2 (V × 100)
  uint8_t mode_auto;         // Mode: 0=manual, 1=turning left, 2=turning right, 3=zigzag 10°, 4=zigzag 20°
};
```

### Format CSV Output

Data dikirim ke Serial Monitor dan Dashboard dalam format CSV:

```csv
timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,roll,pitch,yaw,zigzag_yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
```

## 🚀 Quick Start

### Prerequisites

1. **PlatformIO**: Install di VS Code atau editor lainnya
2. **Python 3.8+**: Untuk dashboard monitoring
3. **Hardware**: 
   - 2x ESP32-S3 (1 untuk sender, 1 untuk receiver)
   - Sensor dan actuator sesuai spesifikasi sender

### Setup Workflow

#### Step 1: Setup Receiver (ESP32-S3 Receiver)

1. Buka proyek receiver di PlatformIO:
   ```bash
   cd Platform_IO/ESP_Now_Receive_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM
   ```

2. Compile dan upload firmware:
   ```bash
   pio run --target upload
   ```

3. Monitor Serial untuk melihat MAC address:
   ```bash
   pio device monitor
   ```
   Catat MAC address receiver (akan muncul saat boot).

#### Step 2: Setup Sender (ESP32-S3 Sender)

1. Buka proyek sender di PlatformIO:
   ```bash
   cd Platform_IO/ESP_Now_Send_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM
   ```

2. **PENTING**: Update MAC address receiver di `src/main.cpp`:
   ```cpp
   uint8_t broadcastAddress[] = {0x80, 0xb5, 0x4e, 0xc1, 0xd5, 0xac}; // Ganti dengan MAC address receiver Anda
   ```

3. Compile dan upload firmware:
   ```bash
   pio run --target upload
   ```

#### Step 3: Setup Dashboard Monitoring

1. Install dependencies Python:
   ```bash
   cd Pythonfile
   pip install PySide6 folium pyqtgraph pyserial
   ```

2. Jalankan dashboard:
   ```bash
   python "Local Monitor Dashboard.py"
   ```

3. Konfigurasi dashboard:
   - Pilih port COM receiver ESP32-S3
   - Set baud rate: 115200
   - Klik "Connect"

## 🔄 Workflow Penggunaan

### Monitoring Real-time (Tab Live Data)

1. **Power ON Receiver**: Nyalakan ESP32-S3 receiver terlebih dahulu
2. **Power ON Sender**: Nyalakan ESP32-S3 sender (pada kapal)
3. **Aktifkan Receiver RC**: Pastikan receiver RC aktif dan terhubung
4. **Koneksi Dashboard**: Hubungkan receiver ke PC dan buka dashboard
5. **Monitor Data**: Data akan tampil real-time di dashboard (tab "Live Data")
6. **Mulai Logging**: Klik "Start Log" untuk menyimpan data ke CSV

### Analisis Data Rekaman (Tab Analize Data)

1. **Buka Tab "Analize Data"**: Klik tab "Analize Data" di dashboard
2. **Load CSV File**: Klik "Load Recorded CSV" dan pilih file CSV hasil rekaman
3. **Navigasi Timeline**: Gunakan slider untuk navigasi waktu dalam data
4. **Interaksi Plot**: Hover mouse di plot RPM untuk melihat nilai detail
5. **Toggle Map Overlays**: Gunakan checkbox untuk menampilkan/menyembunyikan trail line dan heading line
6. **Sinkronisasi**: Semua plot dan peta akan update secara sinkron saat slider berubah

## 🎮 Mode Kontrol

Sistem mendukung 5 mode kontrol:

| Mode | Value | Deskripsi | Color (Dashboard) |
|------|-------|-----------|-------------------|
| Manual | 0 | Kontrol rudder langsung dari RC | Gray |
| Turning Left | 1 | Rudder bergerak bertahap ke kiri | Blue |
| Turning Right | 2 | Rudder bergerak bertahap ke kanan | Red |
| Zigzag 10° | 3 | Zigzag dengan sudut 10° | Amber |
| Zigzag 20° | 4 | Zigzag dengan sudut 20° | Purple |

### Mapping Channel Receiver RC

- **CH1**: Rudder control / Turning mode
  - ≤ 1250: Turning Right
  - ≥ 1750: Turning Left
  - 1250-1750: Manual control
- **CH2**: Zigzag mode selection
  - ≤ 1250: Zigzag 10°
  - ≥ 1750: Zigzag 20°
- **CH3**: Propeller speed
- **CH5**: Propeller direction
- **CH6**: Auto/Manual mode selection
  - ≥ 1750: Auto mode
  - < 1750: Manual mode

## 📊 Monitoring Dashboard

Dashboard memiliki 2 tab utama:

### Tab "Live Data" - Monitoring Real-time

#### 1. Interactive Map
- Peta Google Hybrid (satelit + label)
- Trail tracking (garis biru)
- Heading indicator (garis merah)
- Marker dengan konfigurasi rate (default: 1 marker/detik)

#### 2. Time Series Plots
- RPM Propeller 1 & 2
- Roll & Pitch
- Yaw & Zigzag Yaw
- Rudder Angle 1 & 2

#### 3. Live Indicators
- Roll, Pitch, Yaw (derajat)
- Zigzag Yaw (derajat)
- Rudder 1 & 2 (derajat)
- GPS Speed (m/s)
- RPM Propeller 1 & 2
- Battery Control & Motor (Volt) dengan color coding
- Mode Auto dengan color coding

#### 4. CSV Logging
- Auto-save ke file CSV
- Buffered writing untuk performa
- Header otomatis
- Timestamp setiap data

### Tab "Analize Data" - Analisis Data Rekaman

#### 1. Load CSV Data
- Load file CSV hasil rekaman dari tab Live Data
- Auto-parse data ke semua grafik dan peta
- Validasi format dan koordinat

#### 2. Interactive Timeline Slider
- Navigasi waktu dalam data rekaman
- Sinkronisasi semua plot dan peta
- Kontrol presisi dengan step 100ms
- Display timestamp saat ini

#### 3. Analyze Plots dengan Marker
- RPM Propeller dengan marker dan crosshair
- Roll & Pitch dengan marker terpisah
- Yaw & Zigzag dengan dual marker
- Rudder dengan marker
- Mouse hover untuk melihat nilai detail
- Marker mengikuti posisi slider

#### 4. Analyze Map Viewer
- Peta dengan trail dan heading dari data rekaman
- Toggle checkbox untuk trail line (biru)
- Toggle checkbox untuk heading line (merah)
- Start marker yang update sesuai slider
- Heading indicator yang tersinkronisasi dengan slider

## 🔧 Konfigurasi Penting

### ESP-NOW Range
- **Open space**: 100-200 meter
- **Indoor**: Lebih pendek
- Tidak memerlukan router WiFi (peer-to-peer)

### Update Rate
- **Data rate**: 10 Hz (setiap 100ms)
- **GNSS update**: 10 Hz
- **IMU update**: Berkelanjutan
- **RPM calculation**: Moving average 10 sampel

### Serial Communication
- **Baud rate**: 115200
- **Format**: CSV dengan 15 kolom
- **Line ending**: `\n` (newline)

### Pin Configuration Sender

**Input Pins**:
- GPIO 4: PPM input (Receiver RC)
- GPIO 9: Encoder motor 1
- GPIO 10: Encoder motor 2
- GPIO 8: ADC feedback servo 1
- GPIO 3: ADC feedback servo 2
- GPIO 1: ADC baterai 1
- GPIO 2: ADC baterai 2

**Output Pins**:
- GPIO 5: Servo rudder PWM (50Hz)
- GPIO 6: Motor speed PWM
- GPIO 7: Motor direction PWM

**Serial**:
- Serial 1 (GPIO 17/18): GNSS module
- Serial 2 (GPIO 15/16): HWT905TTL IMU

## 🐛 Troubleshooting Umum

### Data Tidak Terkirim (ESP-NOW)

1. **Periksa MAC Address**:
   - Pastikan MAC address receiver sudah benar di sender code
   - Gunakan Serial Monitor untuk melihat MAC address receiver

2. **Periksa Range**:
   - Pastikan sender dan receiver dalam jangkauan (100-200m)
   - Hindari penghalang logam atau beton

3. **Periksa WiFi Mode**:
   - Pastikan WiFi mode WIFI_STA di kedua device
   - Restart kedua device jika perlu

### Dashboard Tidak Menerima Data

1. **Periksa Port COM**:
   - Pastikan port COM receiver sudah benar
   - Refresh daftar port di dashboard

2. **Periksa Baud Rate**:
   - Default: 115200
   - Pastikan sesuai dengan receiver

3. **Periksa Koneksi USB**:
   - Pastikan kabel USB terhubung dengan baik
   - Periksa driver USB ESP32

### GPS Tidak Terdeteksi

1. **Periksa Koneksi Serial**:
   - Pastikan koneksi ke GNSS module benar
   - Periksa baud rate (otomatis dikonfigurasi)

2. **Tunggu GPS Fix**:
   - GPS memerlukan waktu untuk mendapatkan fix
   - Tunggu beberapa detik di area terbuka

### IMU Tidak Memberikan Data

1. **Periksa Koneksi**:
   - Pastikan koneksi serial ke HWT905TTL benar
   - Periksa baud rate (57600)

2. **Periksa Power Supply**:
   - Pastikan HWT905TTL mendapat power yang cukup
   - Periksa koneksi ground

### RPM Tidak Akurat

1. **Periksa Encoder**:
   - Pastikan rotary encoder terhubung dengan benar
   - Periksa koneksi interrupt pin (GPIO 9 dan 10)

2. **Verifikasi PPR**:
   - Default: 1 pulse per revolution
   - Sesuaikan jika menggunakan encoder dengan PPR berbeda

## 📁 Struktur Direktori Proyek

```
Ship_Model_Control_ESP32-S3/
│
├── Dokumentasi/              # Dokumentasi utama (README ini)
│   └── README.md
│
├── Platform_IO/
│   ├── ESP_Now_Send_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   └── README.md     # Dokumentasi sender
│   │   └── platformio.ini
│   │
│   └── ESP_Now_Receive_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/
│       ├── src/
│       │   ├── main.cpp
│       │   └── README.md     # Dokumentasi receiver
│       └── platformio.ini
│
├── Pythonfile/
│   ├── Local Monitor Dashboard.py
│   └── Local Monitor Dashboard README.md  # Dokumentasi dashboard
│
├── library/                  # Library Arduino/PlatformIO
│   ├── JY901/                # Driver IMU HWT905TTL
│   └── TinyGPSPlus/          # Library GPS
│
├── LogData/                  # Data logging CSV
│
├── image/                    # Gambar dokumentasi
│
└── Ino_File/                 # File Arduino IDE (legacy)
```

## 📚 Dokumentasi Detail

Untuk informasi detail tentang setiap komponen, lihat dokumentasi masing-masing:

1. **ESP-NOW Sender**: [`Platform_IO/ESP_Now_Send_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/src/README.md`](../Platform_IO/ESP_Now_Send_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/src/README.md)
2. **ESP-NOW Receiver**: [`Platform_IO/ESP_Now_Receive_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/src/README.md`](../Platform_IO/ESP_Now_Receive_RudderAngle_Propeller_GNSS_EulerAngle_EncoderRPM/src/README.md)
3. **Dashboard Monitoring**: [`Pythonfile/Local Monitor Dashboard README.md`](../Pythonfile/Local%20Monitor%20Dashboard%20README.md)

## 🔗 Referensi

### Teknologi yang Digunakan
- **ESP32-S3**: [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- **ESP-NOW**: [ESP-NOW Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- **PlatformIO**: [PlatformIO Documentation](https://docs.platformio.org/)
- **PySide6**: [PySide6 Documentation](https://doc.qt.io/qtforpython/)
- **Folium**: [Folium Documentation](https://python-visualization.github.io/folium/)
- **PyQtGraph**: [PyQtGraph Documentation](https://www.pyqtgraph.org/)

### Tutorial dan Sumber Lainnya
- [Random Nerd Tutorials - ESP-NOW](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/)
- [TinyGPSPlus Library](https://github.com/mikalhart/TinyGPSPlus)

## 📝 Catatan Penting

1. **MAC Address**: Pastikan MAC address receiver sudah dikonfigurasi di sender sebelum upload
2. **ADC Limitation**: ADC2 tidak dapat digunakan saat WiFi aktif (gunakan ADC1 saja)
3. **Update Rate**: Sistem berjalan pada 10 Hz (100ms interval)
4. **Format Data**: Semua komponen menggunakan format CSV dengan 15 kolom
5. **Power Supply**: Pastikan power supply cukup untuk semua komponen
6. **GPS Fix**: GPS memerlukan waktu untuk mendapatkan fix, terutama saat pertama kali dinyalakan
7. **ESP-NOW Range**: Jangkauan efektif 100-200m di open space, lebih pendek di indoor

## 👤 Author

**Chandra P** - Ship Model Control System

## 📄 License

Proyek ini dibuat untuk keperluan pengujian kapal model.

## 📌 Versi

- **Version**: 2.0
- **Last Update**: 2025
- **ESP32-S3**: DevKitC1-N16R8
- **Python**: 3.8+
- **Framework**: Arduino (PlatformIO) + PySide6 (Qt 6)

## 🔄 Changelog

### Version 2.0 (Dashboard)
- ✅ Menambahkan tab "Analize Data" untuk analisis data rekaman
- ✅ Fitur load CSV file untuk analisis
- ✅ Timeline slider untuk navigasi data rekaman
- ✅ Marker dan crosshair di plot Analyze
- ✅ Toggle checkbox untuk trail line dan heading line di peta Analyze
- ✅ Sinkronisasi plot dan peta dengan slider timeline
- ✅ Mouse hover interaction di plot RPM Analyze
- ✅ Update marker di peta berdasarkan posisi slider

---

**Selamat Menggunakan Sistem Kontrol Kapal Model ESP32-S3!** 🚢


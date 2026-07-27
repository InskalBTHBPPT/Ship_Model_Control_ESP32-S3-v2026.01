# ESP-Now_ESP32-S3_Remote-Side-01

Firmware sisi kapal (Remote-Side) — clone dari `ESP_Now_Send_Ver2025_revJan2026`.

Sistem kontrol kapal model menggunakan ESP32-S3 yang mengirim telemetry (rudder angle, GNSS, Euler angle, RPM encoder) ke User-Side via ESP-NOW.

**PENDING:** penerimaan waypoint dari User-Side belum di-port ke proyek ini.

## Deskripsi Proyek

Proyek ini mengimplementasikan sistem kontrol kapal model dengan fitur:
- Pembacaan sinyal PPM dari receiver RC (FS-iA6B)
- Kontrol servo rudder dengan berbagai mode (manual, turning, zigzag)
- Kontrol motor propeller dengan PWM
- Pembacaan data GNSS (GPS) dengan update rate 10 Hz
- Pembacaan data IMU (HWT905TTL) untuk roll, pitch, yaw
- Pengukuran RPM motor propeller menggunakan rotary encoder
- Monitoring tegangan baterai
- Pengiriman data via ESP-NOW ke receiver

## Hardware Requirements

- **MCU**: ESP32-S3 (DevKitC1-N16R8)
- **Receiver RC**: FS-iA6B dengan output PPM
- **Servo Rudder**: Servo standar dengan kontrol PWM 50Hz
- **Motor Propeller**: 2x motor dengan rotary encoder (PPR = 1)
- **GNSS Module**: u-blox GPS dengan UART (konfigurasi 115200 baud, 10 Hz)
- **IMU**: HWT905TTL dengan UART (57600 baud)
- **ADC Feedback**: Potensiometer untuk feedback posisi servo
- **Voltage Divider**: Untuk monitoring baterai

## Pin Configuration

### Input Pins
- **GPIO 4**: PPM input dari receiver RC (FS-iA6B)
- **GPIO 9**: Rotary encoder motor propeller 1 (pulse counter)
- **GPIO 10**: Rotary encoder motor propeller 2 (pulse counter)
- **GPIO 8**: ADC feedback servo 1 (ADC1_CH7)
- **GPIO 3**: ADC feedback servo 2 (ADC1_CH2)
- **GPIO 1**: ADC monitoring baterai 1 control, GNSS, and AHRS HWT (ADC1_CH0)
- **GPIO 2**: ADC monitoring baterai 2 servo rudder,  esc propeller, esc rpm meter and receiver RC(ADC1_CH1)

### Output Pins
- **GPIO 5**: Servo rudder PWM (LEDC channel, 50Hz, 12-bit resolution)
- **GPIO 6**: Motor propeller speed PWM (Servo library)
- **GPIO 7**: Motor propeller direction PWM (Servo library)

### Serial Communication
- **Serial 1 (GPIO 17/18)**: GNSS module (RX/TX)
- **Serial 2 (GPIO 15/16)**: HWT905TTL IMU (RX/TX)

## Fitur Kontrol

### Mode Manual
- Kontrol rudder langsung dari channel 1 receiver RC
- Range: -40° hingga +40° dari posisi netral (90°)

### Mode Auto - Turning Left
- Rudder bergerak secara bertahap ke kiri (maksimal -35°)
- Step increment: 5° per interval (default: setiap 2 iterasi)
- Dikontrol melalui channel 1 receiver RC (value ≥ 1750)

### Mode Auto - Turning Right
- Rudder bergerak secara bertahap ke kanan (maksimal +35°)
- Step increment: 5° per interval (default: setiap 2 iterasi)
- Dikontrol melalui channel 1 receiver RC (value ≤ 1250)

### Mode Auto - Zigzag 10°
- Zigzag dengan sudut 10° dari setpoint awal
- FSM dengan 10 states untuk pola zigzag
- Dikontrol melalui channel 2 receiver RC (value ≤ 1250)

### Mode Auto - Zigzag 20°
- Zigzag dengan sudut 20° dari setpoint awal
- FSM dengan 10 states untuk pola zigzag
- Dikontrol melalui channel 2 receiver RC (value ≥ 1750)

## Mapping Channel Receiver RC

- **CH1**: Rudder control / Turning mode
- **CH2**: Zigzag mode selection
- **CH3**: Propeller speed
- **CH5**: Propeller direction
- **CH6**: Auto/Manual mode selection (≥ 1750: Auto, < 1750: Manual)

## Struktur Data ESP-NOW

Data dikirim setiap 100ms dalam struktur `DatatoSend`:

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
  uint16_t rpm_prop_1;       // RPM motor propeller 1 (× 100)
  uint16_t rpm_prop_2;       // RPM motor propeller 2 (× 100)
  uint16_t battery_1;        // Tegangan baterai 1 (V × 100)
  uint16_t battery_2;        // Tegangan baterai 2 (V × 100)
  uint8_t mode_auto;         // Mode: 0=manual, 1=turning left, 2=turning right, 3=zigzag 10°, 4=zigzag 20°
};
```

## Konfigurasi ESP-NOW

### MAC Address Receiver
Ubah MAC address receiver di baris 14:
```cpp
uint8_t user_side_Address[] = {0x94, 0xa9, 0x90, 0x30, 0xab, 0xc0};
```

## Konfigurasi GNSS (u-blox)

- **Baud rate awal**: 9600
- **Baud rate akhir**: 115200 (diubah via UBX command)
- **Update rate**: 10 Hz (100 ms)
- **Protocol**: UBX + NMEA

Fungsi konfigurasi:
- `setUbxUartBaud()`: Mengatur baud rate UART
- `setUbxMeasurementRate()`: Mengatur measurement rate
- `saveUbxConfig()`: Menyimpan konfigurasi ke BBR dan Flash

## Pengukuran RPM

RPM dihitung menggunakan:
- **Moving average**: 10 sampel
- **Interval**: 100 ms per loop
- **PPR**: 1 pulse per revolution
- **Formula**: `RPM = (avg_pulses_per_loop / 0.1s) × 60 = avg_pulses_per_loop × 600`

## Kalibrasi Servo Rudder

- **Reference angle**: 90° (netral)
- **Reference duty**: 307 (7.5% duty cycle, 1.5ms)
- **Duty per degree**: 2.4 duty/derajat
- **Range**: 47.5° hingga 132.9° (duty 205-410)
- **Offset range**: -40° hingga +40°

## Kalibrasi Feedback Servo

ADC feedback dikonversi ke derajat menggunakan:
- **Servo 1**: `deg = (mV × 0.0595) - 98.848`
- **Servo 2**: `deg = (mV × 0.0594) - 98.801`

## Monitoring Baterai

- **Baterai 1**: Untuk ESP32-S3, Servo, HWT905TTL, Receiver RC, GNSS, Rotary Encoder
  - Voltage divider factor: 5×
  - ADC pin: GPIO1 (ADC1_CH0)

- **Baterai 2**: Untuk motor propeller
  - Voltage divider factor: 5×
  - ADC pin: GPIO2 (ADC1_CH1)

## Setup dan Instalasi

### 1. Install PlatformIO

Pastikan PlatformIO sudah terinstall di VS Code atau editor lainnya.

### 2. Clone/Download Proyek

```bash
cd Platform_IO/ESP_Now_Send_RudderAngle_GNSS_EulerAngle_EncoderRPM
```

### 3. Install Dependencies

Dependencies akan diinstall otomatis oleh PlatformIO berdasarkan `platformio.ini`:
- ESP32Servo
- TinyGPSPlus
- JY901 (local library)

### 4. Konfigurasi

1. Ubah MAC address receiver di `main.cpp` (baris 14)
2. Sesuaikan pin configuration jika diperlukan
3. Sesuaikan baud rate serial monitor di `platformio.ini` jika diperlukan

### 5. Compile Proyek

Compile proyek untuk memverifikasi tidak ada error sebelum upload:

```bash
pio run
```

Atau dengan verbose output untuk melihat detail compile:

```bash
pio run -v
```

Pastikan compile berhasil tanpa error sebelum melanjutkan ke langkah upload.

### 6. Upload ke ESP32-S3

Upload firmware ke ESP32-S3:

```bash
pio run --target upload
```

PlatformIO akan compile ulang otomatis sebelum upload jika ada perubahan pada code.

### 7. Monitor Serial

```bash
pio device monitor
```

Serial monitor akan menampilkan:
- Status inisialisasi
- RPM motor propeller 1 dan 2
- Total pulse counter

## Penggunaan

1. **Power ON** ESP32-S3
2. **Tunggu inisialisasi** (GPS dan IMU akan dikonfigurasi otomatis)
3. **Sambungkan receiver RC** ke pin PPM (GPIO 4)
4. **Aktifkan receiver RC** dan pastikan sinyal PPM terdeteksi
5. **Pilih mode** melalui channel 6 (Auto/Manual)
6. **Data akan dikirim** via ESP-NOW setiap 100ms

## Troubleshooting

### GPS tidak terdeteksi
- Pastikan koneksi serial ke GNSS module benar
- Periksa baud rate (akan diubah otomatis dari 9600 ke 115200)
- Tunggu beberapa detik untuk GPS mendapatkan fix

### IMU tidak memberikan data
- Periksa koneksi serial ke HWT905TTL
- Pastikan baud rate 57600
- Periksa power supply HWT905TTL

### RPM tidak akurat
- Pastikan rotary encoder terhubung dengan benar
- Periksa koneksi interrupt pin (GPIO 9 dan 10)
- Verifikasi PPR encoder (default: 1)

### ESP-NOW tidak terkirim
- Periksa MAC address receiver
- Pastikan receiver ESP32 dalam jangkauan
- Periksa status WiFi mode (harus WIFI_STA)

## Catatan Penting

1. **ADC2 tidak dapat digunakan** saat WiFi aktif (gunakan ADC1 saja)
2. **Interval waktu**: 100ms (10 Hz update rate)
3. **Servo PWM**: 50Hz dengan resolusi 12-bit
4. **PPM range**: 600-1600 µs (FS-iA6B) dimapping ke 1000-2000 µs
5. **Zigzag setpoint**: Diambil dari yaw saat masuk mode manual atau auto

## Library Dependencies

- **ESP32Servo**: Kontrol servo motor
- **TinyGPSPlus**: Parsing data GPS
- **JY901**: Driver untuk HWT905TTL IMU
- **ESP-NOW**: Komunikasi wireless peer-to-peer
- **WiFi**: Untuk ESP-NOW initialization

## License

Proyek ini dibuat untuk keperluan pengujian kapal model.

## Author

Chandra P - Ship Model Control System

## Versi

- **Version**: 1.0
- **Last Update**: 2025
- **ESP32-S3**: DevKitC1-N16R8
- **Framework**: Arduino


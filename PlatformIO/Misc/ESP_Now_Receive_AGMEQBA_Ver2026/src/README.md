# ESP32-S3 Ship Model Control - ESP-NOW Receiver

Sistem receiver ESP-NOW untuk menerima dan menampilkan data rudder angle, GNSS, Euler angle, dan RPM encoder dari kapal model.

## Deskripsi Proyek

Proyek ini adalah receiver side dari sistem kontrol kapal model yang menerima data via ESP-NOW dari sender (ESP32-S3) dan menampilkannya dalam format CSV melalui Serial Monitor. Receiver tidak memerlukan hardware tambahan selain ESP32-S3.

## Fitur

- **Menerima data via ESP-NOW** dari sender dengan struktur data yang sama
- **Format output CSV** untuk logging dan analisis data
- **Konversi data** dari fixed-point (× 100) kembali ke float untuk display
- **Real-time monitoring** data kapal model setiap 100ms
- **Tidak memerlukan hardware tambahan** selain ESP32-S3

## Hardware Requirements

- **MCU**: ESP32-S3 (DevKitC1-N16R8) atau ESP32 lainnya
- **Koneksi**: ESP-NOW peer-to-peer (tidak memerlukan router WiFi)
- **Serial Monitor**: Untuk menampilkan data (USB atau Serial)

## Struktur Data yang Diterima

Receiver menerima data dalam struktur `DatatoSend` yang sama dengan sender:

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
  uint16_t rpm_prop_1;       // RPM motor propeller 1 (direct value)
  uint16_t rpm_prop_2;       // RPM motor propeller 2 (direct value)
  uint16_t battery_1;        // Tegangan baterai 1 (V × 100)
  uint16_t battery_2;        // Tegangan baterai 2 (V × 100)
  uint8_t mode_auto;         // Mode: 0=manual, 1=turning left, 2=turning right, 3=zigzag 10°, 4=zigzag 20°
};
```

## Format Output CSV

Data dicetak ke Serial Monitor dalam format CSV dengan urutan sebagai berikut:

```
timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,roll,pitch,yaw,zigzag_yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
```

### Contoh Output:
```
123.456,-6.123456,106.789012,1.23,90.50,89.75,-0.25,0.15,180.50,-0.10,1200,1150,12.50,14.20,0
```

### Penjelasan Kolom:
- **timestamp**: Waktu sejak boot (detik, 3 desimal)
- **latitude**: Latitude GPS (6 desimal)
- **longitude**: Longitude GPS (6 desimal)
- **speedMps**: Kecepatan (m/s, 2 desimal)
- **Calc_deg_servo_1**: Sudut servo 1 (derajat, 2 desimal)
- **Calc_deg_servo_2**: Sudut servo 2 (derajat, 2 desimal)
- **roll**: Roll angle (derajat, 2 desimal)
- **pitch**: Pitch angle (derajat, 2 desimal)
- **yaw**: Yaw angle (derajat, 2 desimal, 0-360°)
- **zigzag_yaw**: Zigzag yaw offset (derajat, 2 desimal)
- **rpm_prop_1**: RPM motor propeller 1 (integer, tanpa desimal)
- **rpm_prop_2**: RPM motor propeller 2 (integer, tanpa desimal)
- **battery_1**: Tegangan baterai 1 (Volt, 2 desimal)
- **battery_2**: Tegangan baterai 2 (Volt, 2 desimal)
- **mode_auto**: Mode kontrol (0-4, integer)

## Konversi Data

Receiver melakukan konversi data dari fixed-point kembali ke float:

- **speedMps**: `dataToSend.speedMps / 100.0`
- **Calc_deg_servo_1/2**: `dataToSend.Calc_deg_servo_1 / 100.0`
- **roll/pitch/yaw**: `dataToSend.roll / 100.0`
- **zigzag_yaw**: `dataToSend.zigzag_yaw / 100.0`
- **battery_1/2**: `dataToSend.battery_1 / 100.0`
- **rpm_prop_1/2**: Direct value (tidak perlu konversi)

## Mode Auto

Receiver menampilkan mode kontrol dari sender:
- **0**: Manual mode
- **1**: Turning left
- **2**: Turning right
- **3**: Zigzag 10°
- **4**: Zigzag 20°

## Setup dan Instalasi

### 1. Install PlatformIO

Pastikan PlatformIO sudah terinstall di VS Code atau editor lainnya.

### 2. Clone/Download Proyek

```bash
cd Platform_IO/ESP_Now_Receive_RudderAngle_GNSS_EulerAngle_EncoderRPM
```

### 3. Install Dependencies

Dependencies akan diinstall otomatis oleh PlatformIO berdasarkan `platformio.ini`:
- ESP-NOW (built-in ESP32)
- WiFi (built-in ESP32)

Tidak ada library eksternal yang diperlukan.

### 4. Konfigurasi

1. Sesuaikan serial monitor baud rate di `platformio.ini` jika diperlukan (default: 115200)
2. Sesuaikan upload port dan monitor port di `platformio.ini` sesuai dengan port ESP32 Anda

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

Buka Serial Monitor untuk melihat data yang diterima:

```bash
pio device monitor
```

Atau gunakan Serial Monitor di VS Code dengan menekan tombol "Monitor" di PlatformIO toolbar.

## Penggunaan

1. **Power ON** ESP32-S3 receiver
2. **Tunggu inisialisasi** ESP-NOW (akan muncul pesan "ESP32 WROVER" di Serial Monitor)
3. **Pastikan sender aktif** dan mengirim data
4. **Data akan diterima** dan ditampilkan dalam format CSV setiap kali data diterima
5. **Simpan data** dengan menyalin output Serial Monitor atau menggunakan logging software

## Menyimpan Data ke File

### Metode 1: Copy-Paste dari Serial Monitor
1. Buka Serial Monitor
2. Copy semua output CSV
3. Paste ke file `.csv` atau `.txt`
4. Gunakan software seperti Excel atau Python untuk analisis

### Metode 2: PlatformIO Device Monitor dengan Redirect
```bash
pio device monitor > output.csv
```

### Metode 3: Python Script untuk Logging
Gunakan Python dengan `pyserial` untuk membaca Serial dan menyimpan ke file:

```python
import serial
import datetime

ser = serial.Serial('COM16', 115200)  # Sesuaikan port
with open('data_log.csv', 'a') as f:
    f.write('timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,roll,pitch,yaw,zigzag_yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto\n')
    while True:
        line = ser.readline().decode('utf-8').strip()
        if line:
            f.write(line + '\n')
            f.flush()
```

## Troubleshooting

### Tidak Ada Data yang Diterima

1. **Periksa koneksi ESP-NOW**:
   - Pastikan sender dan receiver dalam jangkauan (ESP-NOW range: ~100-200m di open space)
   - Pastikan MAC address sender sudah benar di sender code
   - Pastikan sender sudah terhubung ke receiver sebagai peer

2. **Periksa Serial Monitor**:
   - Pastikan baud rate 115200
   - Pastikan port COM sudah benar
   - Pastikan Serial Monitor tidak terhubung ke device lain

3. **Periksa status ESP-NOW**:
   - Pastikan tidak ada error "Error initializing ESP-NOW"
   - Pastikan WiFi mode sudah diset ke WIFI_STA

### Data Tidak Lengkap atau Korup

1. **Periksa ukuran data**:
   - Pastikan struktur data receiver sama dengan sender
   - Pastikan ukuran paket data tidak melebihi limit ESP-NOW (250 bytes)

2. **Periksa kecepatan transmisi**:
   - Pastikan sender tidak mengirim terlalu cepat
   - ESP-NOW memiliki limit transmisi per detik

### Serial Monitor Tidak Menampilkan Data

1. **Periksa koneksi USB**:
   - Pastikan kabel USB terhubung dengan baik
   - Coba cabut dan pasang kembali kabel USB

2. **Periksa driver USB**:
   - Pastikan driver USB ESP32 sudah terinstall
   - Coba restart komputer atau update driver

3. **Periksa port COM**:
   - Pastikan port COM di `platformio.ini` sesuai dengan port ESP32
   - Cek Device Manager untuk melihat port yang tersedia

## Catatan Penting

1. **Struktur data harus sama** dengan sender (ukuran dan tipe data)
2. **ESP-NOW tidak memerlukan router WiFi** (peer-to-peer communication)
3. **Range ESP-NOW** sekitar 100-200 meter di open space, lebih pendek di indoor
4. **Update rate**: Data diterima setiap kali sender mengirim (default: setiap 100ms)
5. **Serial baud rate**: 115200 (harus sesuai dengan konfigurasi di code)
6. **Format CSV**: Semua data dipisahkan dengan koma (`,`) dan setiap baris diakhiri dengan newline (`\n`)

## Library Dependencies

- **ESP-NOW**: Komunikasi wireless peer-to-peer (built-in ESP32)
- **WiFi**: Untuk ESP-NOW initialization (built-in ESP32)

Tidak ada library eksternal yang diperlukan. Semua library sudah built-in di ESP32 Arduino Core.

## Perbedaan dengan Sender

| Aspek | Sender | Receiver |
|-------|--------|----------|
| Hardware | ESP32-S3 + sensors + actuators | ESP32-S3 saja |
| Fungsi | Mengumpulkan data dan mengirim | Menerima data dan menampilkan |
| Output | ESP-NOW broadcast | Serial Monitor (CSV) |
| Libraries | ESP32Servo, TinyGPSPlus, JY901 | Hanya ESP-NOW dan WiFi |
| Complexity | Tinggi | Rendah |

## Integrasi dengan Sender

1. **Pastikan MAC address receiver** sudah dikonfigurasi di sender code
2. **Pastikan struktur data** sama antara sender dan receiver
3. **Upload firmware** ke receiver terlebih dahulu
4. **Upload firmware** ke sender setelah receiver aktif
5. **Monitor Serial** receiver untuk melihat data yang diterima

## License

Proyek ini dibuat untuk keperluan pengujian kapal model.

## Author

Chandra P - Ship Model Control System

## Versi

- **Version**: 1.0
- **Last Update**: 2025
- **ESP32-S3**: DevKitC1-N16R8
- **Framework**: Arduino

## Referensi

- [ESP-NOW Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [Random Nerd Tutorials - ESP-NOW](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/)
- [PlatformIO Documentation](https://docs.platformio.org/)


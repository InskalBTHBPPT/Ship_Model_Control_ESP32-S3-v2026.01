# Local Monitor Dashboard - Ship Model Control

Dashboard monitoring real-time untuk sistem kontrol kapal model menggunakan ESP32-S3. Aplikasi GUI ini menampilkan data GNSS, IMU (Roll/Pitch/Yaw), RPM propeller, rudder angle, dan status baterai dalam format interaktif dengan peta dan grafik time series.

## Deskripsi Proyek

Aplikasi Python GUI berbasis PySide6 (Qt) yang menerima data dari ESP32-S3 receiver via Serial (USB) dan menampilkannya dalam format dashboard interaktif dengan:

- **Peta Interaktif**: Menampilkan posisi kapal model dengan trail dan heading indicator
- **Time Series Plots**: Grafik real-time untuk RPM, Roll/Pitch, Yaw, dan Rudder angle
- **Live Indicators**: Nilai real-time untuk semua parameter penting
- **CSV Logging**: Mencatat data ke file CSV untuk analisis selanjutnya
- **Serial Communication**: Koneksi serial dengan ESP32-S3 receiver

## Fitur

### Tab "Live Data" - Monitoring Real-time

#### 1. Peta Interaktif
- **Google Hybrid Map**: Peta satelit dengan label (default)
- **Trail Tracking**: Garis biru yang menghubungkan semua posisi kapal
- **Heading Indicator**: Garis merah yang menunjukkan arah heading
- **Marker Control**: Konfigurasi jumlah marker per detik untuk performa
- **Zoom Control**: Kontrol zoom dan pan untuk navigasi peta

#### 2. Time Series Plots
- **RPM Propeller**: Grafik RPM motor propeller 1 dan 2 (rolling window: 50 titik)
- **Roll & Pitch**: Grafik attitude (roll dan pitch) dalam waktu
- **Yaw & Zigzag Yaw**: Grafik yaw dengan dual axis (yaw di kiri, zigzag yaw di kanan)
- **Rudder Angle**: Grafik sudut rudder 1 dan 2

#### 3. Live Indicators
- **Roll, Pitch, Yaw**: Nilai attitude dalam derajat
- **Zigzag Yaw**: Offset yaw untuk mode zigzag
- **Rudder 1 & 2**: Sudut rudder dalam derajat
- **GPS Speed**: Kecepatan dalam m/s
- **RPM Propeller 1 & 2**: RPM motor propeller
- **Battery Control & Motor**: Tegangan baterai dengan color coding
- **Mode Auto**: Status mode kontrol dengan color coding

#### 4. CSV Logging
- **Auto-save**: Data disimpan otomatis ke file CSV
- **Buffered Writing**: Menulis data dalam batch untuk performa
- **Header CSV**: Header otomatis untuk kolom data
- **Timestamp**: Setiap data memiliki timestamp

#### 5. Serial Communication
- **Auto-detect Ports**: Deteksi otomatis port COM yang tersedia
- **Configurable Baud Rate**: Pilihan baud rate (115200, 57600, dll)
- **Buffered Reading**: Membaca data dalam buffer untuk stabilitas
- **Error Handling**: Penanganan error yang robust

### Tab "Analize Data" - Analisis Data Rekaman

#### 1. Load CSV Data
- **Load Recorded CSV**: Memuat file CSV hasil rekaman dari tab Live Data
- **Auto-parse**: Parsing otomatis data CSV ke dalam grafik dan peta
- **Data Validation**: Validasi format dan koordinat data

#### 2. Interactive Timeline Slider
- **Time Navigation**: Slider untuk navigasi waktu dalam data rekaman
- **Synchronized Updates**: Update semua plot dan peta secara sinkron berdasarkan timestamp
- **Precise Control**: Kontrol presisi dengan step 100ms
- **Timestamp Display**: Menampilkan timestamp saat ini di label

#### 3. Analyze Plots dengan Marker
- **RPM Propeller Plot**: Grafik RPM dengan marker dan crosshair
- **Roll & Pitch Plot**: Grafik attitude dengan marker untuk roll dan pitch
- **Yaw & Zigzag Plot**: Grafik yaw dengan dual marker (yaw dan zigzag)
- **Rudder Plot**: Grafik rudder dengan marker
- **Mouse Hover**: Crosshair dan label nilai saat hover di plot RPM
- **Slider Sync**: Marker mengikuti posisi slider timeline

#### 4. Analyze Map Viewer
- **Map Display**: Peta interaktif dengan trail dan heading dari data rekaman
- **Trail Line Toggle**: Checkbox untuk menampilkan/menyembunyikan trail line (biru)
- **Heading Line Toggle**: Checkbox untuk menampilkan/menyembunyikan heading line (merah)
- **Start Marker**: Marker yang menunjukkan posisi saat ini berdasarkan slider
- **Heading Indicator**: Garis heading yang update sesuai posisi slider
- **Synchronized Navigation**: Peta update otomatis saat slider berubah

## Requirements

### Python Version
- **Python 3.8 atau lebih tinggi** (disarankan Python 3.9+)

### Dependencies
Install dependencies menggunakan pip:

```bash
pip install PySide6 folium pyqtgraph pyserial
```

Atau install dari `requirements.txt` (jika ada):

```bash
pip install -r requirements.txt
```

### Dependencies Detail:
- **PySide6**: Framework GUI (Qt untuk Python)
- **folium**: Library untuk membuat peta interaktif
- **pyqtgraph**: Library untuk plotting real-time
- **pyserial**: Library untuk komunikasi serial
- **QtWebEngine**: Untuk menampilkan peta HTML (termasuk dalam PySide6)

## Instalasi

### 1. Clone/Download Proyek

```bash
cd Pythonfile
```

### 2. Install Dependencies

```bash
pip install PySide6 folium pyqtgraph pyserial
```

**Note**: QtWebEngine biasanya sudah termasuk dalam PySide6, tapi jika ada masalah, install secara terpisah:

```bash
pip install PySide6-WebEngine
```

### 3. Verifikasi Instalasi

```bash
python -c "import PySide6; import folium; import pyqtgraph; import serial; print('All dependencies installed successfully')"
```

## Penggunaan

### 1. Menjalankan Aplikasi

```bash
python "Local Monitor Dashboard.py"
```

Atau dari direktori proyek:

```bash
cd Pythonfile
python "Local Monitor Dashboard.py"
```

### 2. Konfigurasi Serial

1. **Pilih Port COM**: Pilih port COM yang terhubung ke ESP32-S3 receiver
2. **Pilih Baud Rate**: Pilih baud rate (default: 115200)
3. **Pilih Map Marker Rate**: Pilih jumlah marker per detik (default: 1 marker/detik)
   - **10**: 10 marker/detik (semua data)
   - **5**: 5 marker/detik
   - **2**: 2 marker/detik
   - **1**: 1 marker/detik (default)
   - **0.5**: 0.5 marker/detik
   - **0.2**: 0.2 marker/detik
   - **0.1**: 0.1 marker/detik

### 3. Koneksi Serial

1. **Klik "Connect"**: Aplikasi akan terhubung ke ESP32-S3 receiver
2. **Pastikan Receiver Aktif**: Pastikan ESP32-S3 receiver sudah aktif dan mengirim data
3. **Monitor Data**: Data akan ditampilkan secara real-time di dashboard

### 4. Logging Data

1. **Klik "Start Log"**: Pilih lokasi file CSV untuk menyimpan data
2. **Data akan disimpan**: Data akan disimpan otomatis ke file CSV
3. **Klik "Stop Log"**: Hentikan logging data

### 5. Memantau Data (Tab Live Data)

- **Peta**: Posisi kapal akan ditampilkan di peta dengan trail biru
- **Grafik**: Grafik time series akan diperbarui secara real-time
- **Indicators**: Nilai parameter akan diperbarui secara real-time

### 6. Menganalisis Data Rekaman (Tab Analize Data)

1. **Klik "Load Recorded CSV"**: Pilih file CSV hasil rekaman dari tab Live Data
2. **Data akan dimuat**: Semua grafik dan peta akan diisi dengan data rekaman
3. **Navigasi Timeline**: Gunakan slider untuk navigasi waktu dalam data
4. **Toggle Map Overlays**: Gunakan checkbox untuk menampilkan/menyembunyikan trail line dan heading line
5. **Interaksi Plot**: Hover mouse di plot RPM untuk melihat nilai detail
6. **Sinkronisasi**: Semua plot dan peta akan update secara sinkron saat slider berubah

## Format Data CSV

Data disimpan dalam format CSV dengan header:

```csv
timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,roll,pitch,yaw,zigzag_yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
```

### Penjelasan Kolom:
- **timestamp**: Waktu sejak boot ESP32 (detik)
- **latitude**: Latitude GPS (derajat)
- **longitude**: Longitude GPS (derajat)
- **speedMps**: Kecepatan (m/s)
- **Calc_deg_servo_1**: Sudut servo 1 (derajat)
- **Calc_deg_servo_2**: Sudut servo 2 (derajat)
- **roll**: Roll angle (derajat)
- **pitch**: Pitch angle (derajat)
- **yaw**: Yaw angle (derajat, 0-360°)
- **zigzag_yaw**: Zigzag yaw offset (derajat)
- **rpm_prop_1**: RPM motor propeller 1 (integer)
- **rpm_prop_2**: RPM motor propeller 2 (integer)
- **battery_1**: Tegangan baterai control (Volt)
- **battery_2**: Tegangan baterai motor (Volt)
- **mode_auto**: Mode kontrol (0-4)

## Mode Auto

Aplikasi menampilkan mode kontrol dengan color coding:

- **0 - Manual** (Gray): Mode manual
- **1 - Turning Left** (Blue): Mode turning left
- **2 - Turning Right** (Red): Mode turning right
- **3 - Zigzag 10** (Amber): Mode zigzag 10°
- **4 - Zigzag 20** (Purple): Mode zigzag 20°

## Battery Status

Tegangan baterai ditampilkan dengan color coding:

- **< 11.5 V** (Red): Baterai rendah
- **11.5 - 12.5 V** (Yellow): Baterai sedang
- **> 12.5 V** (Green): Baterai normal

## Struktur Kode

### Class `MapWebView`
- Menangani peta interaktif dengan Folium
- Menambahkan marker dan trail
- Update heading indicator
- Method untuk Analyze tab: `move_start_marker()`, `update_slider_heading_line()`, `add_heading_line_segment()`

### Class `MainWindow`
- Main window aplikasi dengan 2 tab: "Live Data" dan "Analize Data"
- Menangani serial communication
- Menampilkan plots dan indicators
- Menangani logging CSV
- Menangani analisis data rekaman

### Methods Utama - Live Data:
- `connect_serial()`: Koneksi ke serial port
- `disconnect_serial()`: Putus koneksi serial
- `poll_serial()`: Membaca data dari serial
- `update_indicators()`: Update indicators dan plots
- `toggle_logging()`: Toggle logging CSV
- `clear_all_plots()`: Clear semua plots

### Methods Utama - Analize Data:
- `load_analyze_csv()`: Load file CSV untuk analisis
- `clear_analyze_plots()`: Clear semua data plot Analyze
- `_on_analyze_time_slider_changed()`: Handler saat slider timeline berubah
- `_update_analyze_rpm_marker()`: Update marker di plot RPM
- `_update_analyze_attitude_marker()`: Update marker di plot Roll/Pitch
- `_update_analyze_yaw_marker()`: Update marker di plot Yaw
- `_update_analyze_rudder_marker()`: Update marker di plot Rudder
- `_update_analyze_map_marker()`: Update marker dan heading di peta
- `toggle_analyze_map_blue_line()`: Toggle visibility trail line
- `toggle_analyze_map_red_line()`: Toggle visibility heading line
- `_on_analyze_rpm_mouse_moved()`: Handler hover mouse di plot RPM

## Troubleshooting

### Port COM Tidak Terdeteksi
1. **Periksa Kabel USB**: Pastikan kabel USB terhubung dengan baik
2. **Periksa Driver**: Pastikan driver USB ESP32 sudah terinstall
3. **Refresh Ports**: Klik "Refresh Ports" untuk refresh daftar port
4. **Restart Aplikasi**: Restart aplikasi jika port masih tidak terdeteksi

### Data Tidak Tampil
1. **Periksa Koneksi Serial**: Pastikan koneksi serial sudah terhubung
2. **Periksa Baud Rate**: Pastikan baud rate sesuai dengan receiver (default: 115200)
3. **Periksa Format Data**: Pastikan format data CSV sesuai (15 kolom)
4. **Periksa Receiver**: Pastikan ESP32-S3 receiver aktif dan mengirim data

### Peta Tidak Tampil
1. **Periksa Koneksi Internet**: Peta memerlukan koneksi internet untuk load tile
2. **Periksa Firewall**: Pastikan firewall tidak memblokir akses ke Google Maps
3. **Periksa QtWebEngine**: Pastikan QtWebEngine sudah terinstall

### Plot Tidak Update
1. **Periksa Data**: Pastikan data masuk dengan benar
2. **Periksa Timestamp**: Pastikan timestamp valid
3. **Clear Plots**: Coba clear plots dan reconnect

### Logging Tidak Berfungsi
1. **Periksa Permission**: Pastikan permission menulis file ada
2. **Periksa Path**: Pastikan path file valid
3. **Periksa Disk Space**: Pastikan disk space cukup

## Konfigurasi

### Default Settings
- **Baud Rate**: 115200
- **Map Marker Rate**: 1 marker/detik
- **Rolling Window**: 50 titik (5 detik @ 10Hz)
- **Base Location**: Surabaya (-7.281500, 112.798900)
- **Map Zoom**: 18
- **Serial Timeout**: 0.1 detik
- **Poll Interval**: 50 ms

### Customization
Anda dapat mengubah default settings di file `Local Monitor Dashboard.py`:

```python
# Base location (baris 265-266)
self.base_lat = -7.281500
self.base_lon = 112.798900

# Rolling window (baris 492)
self.max_points = 50  # Jumlah titik maksimal di plot

# Poll interval (baris 920)
self.serial_timer.start(50)  # Interval polling (ms)
```

## Performa

### Optimasi Performa
1. **Map Marker Rate**: Kurangi marker rate untuk performa yang lebih baik
2. **Rolling Window**: Kurangi jumlah titik di plot untuk performa yang lebih baik
3. **Buffered Writing**: Logging menggunakan buffered writing untuk performa
4. **Serial Buffer**: Serial reading menggunakan buffer untuk stabilitas

### Rekomendasi Hardware
- **CPU**: Multi-core processor (disarankan 4+ cores)
- **RAM**: Minimal 4 GB RAM
- **Storage**: Minimal 100 MB free space
- **Network**: Koneksi internet untuk load peta tile

## Catatan Penting

1. **Format Data**: Aplikasi mengharapkan format CSV dengan 15 kolom
2. **Serial Communication**: Aplikasi menggunakan buffered reading untuk stabilitas
3. **Peta Tile**: Peta memerlukan koneksi internet untuk load tile
4. **Timestamp**: Timestamp menggunakan waktu dari ESP32, bukan waktu lokal
5. **Coordinate Validation**: Koordinat 0.0, 0.0 akan diganti dengan default location
6. **Tab Navigation**: Aplikasi memiliki 2 tab: "Live Data" untuk monitoring real-time dan "Analize Data" untuk analisis data rekaman
7. **Data Synchronization**: Di tab Analyze, semua plot dan peta tersinkronisasi dengan slider timeline
8. **Marker Interpolation**: Marker di plot menggunakan interpolasi linear untuk nilai yang akurat
9. **Map Overlays**: Trail line dan heading line di peta Analyze dapat di-toggle on/off
10. **Slider Precision**: Timeline slider menggunakan step 100ms untuk kontrol yang presisi

## Integrasi dengan ESP32-S3 Receiver

Aplikasi ini dirancang untuk bekerja dengan ESP32-S3 receiver yang mengirim data dalam format CSV melalui Serial (USB). Pastikan receiver sudah dikonfigurasi dengan benar:

1. **Baud Rate**: 115200 (atau sesuai konfigurasi)
2. **Format Data**: CSV dengan 15 kolom
3. **Line Ending**: `\n` (newline)
4. **Data Rate**: 10 Hz (setiap 100ms)

## License

Proyek ini dibuat untuk keperluan pengujian kapal model.

## Author

Chandra P - Ship Model Control System

## Versi

- **Version**: 2.0
- **Last Update**: 2025
- **Python**: 3.8+
- **Framework**: PySide6 (Qt 6)

## Changelog

### Version 2.0
- ✅ Menambahkan tab "Analize Data" untuk analisis data rekaman
- ✅ Fitur load CSV file untuk analisis
- ✅ Timeline slider untuk navigasi data rekaman
- ✅ Marker dan crosshair di plot Analyze
- ✅ Toggle checkbox untuk trail line dan heading line di peta Analyze
- ✅ Sinkronisasi plot dan peta dengan slider timeline
- ✅ Mouse hover interaction di plot RPM Analyze
- ✅ Update marker di peta berdasarkan posisi slider

## Referensi

- [PySide6 Documentation](https://doc.qt.io/qtforpython/)
- [Folium Documentation](https://python-visualization.github.io/folium/)
- [PyQtGraph Documentation](https://www.pyqtgraph.org/)
- [PySerial Documentation](https://pyserial.readthedocs.io/)


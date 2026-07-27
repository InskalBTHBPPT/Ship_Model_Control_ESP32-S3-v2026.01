/**
 * @file main.cpp
 * @brief ESP32-S3 ESP-NOW Receiver untuk Ship Model Control System
 * 
 * @description
 * Aplikasi receiver ESP-NOW yang menerima data dari sender ESP32-S3 dan 
 * menampilkannya dalam format CSV melalui Serial Monitor. Receiver ini 
 * tidak memerlukan hardware tambahan selain ESP32-S3.
 * 
 * Data yang diterima:
 * - Timestamp: milidetik sejak boot (millis())
 * - GNSS (GPS): latitude, longitude (fixed-point × 10,000,000)
 * - IMU (HWT905TTL): roll, pitch, yaw, accelerometer (X, Y, Z), gyroscope (X, Y, Z), magnetometer (X, Y, Z), quaternion (Q0, Q1, Q2, Q3) - semua dalam raw LSB
 * - Battery: battery_1, battery_2 (fixed-point × 100, dalam Volt)
 * 
 * @author Chandra P - Ship Model Control System
 * @version 1.0
 * @date 2025
 * 
 * @note
 * - Struktur data harus sama dengan sender
 * - Format output: CSV dengan 21 kolom
 * - Update rate: Setiap kali data diterima (dinamis: 10ms atau 100ms dari sender)
 * - Semua data sensor dikirim dalam raw LSB dan dikonversi di receiver
 * 
 * @reference
 * Based on ESP-NOW example by Rui Santos & Sara Santos - Random Nerd Tutorials
 * https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

/**
 * @struct DatatoSend
 * @brief Struktur data yang diterima dari sender via ESP-NOW
 * 
 * @note Struktur ini HARUS sama dengan struktur di sender
 * @note Data dikirim dalam raw LSB untuk efisiensi transmisi, konversi dilakukan di receiver
 */
struct DatatoSend {
  uint32_t timestamp;     // milidetik sejak boot (millis())
  int32_t latitude;       // × 10,000,000 (presisi 7 digit, ~1.11 cm)
  int32_t longitude;      // × 10,000,000 (presisi 7 digit, ~1.11 cm)
  int16_t roll_raw;       // roll raw LSB (konversi di receiver: ÷32768×180 untuk derajat)
  int16_t pitch_raw;      // pitch raw LSB (konversi di receiver: ÷32768×180 untuk derajat)
  int16_t yaw_raw;        // yaw raw LSB (konversi di receiver: ÷32768×180 untuk derajat, jika <0: 360+yaw)
  int16_t accel_x_raw;    // akselerometer X raw LSB (konversi di receiver: ÷32768×16 untuk g)
  int16_t accel_y_raw;    // akselerometer Y raw LSB (konversi di receiver: ÷32768×16 untuk g)
  int16_t accel_z_raw;    // akselerometer Z raw LSB (konversi di receiver: ÷32768×16 untuk g)
  int16_t gyro_x_raw;     // gyroscope X raw LSB (konversi di receiver: ÷32768×2000 untuk °/s)
  int16_t gyro_y_raw;     // gyroscope Y raw LSB (konversi di receiver: ÷32768×2000 untuk °/s)
  int16_t gyro_z_raw;     // gyroscope Z raw LSB (konversi di receiver: ÷32768×2000 untuk °/s)
  int16_t mag_x_raw;      // magnetometer X raw LSB (konversi di receiver: ÷77 untuk µT)
  int16_t mag_y_raw;      // magnetometer Y raw LSB (konversi di receiver: ÷77 untuk µT)
  int16_t mag_z_raw;      // magnetometer Z raw LSB (konversi di receiver: ÷77 untuk µT)
  int16_t quat0_raw;      // quaternion Q0 raw LSB (konversi di receiver: ÷32768 untuk quaternion)
  int16_t quat1_raw;      // quaternion Q1 raw LSB (konversi di receiver: ÷32768 untuk quaternion)
  int16_t quat2_raw;      // quaternion Q2 raw LSB (konversi di receiver: ÷32768 untuk quaternion)
  int16_t quat3_raw;      // quaternion Q3 raw LSB (konversi di receiver: ÷32768 untuk quaternion)
  uint16_t battery_1;     // batere for ESP32-S3, Servo, HWT905TTL, Receiver RC, GNSS (× 100)
  uint16_t battery_2;     // batere for motor propeller (× 100)
};

DatatoSend dataToSend;  ///< Buffer untuk menyimpan data yang diterima

/**
 * @brief Callback function yang dieksekusi ketika data ESP-NOW diterima
 * 
 * @param mac MAC address dari sender
 * @param incomingData Pointer ke data yang diterima
 * @param len Panjang data dalam bytes
 * 
 * @details
 * Function ini:
 * 1. Menyalin data dari incomingData ke struktur dataToSend
 * 2. Mengkonversi data raw LSB ke nilai fisik untuk display
 * 3. Mencetak data dalam format CSV ke Serial Monitor
 * 
 * Format CSV output:
 * timestamp,latitude,longitude,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z,quat0,quat1,quat2,quat3,battery_1,battery_2
 */
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Copy data dari buffer ke struktur
  memcpy(&dataToSend, incomingData, sizeof(dataToSend));

  // Optional: Uncomment untuk debug
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  
  // ========== Konversi Raw LSB ke Nilai Fisik ==========
  // Timestamp: langsung dalam milidetik (uint32_t), tidak perlu konversi
  // Latitude/Longitude: × 10,000,000 → bagi 10,000,000 untuk mendapatkan derajat
  
  double latitude = (double)dataToSend.latitude / 10000000.0;
  double longitude = (double)dataToSend.longitude / 10000000.0;
  
  // Konversi Angle (Roll, Pitch, Yaw): raw LSB → derajat
  // Formula: (raw_LSB / 32768) × 180
  float roll = (float)dataToSend.roll_raw / 32768 * 180;
  float pitch = (float)dataToSend.pitch_raw / 32768 * 180;
  float rawYaw = (float)dataToSend.yaw_raw / 32768 * 180;
  
  // Modifikasi yaw: jika < 0, jadikan 360 + yaw
  float yaw;
  if (rawYaw < 0) {
    yaw = 360.0 + rawYaw;
  } else {
    yaw = rawYaw;
  }
  
  // Konversi Accelerometer: raw LSB → g
  // Formula: (raw_LSB / 32768) × 16
  float accel_x = (float)dataToSend.accel_x_raw / 32768 * 16;
  float accel_y = (float)dataToSend.accel_y_raw / 32768 * 16;
  float accel_z = (float)dataToSend.accel_z_raw / 32768 * 16;
  
  // Konversi Gyroscope: raw LSB → °/s
  // Formula: (raw_LSB / 32768) × 2000
  float gyro_x = (float)dataToSend.gyro_x_raw / 32768 * 2000;
  float gyro_y = (float)dataToSend.gyro_y_raw / 32768 * 2000;
  float gyro_z = (float)dataToSend.gyro_z_raw / 32768 * 2000;
  
  // Konversi Magnetometer: raw LSB → µT (microtesla)
  // Formula: raw_LSB / 77
  float mag_x = (float)dataToSend.mag_x_raw / 77;
  float mag_y = (float)dataToSend.mag_y_raw / 77;
  float mag_z = (float)dataToSend.mag_z_raw / 77;
  
  // Konversi Quaternion: raw LSB → quaternion (-1.0 hingga +1.0)
  // Formula: raw_LSB / 32768
  float quat0 = (float)dataToSend.quat0_raw / 32768;
  float quat1 = (float)dataToSend.quat1_raw / 32768;
  float quat2 = (float)dataToSend.quat2_raw / 32768;
  float quat3 = (float)dataToSend.quat3_raw / 32768;
  
  // Konversi Battery: × 100 → bagi 100 untuk mendapatkan Volt
  float battery_1 = dataToSend.battery_1 / 100.0;
  float battery_2 = dataToSend.battery_2 / 100.0;
  
  // ========== Output CSV Format ==========
  // Format: timestamp,latitude,longitude,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z,quat0,quat1,quat2,quat3,battery_1,battery_2
  Serial.print(dataToSend.timestamp); Serial.print(",");
  Serial.print(latitude, 7); Serial.print(",");  // 7 decimal places untuk presisi GPS
  Serial.print(longitude, 7); Serial.print(",");  // 7 decimal places untuk presisi GPS
  Serial.print(roll, 3); Serial.print(",");
  Serial.print(pitch, 3); Serial.print(",");
  Serial.print(yaw, 3); Serial.print(",");
  Serial.print(accel_x, 3); Serial.print(",");
  Serial.print(accel_y, 3); Serial.print(",");
  Serial.print(accel_z, 3); Serial.print(",");
  Serial.print(gyro_x, 3); Serial.print(",");
  Serial.print(gyro_y, 3); Serial.print(",");
  Serial.print(gyro_z, 3); Serial.print(",");
  Serial.print(mag_x, 3); Serial.print(",");  // 3 decimal places untuk presisi magnetometer
  Serial.print(mag_y, 3); Serial.print(",");
  Serial.print(mag_z, 3); Serial.print(",");
  Serial.print(quat0, 4); Serial.print(",");  // 4 decimal places untuk presisi quaternion
  Serial.print(quat1, 4); Serial.print(",");
  Serial.print(quat2, 4); Serial.print(",");
  Serial.print(quat3, 4); Serial.print(",");
  Serial.print(battery_1, 2); Serial.print(",");  // 2 decimal places untuk presisi battery
  Serial.println(battery_2, 2);
}

/**
 * @brief Setup function - Inisialisasi ESP32 dan ESP-NOW
 * 
 * @details
 * Function ini melakukan:
 * 1. Inisialisasi Serial Monitor (230400 baud)
 * 2. Set WiFi mode ke Station (WIFI_STA)
 * 3. Inisialisasi ESP-NOW
 * 4. Register callback function untuk menerima data
 * 
 * @note
 * - ESP-NOW memerlukan WiFi mode Station (WIFI_STA)
 * - Callback function akan dipanggil setiap kali data diterima
 * - Tidak perlu konfigurasi peer karena receiver menerima dari semua sender
 */
void setup() {
  // ========== Serial Monitor Initialization ==========
  Serial.begin(230400);
  // Serial.println("ESP32 WROVER");
  delay(100);
  
  // ========== WiFi Configuration ==========
  // Set device sebagai WiFi Station (required untuk ESP-NOW)
  WiFi.mode(WIFI_STA);

  // ========== ESP-NOW Initialization ==========
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;  // Stop execution jika ESP-NOW gagal diinisialisasi
  }
  
  // ========== Register Receive Callback ==========
  // Register callback function untuk menerima data ESP-NOW
  // Callback akan dipanggil setiap kali data diterima dari sender
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  
  Serial.println("ESP-NOW Receiver initialized. Waiting for data...");
}

/**
 * @brief Loop function - Main program loop
 * 
 * @details
 * Function ini kosong karena semua processing dilakukan di callback function.
 * ESP-NOW menggunakan interrupt-based communication, jadi tidak perlu polling.
 * 
 * @note
 * - Semua data processing dilakukan di OnDataRecv() callback
 * - Loop() hanya untuk menjaga program tetap running
 */
void loop() {
  // ESP-NOW menggunakan callback-based communication
  // Tidak perlu polling di loop()
  delay(100);  // Small delay untuk mengurangi CPU usage
}
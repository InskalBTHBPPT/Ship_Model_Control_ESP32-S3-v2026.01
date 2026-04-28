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
 * - GNSS (GPS): latitude, longitude, speed
 * - IMU (HWT905TTL): roll, pitch, yaw
 * - Servo feedback: Calc_deg_servo_1, Calc_deg_servo_2
 * - RPM motor propeller: rpm_prop_1, rpm_prop_2
 * - Battery voltage: battery_1, battery_2
 * - Control mode: mode_auto
 * 
 * @author Chandra P - Ship Model Control System
 * @version 1.0
 * @date 2025
 * 
 * @note
 * - Struktur data harus sama dengan sender
 * - Format output: CSV dengan 15 kolom
 * - Update rate: Setiap kali data diterima (default: 10 Hz dari sender)
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
 * @note Data menggunakan fixed-point format (× 100) untuk efisiensi transmisi
 */
struct DatatoSend {
  double timestamp;       // detik sejak boot (millis()/1000.0)
  double latitude;        // dari latestGpsData.latitude
  double longitude;       // dari latestGpsData.longitude
  uint16_t speedMps;      // dari latestGpsData.speedMps (× 100, range 0-655.35 m/s)
  int16_t Calc_deg_servo_1; // derajat hasil kalkulasi feedback servo 1 (× 100)
  int16_t Calc_deg_servo_2; // derajat hasil kalkulasi feedback servo 2 (× 100)
  int16_t roll;           // derajat roll (× 100)
  int16_t pitch;          // derajat pitch (× 100)
  uint16_t yaw;           // derajat yaw (× 100, 0-360°)
  int16_t zigzag_yaw;     // zigzag yaw (× 100)
  uint16_t rpm_prop_1;    // rpm motor propeller 1 (direct RPM value, range 0-65535)
  uint16_t rpm_prop_2;    // rpm motor propeller 2 (direct RPM value, range 0-65535)
  uint16_t battery_1;     // batere for ESP32-S3, Servo, HWT905TTL, Receiver RC, GNSS, Rotary Encoder (× 100)
  uint16_t battery_2;     // batere for motor propeller (× 100)
  uint8_t mode_auto;      // 0: manual, 1: turning left, 2: turning right, 3: zigzag 10, 4: zigzag 20
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
 * 2. Mengkonversi data fixed-point (× 100) kembali ke float
 * 3. Mencetak data dalam format CSV ke Serial Monitor
 * 
 * Format CSV output:
 * timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,
 * roll,pitch,yaw,zigzag_yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
 */
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Copy data dari buffer ke struktur
  memcpy(&dataToSend, incomingData, sizeof(dataToSend));

  // Optional: Uncomment untuk debug
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  
  // ========== Konversi Fixed-Point ke Float ==========
  // Data dikirim dalam format fixed-point (× 100) untuk efisiensi
  // Konversi kembali ke float dengan membagi 100
  float speedMps = dataToSend.speedMps / 100.0;
  float Calc_deg_servo_1 = dataToSend.Calc_deg_servo_1 / 100.0;
  float Calc_deg_servo_2 = dataToSend.Calc_deg_servo_2 / 100.0;
  float roll = dataToSend.roll / 100.0;
  float pitch = dataToSend.pitch / 100.0;
  float yaw = dataToSend.yaw / 100.0;
  float zigzag_yaw = dataToSend.zigzag_yaw / 100.0;
  uint16_t rpm_prop_1 = dataToSend.rpm_prop_1;  // Direct RPM value (no conversion needed)
  uint16_t rpm_prop_2 = dataToSend.rpm_prop_2;  // Direct RPM value (no conversion needed)
  float battery_1 = dataToSend.battery_1 / 100.0;  // Volt (× 100)
  float battery_2 = dataToSend.battery_2 / 100.0;  // Volt (× 100)
  
  // ========== Output CSV Format ==========
  // Format: timestamp,latitude,longitude,speedMps,Calc_deg_servo_1,Calc_deg_servo_2,
  //         roll,pitch,yaw,zigzag_yaw,rpm_prop_1,rpm_prop_2,battery_1,battery_2,mode_auto
  Serial.print(dataToSend.timestamp, 3); Serial.print(",");
  Serial.print(dataToSend.latitude, 6); Serial.print(",");
  Serial.print(dataToSend.longitude, 6); Serial.print(",");
  Serial.print(speedMps, 2); Serial.print(",");
  Serial.print(Calc_deg_servo_1, 2); Serial.print(",");
  Serial.print(Calc_deg_servo_2, 2); Serial.print(",");
  Serial.print(roll, 2); Serial.print(",");
  Serial.print(pitch, 2); Serial.print(",");
  Serial.print(yaw, 2); Serial.print(",");
  Serial.print(zigzag_yaw, 2); Serial.print(",");
  Serial.print(rpm_prop_1); Serial.print(",");  // Integer RPM, no decimal places
  Serial.print(rpm_prop_2); Serial.print(",");  // Integer RPM, no decimal places
  Serial.print(battery_1, 2); Serial.print(",");
  Serial.print(battery_2, 2); Serial.print(",");
  Serial.println(dataToSend.mode_auto);  // Mode: 0=manual, 1=turning left, 2=turning right, 3=zigzag 10°, 4=zigzag 20°
}

/**
 * @brief Setup function - Inisialisasi ESP32 dan ESP-NOW
 * 
 * @details
 * Function ini melakukan:
 * 1. Inisialisasi Serial Monitor (115200 baud)
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
  Serial.begin(115200);
  Serial.println("ESP32 WROVER");
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
/**
 * @file main.cpp
 * @brief ESP32-S3 ESP-NOW Sender untuk Ship Model Control System
 * 
 * @description
 * Aplikasi sender ESP-NOW yang mengumpulkan data dari berbagai sensor dan 
 * actuator, kemudian mengirimkannya ke receiver via ESP-NOW.
 * 
 * Hardware yang digunakan:
 * - Receiver RC (FS-iA6B) dengan output PPM
 * - Servo rudder
 * - Motor propeller
 * - GNSS module (u-blox GPS) via Serial1
 * - IMU (HWT905TTL) via Serial2
 * - ADC untuk monitoring baterai
 * 
 * Fitur kontrol:
 * - Mode Manual: Kontrol rudder langsung dari RC
 * - Interval time dinamis: CH6 >= 1750 → 10ms (100 Hz), else → 100ms (10 Hz)
 * 
 * @author Chandra P - Ship Model Control System
 * @version 1.0
 * @date 2025
 * 
 * @note
 * - Update rate: Dinamis berdasarkan CH6 PPM (10ms atau 100ms)
 * - Data dikirim dalam format fixed-point (× 100) untuk efisiensi
 * - Struktur data harus sama dengan receiver
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <JY901.h>
#include <esp_now.h>
#include <WiFi.h>

// ============================================================================
// ESP-NOW Configuration
// ============================================================================

/**
 * @brief MAC Address dari ESP32-S3 receiver
 * 
 * @note UBAH MAC ADDRESS INI sesuai dengan MAC address receiver Anda
 * Cara mendapatkan MAC address receiver:
 * 1. Upload code receiver ke ESP32
 * 2. Buka Serial Monitor
 * 3. MAC address akan ditampilkan saat boot
 */
// Contoh MAC address yang pernah digunakan:
// uint8_t broadcastAddress[] = {0x7c, 0x9e, 0xbd, 0xe4, 0x2a, 0x20}; // WROOM
// uint8_t broadcastAddress[] = {0xb4, 0xe6, 0x2d, 0xba, 0x07, 0x5d}; // WROVER
// uint8_t broadcastAddress[] = {0xac, 0x15, 0x18, 0xed, 0x9c, 0xe0}; // WROVER 2 Non Cam
uint8_t broadcastAddress[] = {0x80, 0xb5, 0x4e, 0xc1, 0xd5, 0xac};

esp_now_peer_info_t peerInfo;  ///< Peer info untuk ESP-NOW communication

/**
 * @brief Callback function yang dieksekusi ketika data ESP-NOW terkirim
 * 
 * @param mac_addr MAC address dari receiver
 * @param status Status pengiriman (ESP_NOW_SEND_SUCCESS atau ESP_NOW_SEND_FAIL)
 * 
 * @details
 * Function ini dipanggil setelah setiap pengiriman data ESP-NOW.
 * Digunakan untuk monitoring status pengiriman (optional, bisa di-comment).
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.print("\r\nLast Packet Send Status:\t");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// ============================================================================
// PPM (Pulse Position Modulation) Configuration
// ============================================================================

/**
 * @brief Konfigurasi PPM untuk receiver RC (FS-iA6B)
 * 
 * @details
 * PPM adalah format sinyal yang digunakan oleh receiver RC untuk mengirim
 * data dari semua channel dalam satu sinyal. Setiap channel direpresentasikan
 * oleh pulse width (dalam mikrodetik).
 */
#define PPM_PIN 4                    ///< GPIO pin untuk input PPM
#define CHANNEL_COUNT 8              ///< Jumlah channel PPM (FS-iA6B memiliki 8 channel)
#define PPM_SYNC_THRESHOLD 2500      ///< Threshold untuk sync pulse (µs) - nilai > ini adalah sync pulse
#define PPM_MIN_CHANNEL_VALUE 600    ///< Nilai minimum channel (µs) - FS-iA6B lowest
#define PPM_MAX_CHANNEL_VALUE 1600   ///< Nilai maksimum channel (µs) - FS-iA6B highest

// ============================================================================
// Timing Configuration
// ============================================================================

/**
 * @brief Interval timing untuk main loop
 * 
 * @details
 * Main loop berjalan setiap 100ms (10 Hz update rate).
 * Interval ini menentukan frekuensi update data dan pengiriman ESP-NOW.
 */
unsigned long previousMillis = 0;    ///< Waktu millis() sebelumnya untuk interval timing
long intervaltime = 100;              ///< Interval waktu dalam ms (dinamis: 10ms atau 100ms berdasarkan PPM CH6)

// ============================================================================
// Control Input Structure
// ============================================================================

/**
 * @struct ControlInput
 * @brief Struktur untuk menyimpan input kontrol dari receiver RC
 * 
 * @details
 * Data berasal dari PPM receiver RC yang sudah di-mapping ke range 1000-2000 µs
 * (standard servo range).
 */
struct ControlInput {
    uint16_t rudder;           ///< Channel 1: Kontrol rudder
    uint16_t propSpeed;        ///< Channel 3: Kecepatan motor propeller
    uint16_t propDirection;    ///< Channel 5: Arah motor propeller
    uint16_t intervaltime_mode; ///< Channel 6: Mode interval time (>=1750: 10ms, else: 100ms)
};

volatile ControlInput controlInput = {0};  ///< Buffer untuk input kontrol dari RC
volatile uint16_t ppm_values[CHANNEL_COUNT] = {0};      ///< Nilai PPM mentah (600-1600 µs)
volatile uint16_t ppm_mapped[CHANNEL_COUNT] = {0};     ///< Nilai PPM yang sudah di-mapping (1000-2000 µs)
volatile uint8_t current_channel = 0;      ///< Channel PPM saat ini yang sedang diproses
volatile uint32_t rising_time = 0;         ///< Waktu rising edge untuk menghitung pulse width

// ============================================================================
// Servo Rudder Configuration
// ============================================================================

/**
 * @brief Konfigurasi Servo Rudder
 * 
 * @details
 * Servo menggunakan LEDC (LED Controller) dengan resolusi 12-bit (0-4095).
 * Servo standar menggunakan pulse width 1.0ms - 2.0ms pada frekuensi 50Hz.
 * 
 * Duty cycle calculation:
 * - 1.0ms pulse = 5% duty cycle = 205 (dari 4095)
 * - 1.5ms pulse = 7.5% duty cycle = 307 (neutral)
 * - 2.0ms pulse = 10% duty cycle = 410
 */
const uint32_t SERVO_DUTY_MIN = 205;      ///< Duty minimum (5% = 1.0ms pulse)
const uint32_t SERVO_DUTY_NEUTRAL = 307;  ///< Duty neutral (7.5% = 1.5ms pulse)
const uint32_t SERVO_DUTY_MAX = 410;      ///< Duty maksimum (10% = 2.0ms pulse)
uint32_t servo_duty = 307;  ///< Duty saat ini (start di posisi neutral)

/**
 * @brief Kalibrasi Servo Angle
 * 
 * @details
 * Konversi dari sudut (derajat) ke duty value:
 * - Reference: 307 duty = 90 derajat (neutral)
 * - Conversion: 12 duty = 5 derajat, jadi 1 derajat = 2.4 duty
 * - Range: 47.5° hingga 132.9° (sesuai dengan duty 205-410)
 */
const float REFERENCE_ANGLE = 90.0;        ///< Sudut referensi (neutral position)
const uint32_t REFERENCE_DUTY = 307;       ///< Duty referensi (neutral position)
const float DUTY_PER_DEGREE = 12.0 / 5.0; ///< Konversi: 2.4 duty per derajat
const float ANGLE_MIN = 47.5;              ///< Sudut minimum (205 duty)
const float ANGLE_MAX = 132.9;             ///< Sudut maksimum (410 duty)
uint32_t current_angle = 90;               ///< Sudut saat ini (start di 90°)
float servo_angle_current_offset = 0;     ///< Offset sudut dari posisi neutral (-40° hingga +40°)

// ============================================================================
// Control Mode Flags
// ============================================================================


// Servo Pin Configuration
// #define SERVO_RUDDER_PIN 5
uint8_t SERVO_RUDDER_pin = 5;
uint32_t SERVO_RUDDER_freq = 50;
uint8_t SERVO_RUDDER_resolution = 12;

// LEDC Configuration untuk Propeller Speed
uint8_t PROP_SPEED_pin = 6;  // GPIO 6 (SERVO_PROP_SPEED_PIN)
uint32_t PROP_SPEED_freq = 50;  // 50 Hz (sama seperti servo)
uint8_t PROP_SPEED_resolution = 12;  // 12-bit resolution

// LEDC Configuration untuk Propeller Direction
uint8_t PROP_DIRECTION_pin = 7;  // GPIO 7 (SERVO_PROP_DIRECTION_PIN)
uint32_t PROP_DIRECTION_freq = 50;  // 50 Hz (sama seperti servo)
uint8_t PROP_DIRECTION_resolution = 12;  // 12-bit resolution

// ADC Pin Configuration
// Jangan gunakan grup ADC2 jika akan menggunakan fitur WIFI ESP32-S3
uint8_t ADC_PIN_BATT_1 = 1;   // GPIO1 = ADC1_0 pin for analog input (ESP32-S3)
uint8_t ADC_PIN_BATT_2 = 2;   // GPIO2 = ADC1_1 pin for analog input (ESP32-S3)

// PWM for propeller
#define SERVO_PROP_SPEED_PIN 6
#define SERVO_PROP_DIRECTION_PIN 7

// Servo PWM parameters (range: 1000-2000 µs)
#define SERVO_MIN_US 1000
#define SERVO_MAX_US 2000
#define SERVO_HZ 50

// Note: servoPropSpeed dan servoPropDirection tidak digunakan lagi
// Semua PWM propeller sekarang menggunakan LEDC
// Servo servoPropSpeed;
// Servo servoPropDirection;

// Define the RX and TX pins for Serial 1 for GNSS
#define RXD1 18
#define TXD1 17

#define GPS_BAUD 9600

// The TinyGPS++ object
TinyGPSPlus gps;

// Create an instance of the HardwareSerial class for Serial 1
HardwareSerial gpsSerial(1);

// Simple container for parsed GPS data (snapshot per interval)
struct GpsData {
  double latitude;
  double longitude;
  // double altitudeM;
  // double hdop;
  // uint32_t satellites;
  // bool locationValid;
  // bool timeValid;
  // unsigned long locationAgeMs;
  // unsigned long timeAgeMs;
};

GpsData latestGpsData{};

// Define the RX and TX pins for Serial 2 for hwt905ttl
#define RXD2 16
#define TXD2 15

//#define HWT905TTL_BAUD 57600
#define HWT905TTL_BAUD 115200

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial HWT905TTL_Serial(2);

// Struct data untuk pengiriman/logging ringkas
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

DatatoSend dataToSend; 

// Utility: map long to float with custom output range
static inline float mapFloat(long x, long in_min, long in_max, float out_min, float out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

/**
 * @brief Interrupt handler untuk PPM signal
 * 
 * @details
 * Function ini dipanggil setiap kali terjadi perubahan level pada PPM_PIN
 * (CHANGE interrupt). Menggunakan edge detection untuk mengukur pulse width.
 * 
 * PPM Format:
 * - Sync pulse: Pulse panjang (>2500µs) menandai awal frame
 * - Channel pulses: Pulse pendek (600-1600µs) adalah nilai channel
 * 
 * @note
 * Function ini harus dideklarasikan dengan IRAM_ATTR karena dipanggil dari interrupt.
 * Semua variabel yang diakses harus volatile.
 */
void IRAM_ATTR ppm_interrupt_handler() {
  uint32_t current_time = micros();
  
  if (digitalRead(PPM_PIN) == HIGH) {
      // ========== RISING EDGE ==========
      // Simpan waktu untuk menghitung pulse width
      rising_time = current_time;
  } else {
      // ========== FALLING EDGE ==========
      // Hitung durasi HIGH (pulse width = channel value)
      uint32_t pulse_width = current_time - rising_time;
      
      // Detect sync pulse (HIGH period panjang menandai awal frame)
      if (pulse_width > PPM_SYNC_THRESHOLD) {
          current_channel = 0;  // Reset ke channel pertama
      } else {
          // Simpan channel value jika dalam range valid
          if (current_channel < CHANNEL_COUNT) {
              ppm_values[current_channel] = pulse_width;
              current_channel++;
          }
      }
  }
}

// ---------------------- u-blox helpers (CFG-PRT / CFG-RATE / CFG-CFG) ----------------------
// UBX checksum calculator (CK_A, CK_B) over class, id, length(2), payload
void ubxChecksum(const uint8_t *payload, uint16_t length, uint8_t &ckA, uint8_t &ckB) {
  ckA = 0;
  ckB = 0;
  for (uint16_t i = 0; i < length; i++) {
    ckA = ckA + payload[i];
    ckB = ckB + ckA;
  }
}

// Send a UBX packet (class, id, payload, length)
void sendUBX(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t length) {
  uint8_t header[2] = {0xB5, 0x62};
  gpsSerial.write(header, 2);
  gpsSerial.write(cls);
  gpsSerial.write(id);
  gpsSerial.write((uint8_t)(length & 0xFF));
  gpsSerial.write((uint8_t)((length >> 8) & 0xFF));
  if (length > 0 && payload != nullptr) {
    gpsSerial.write(payload, length);
  }
  uint8_t ckA, ckB;
  // checksum is computed over cls, id, length(2), payload
  uint8_t chkBuf[2 + 2 + 256]; // enough for small payloads
  uint16_t idx = 0;
  chkBuf[idx++] = cls;
  chkBuf[idx++] = id;
  chkBuf[idx++] = (uint8_t)(length & 0xFF);
  chkBuf[idx++] = (uint8_t)((length >> 8) & 0xFF);
  for (uint16_t i = 0; i < length; i++) chkBuf[idx++] = payload[i];
  ubxChecksum(chkBuf, idx, ckA, ckB);
  gpsSerial.write(ckA);
  gpsSerial.write(ckB);
}

// Configure UART1 baudrate on u-blox via CFG-PRT (8N1, UBX+NMEA)
void setUbxUartBaud(uint32_t baud) {
  // Build CFG-PRT payload for UART1
  // Structure (20 bytes):
  // portID(1)=1, reserved0(1)=0, txReady(2)=0,
  // mode(4)=0x000008D0 (8N1), baudrate(4)=LE,
  // inProtoMask(2)=0x0003 (UBX+NMEA), outProtoMask(2)=0x0003,
  // flags(2)=0, reserved5(2)=0
  uint8_t p[20];
  p[0] = 0x01; // UART1
  p[1] = 0x00; // reserved0
  p[2] = 0x00; p[3] = 0x00; // txReady
  // mode 0x000008D0 little-endian
  p[4] = 0xD0; p[5] = 0x08; p[6] = 0x00; p[7] = 0x00;
  // baudrate little-endian
  p[8]  = (uint8_t)(baud & 0xFF);
  p[9]  = (uint8_t)((baud >> 8) & 0xFF);
  p[10] = (uint8_t)((baud >> 16) & 0xFF);
  p[11] = (uint8_t)((baud >> 24) & 0xFF);
  // inProtoMask = UBX(0x01) | NMEA(0x02) = 0x0003
  p[12] = 0x03; p[13] = 0x00;
  // outProtoMask = UBX | NMEA
  p[14] = 0x03; p[15] = 0x00;
  // flags
  p[16] = 0x00; p[17] = 0x00;
  // reserved5
  p[18] = 0x00; p[19] = 0x00;
  sendUBX(0x06, 0x00, p, 20); // CFG-PRT
}

// Set measurement rate (CFG-RATE), measRateMs: 1000 = 1 Hz, 200 = 5 Hz, 100 = 10 Hz
void setUbxMeasurementRate(uint16_t measRateMs) {
  // Payload: measRate(2), navRate(2)=1, timeRef(2)=1 (GPS time)
  uint8_t payload[6];
  payload[0] = (uint8_t)(measRateMs & 0xFF);
  payload[1] = (uint8_t)((measRateMs >> 8) & 0xFF);
  payload[2] = 0x01; // navRate LSB
  payload[3] = 0x00; // navRate MSB
  payload[4] = 0x01; // timeRef LSB (1 = GPS time)
  payload[5] = 0x00; // timeRef MSB
  sendUBX(0x06, 0x08, payload, 6); // CFG-RATE
}

// Save current configuration to BBR and/or Flash (UBX-CFG-CFG)
// saveMask selects which config blocks to save. We pick ioPort (bit0, CFG-PRT) and navConf (bit2, CFG-RATE)
void saveUbxConfig(bool toBBR, bool toFlash) {
  uint32_t clearMask = 0x00000000; // don't clear
  uint32_t saveMask  = 0x00000000;
  // bit0: ioPort (includes CFG-PRT), bit2: navConf (includes CFG-RATE)
  saveMask |= (1UL << 0); // ioPort
  saveMask |= (1UL << 2); // navConf
  uint32_t loadMask  = 0x00000000; // not used here
  uint8_t deviceMask = 0x00;
  if (toBBR)  deviceMask |= 0x01; // save to BBR
  if (toFlash) deviceMask |= 0x02; // save to Flash (if supported)

  uint8_t payload[13];
  // clearMask LE
  payload[0] = (uint8_t)(clearMask & 0xFF);
  payload[1] = (uint8_t)((clearMask >> 8) & 0xFF);
  payload[2] = (uint8_t)((clearMask >> 16) & 0xFF);
  payload[3] = (uint8_t)((clearMask >> 24) & 0xFF);
  // saveMask LE
  payload[4] = (uint8_t)(saveMask & 0xFF);
  payload[5] = (uint8_t)((saveMask >> 8) & 0xFF);
  payload[6] = (uint8_t)((saveMask >> 16) & 0xFF);
  payload[7] = (uint8_t)((saveMask >> 24) & 0xFF);
  // loadMask LE
  payload[8]  = (uint8_t)(loadMask & 0xFF);
  payload[9]  = (uint8_t)((loadMask >> 8) & 0xFF);
  payload[10] = (uint8_t)((loadMask >> 16) & 0xFF);
  payload[11] = (uint8_t)((loadMask >> 24) & 0xFF);
  // deviceMask
  payload[12] = deviceMask;

  sendUBX(0x06, 0x09, payload, 13); // CFG-CFG
}

/**
 * @brief Konversi sudut (derajat) ke duty value untuk servo
 * 
 * @param angle Sudut dalam derajat (47.5° hingga 132.9°)
 * @return uint32_t Duty value (205-410 untuk 12-bit resolution)
 * 
 * @details
 * Function ini mengkonversi sudut servo ke duty value yang digunakan oleh LEDC.
 * Formula: duty = REFERENCE_DUTY + (angle - REFERENCE_ANGLE) * DUTY_PER_DEGREE
 * 
 * @note
 * - Sudut akan di-clamp ke ANGLE_MIN/ANGLE_MAX jika di luar range
 * - Duty value akan di-clamp ke SERVO_DUTY_MIN/SERVO_DUTY_MAX
 */
uint32_t angleToDuty(float angle) {
  // Validate angle range - clamp ke batas minimum/maksimum
  if (angle < ANGLE_MIN) {
    angle = ANGLE_MIN;
  } else if (angle > ANGLE_MAX) {
    angle = ANGLE_MAX;
  }
  
  // Convert angle to duty: duty = reference_duty + (angle - reference_angle) * duty_per_degree
  uint32_t calculated_duty = REFERENCE_DUTY + (angle - REFERENCE_ANGLE) * DUTY_PER_DEGREE;
  
  // Ensure duty is within limits - double check untuk safety
  if (calculated_duty < SERVO_DUTY_MIN) calculated_duty = SERVO_DUTY_MIN;
  if (calculated_duty > SERVO_DUTY_MAX) calculated_duty = SERVO_DUTY_MAX;
  
  return calculated_duty;
}

/**
 * @brief Konversi pulse width (microseconds) ke duty cycle untuk LEDC
 * 
 * @param microseconds Pulse width dalam microseconds (1000-2000 µs)
 * @return uint32_t Duty cycle value (205-410 untuk 12-bit, 50 Hz)
 * 
 * @details
 * Formula: duty = round((microseconds / period) * max_duty)
 * Period = 20000 µs (50 Hz)
 * Max duty = 4095 (12-bit)
 * 
 * Verifikasi:
 * - 1000 µs → duty = 205 → pulse = 1000.61 µs (error: +0.61 µs)
 * - 1500 µs → duty = 307 → pulse = 1499.39 µs (error: -0.61 µs)
 * - 2000 µs → duty = 410 → pulse = 2000.49 µs (error: +0.49 µs)
 * 
 * Error maksimal: ±0.61 µs (sangat kecil, acceptable untuk servo)
 */
uint32_t microsecondsToDuty(uint16_t microseconds) {
  // Clamp input ke range valid
  if (microseconds < 1000) microseconds = 1000;
  if (microseconds > 2000) microseconds = 2000;
  
  // Konversi dengan rounding untuk akurasi lebih baik
  // Formula: duty = round((microseconds / 20000) * 4095)
  // Optimized: duty = round(microseconds * 4095 / 20000)
  // Untuk integer rounding: (x + half) / divisor
  uint32_t duty = ((uint32_t)microseconds * 4095 + 10000) / 20000;  // +10000 untuk rounding (half of 20000)
  
  // Clamp hasil ke range valid (double check)
  if (duty < 205) duty = 205;  // Min duty untuk 1000 µs
  if (duty > 410) duty = 410;  // Max duty untuk 2000 µs
  
  return duty;
}

/**
 * @brief Mode Manual - Kontrol rudder langsung dari receiver RC
 * 
 * @details
 * Function ini mengontrol rudder secara langsung berdasarkan input dari channel 1 RC.
 * Range kontrol: -40° hingga +40° dari posisi neutral (90°).
 */
void rudder_manual() {
  // ========== Map RC Input ke Servo Offset ==========
  // Map rudder (1000..1992) ke servo_angle_current_offset (-40..40 derajat)
  servo_angle_current_offset = mapFloat(controlInput.rudder, 1000, 1992, -40.0f, 40.0f);
  servo_angle_current_offset = constrain(servo_angle_current_offset, -40.0f, 40.0f);

  // ========== Calculate dan Set Servo Position ==========
  float calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
  servo_duty = angleToDuty(calculated_angle);
  bool write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
}

/**
 * @brief Setup function - Inisialisasi semua hardware dan komunikasi
 * 
 * @details
 * Function ini melakukan inisialisasi:
 * 1. Serial Monitor (115200 baud)
 * 2. PPM interrupt handler untuk receiver RC
 * 3. ADC configuration untuk feedback servo
 * 4. Servo PWM configuration (rudder, propeller speed/direction)
 * 5. GNSS module (u-blox GPS) via Serial1
 * 6. IMU (HWT905TTL) via Serial2
 * 7. ESP-NOW untuk wireless communication
 * 
 * @note
 * - Semua interrupt handler harus dideklarasikan dengan IRAM_ATTR
 * - ADC2 tidak dapat digunakan saat WiFi aktif (gunakan ADC1 saja)
 * - GNSS akan dikonfigurasi ke 115200 baud dan 10 Hz update rate
 */
void setup() {
    // ========== Serial Monitor Initialization ==========
    Serial.begin(115200);
    delay(500);
    
    // ========== PPM Interrupt Setup ==========
    pinMode(PPM_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PPM_PIN), ppm_interrupt_handler, CHANGE);
    
    // Configure ADC settings
    analogReadResolution(12);     
    

    bool attach_PWM_pin = ledcAttachChannel(SERVO_RUDDER_pin, SERVO_RUDDER_freq, SERVO_RUDDER_resolution, 0);
    if (attach_PWM_pin) {
      Serial.println("Servo Rudder attached to pin 5");
    } else {
      Serial.println("Failed to attach servo rudder");
    }

    // Setup LEDC untuk Propeller Speed
    bool attach_prop_speed = ledcAttachChannel(PROP_SPEED_pin, PROP_SPEED_freq, PROP_SPEED_resolution, 2);
    if (attach_prop_speed) {
      Serial.println("Propeller Speed attached to pin 6 (LEDC)");
    } else {
      Serial.println("Failed to attach propeller speed");
    }

    // Setup LEDC untuk Propeller Direction
    bool attach_prop_dir = ledcAttachChannel(PROP_DIRECTION_pin, PROP_DIRECTION_freq, PROP_DIRECTION_resolution, 1);
    if (attach_prop_dir) {
      Serial.println("Propeller Direction attached to pin 7 (LEDC)");
    } else {
      Serial.println("Failed to attach propeller direction");
    }
    // Note: servoPropSpeed dan servoPropDirection tidak digunakan lagi, sudah diganti dengan LEDC

    delay(1000);
    Serial.println("PPM Reader initialized - Reading FS-iA6B on pin 4");

      // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD1, TXD1);
    Serial.println("Serial 2 started at 9600 baud rate");

    // 1) Set u-blox UART1 baud to 115200 at current link speed (9600)
    delay(200);
    setUbxUartBaud(115200);
    delay(200);
    // 2) Switch ESP32 UART to 115200 to match the module
  #if ARDUINO_USB_CDC_ON_BOOT
    // ensure buffered data is flushed before changing baud
  #endif
    gpsSerial.flush();
    // Prefer updateBaudRate if available; fallback to end/begin
  #if defined(HW_SERIAL_HAS_UPDATEBAUDRATE)
    gpsSerial.updateBaudRate(115200);
  #else
    gpsSerial.end();
    delay(50);
    gpsSerial.begin(115200, SERIAL_8N1, RXD1, TXD1);
  #endif
    Serial.println("GPS UART switched to 115200");

    // 3) Now set 10 Hz update rate at the new baud
    delay(200);
    setUbxMeasurementRate(100); // 100 ms = 10 Hz

    // 4) Persist configuration to BBR and Flash (if supported)
    delay(200);
    saveUbxConfig(true, true);

    // setting for hwt905ttl serial port
    HWT905TTL_Serial.begin(HWT905TTL_BAUD, SERIAL_8N1, RXD2, TXD2);

        // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    // esp_now_register_send_cb(OnDataSent);
    
    // Register peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }
}

/**
 * @brief Loop function - Main program loop
 * 
 * @details
 * Function ini berjalan terus menerus dan melakukan:
 * 1. Membaca data dari GNSS module (Serial1)
 * 2. Membaca data dari IMU (Serial2)
 * 3. Setiap 100ms (10 Hz):
 *    - Update data GPS (latitude, longitude)
 *    - Update data IMU (yaw, accelerometer)
 *    - Map PPM values ke range 1000-2000
 *    - Kontrol rudder manual dari RC
 *    - Kontrol motor propeller (speed dan direction)
 *    - Kirim data via ESP-NOW
 * 
 * @note
 * - Update rate: 10 Hz (setiap 100ms)
 * - Semua data dikirim dalam format fixed-point (× 100)
 * - Struktur data harus sama dengan receiver
 */
void loop() {
    unsigned long currentMillis = millis();
    
    // ========== Read GNSS Data ==========
    // Parse data GPS dari Serial1 (non-blocking)
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }

    // ========== Read IMU Data ==========
    // Parse data IMU dari Serial2 (non-blocking)
    while (HWT905TTL_Serial.available()) 
    {
      JY901.CopeSerialData(HWT905TTL_Serial.read()); // Call JY901 data cope function
    }

    // ========== Main Loop - Run setiap intervaltime ms (dinamis: 10ms atau 100ms) ==========
    if (currentMillis - previousMillis >= intervaltime) {
        // Serial.print(currentMillis);
        // Serial.print(" ");
        previousMillis = currentMillis;
        dataToSend.timestamp = currentMillis;  // Timestamp dalam milidetik

        // Snapshot to struct every interval, regardless of isUpdated()
        latestGpsData.latitude = gps.location.lat();
        latestGpsData.longitude = gps.location.lng();

        // latestGpsData.locationValid = gps.location.isValid();
        // latestGpsData.timeValid = gps.time.isValid();
        // latestGpsData.locationAgeMs = gps.location.age();
        // latestGpsData.timeAgeMs = gps.time.age();
        // latestGpsData.altitudeM = gps.altitude.meters();
        // latestGpsData.hdop = gps.hdop.value() / 100.0;
        // latestGpsData.satellites = gps.satellites.value();
        // if (latestGpsData.locationValid) {
        // }
        // Konversi ke int32_t fixed-point (× 10,000,000 untuk presisi 7 digit)
        dataToSend.latitude = (int32_t)(latestGpsData.latitude * 10000000);
        dataToSend.longitude = (int32_t)(latestGpsData.longitude * 10000000);

        // Kirim data raw LSB langsung (konversi dilakukan di receiver)
        dataToSend.roll_raw = JY901.stcAngle.Angle[0];
        dataToSend.pitch_raw = JY901.stcAngle.Angle[1];
        dataToSend.yaw_raw = JY901.stcAngle.Angle[2];
        dataToSend.accel_x_raw = JY901.stcAcc.a[0];
        dataToSend.accel_y_raw = JY901.stcAcc.a[1];
        dataToSend.accel_z_raw = JY901.stcAcc.a[2];
        dataToSend.gyro_x_raw = JY901.stcGyro.w[0];
        dataToSend.gyro_y_raw = JY901.stcGyro.w[1];
        dataToSend.gyro_z_raw = JY901.stcGyro.w[2];
        dataToSend.mag_x_raw = JY901.stcMag.h[0];
        dataToSend.mag_y_raw = JY901.stcMag.h[1];
        dataToSend.mag_z_raw = JY901.stcMag.h[2];
        dataToSend.quat0_raw = JY901.stcQuater.sQuat0;
        dataToSend.quat1_raw = JY901.stcQuater.sQuat1;
        dataToSend.quat2_raw = JY901.stcQuater.sQuat2;
        dataToSend.quat3_raw = JY901.stcQuater.sQuat3;
        
        for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
            // Convert raw PPM FS-iA6B values (600-1600) to standard servo range (1000-2000)
            ppm_mapped[i] = map(ppm_values[i], PPM_MIN_CHANNEL_VALUE, PPM_MAX_CHANNEL_VALUE, 1000, 2000);
        }
        
        /*
        controlInput.rudder = ppm_mapped[0]; CH1 tuas kanan bergerak kanan kiri
        controlInput.propSpeed = ppm_mapped[2];   // CH3 CH2 tuas kiri bergerak atas bawah
        controlInput.propDirection = ppm_mapped[4]; // CH5 Switch A bergerak atas bawah
        
        */
        // Kontrol rudder manual
        controlInput.rudder = ppm_mapped[0];              // CH1
        rudder_manual();

        // Set PWM for PropSpeed dan PropDirection
        controlInput.propSpeed = ppm_mapped[2];           // CH3
        controlInput.propDirection = ppm_mapped[4];       // CH5
        
        // Set interval time mode berdasarkan CH6
        controlInput.intervaltime_mode = ppm_mapped[5];   // CH6
        if (controlInput.intervaltime_mode >= 1750) {
            intervaltime = 10;   // High speed mode: 10ms (100 Hz)
        } else {
            intervaltime = 100;  // Normal mode: 100ms (10 Hz)
        }
        
        // Generate PWM for PropSpeed (menggunakan LEDC)
        // Konversi microseconds ke duty cycle menggunakan fungsi yang sama
        uint32_t prop_speed_duty = microsecondsToDuty(controlInput.propSpeed);
        // Generate PWM menggunakan LEDC
        bool write_result_prop_speed = ledcWrite(PROP_SPEED_pin, prop_speed_duty);
        
        // Generate PWM for PropDirection (menggunakan LEDC)
        // Konversi microseconds ke duty cycle menggunakan fungsi yang sama
        uint32_t prop_dir_duty = microsecondsToDuty(controlInput.propDirection);
        // Generate PWM menggunakan LEDC
        bool write_result_prop_dir = ledcWrite(PROP_DIRECTION_pin, prop_dir_duty); 

        // ========== Read Battery Voltage ==========
        // Baca ADC dalam miliVolt dari GPIO1 (ADC1_CH0)
        uint32_t adc_millivolts_batt_1 = analogReadMilliVolts(ADC_PIN_BATT_1);
        // Konversi mV ke V, lalu kalikan dengan faktor skala 5 (voltage divider)
        float volt_batt_1 = (adc_millivolts_batt_1 / 1000.0) * 5.0;
        // Konversi ke uint16_t (× 100) untuk pengiriman
        dataToSend.battery_1 = (uint16_t)(volt_batt_1 * 100); // batere for ESP32-S3, Servo, HWT905TTL, Receiver RC, GNSS
        
        // Baca ADC dalam miliVolt dari GPIO2 (ADC1_CH1)
        uint32_t adc_millivolts_batt_2 = analogReadMilliVolts(ADC_PIN_BATT_2);
        // Konversi mV ke V, lalu kalikan dengan faktor skala 5 (voltage divider)
        float volt_batt_2 = (adc_millivolts_batt_2 / 1000.0) * 5.0;
        // Konversi ke uint16_t (× 100) untuk pengiriman
        dataToSend.battery_2 = (uint16_t)(volt_batt_2 * 100); // batere for motor propeller

        //Print CSV: timestamp,lat,lon,yaw
        // Konversi fixed-point kembali ke float untuk display (÷ 100)
        // 
        // Serial.print(dataToSend.timestamp); Serial.println("");  // Timestamp dalam ms (uint32_t)
        // Serial.print(dataToSend.latitude / 10000000.0, 7); Serial.println(""); // Konversi dari fixed-point ke float
        // Serial.print(dataToSend.longitude / 10000000.0, 7); Serial.print(","); // Konversi dari fixed-point ke float
        // Serial.print(dataToSend.yaw / 100.0, 2); Serial.print(",");

        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &dataToSend, sizeof(dataToSend));
        
        if (result == ESP_OK) {
          //Serial.println("Sent with success");
        }
        else {
          //Serial.println("Error sending the data");
        }

        // Serial.println(millis()/1000.000,3);
    } // end of if interval
}

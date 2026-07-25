/**
 * @file main.cpp
 * @brief ESP32-S3 Remote-Side-01 — Ship Model Control System
 *
 * @description
 * Firmware sisi kapal (Remote-Side) yang mengumpulkan data sensor dan
 * actuator, menjalankan kontrol rudder/propeller, lalu mengirim telemetry
 * 15-kolom ke User-Side via ESP-NOW.
 *
 * Clone dari ESP_Now_Send_Ver2025_revJan2026.
 *
 * Hardware yang digunakan:
 * - Receiver RC (FS-iA6B) dengan output PPM
 * - Servo rudder dengan feedback potensiometer
 * - Motor propeller dengan rotary encoder (RPM measurement)
 * - GNSS module (u-blox GPS) via Serial1
 * - IMU (HWT905TTL) via Serial2
 * - ADC untuk monitoring baterai
 *
 * Fitur kontrol:
 * - Mode Manual: Kontrol rudder langsung dari RC
 * - Mode Auto - Turning Left/Right: Rudder bergerak bertahap
 * - Mode Auto - Zigzag 10°/20°: Zigzag dengan sudut tertentu
 *
 * @author Chandra P - Ship Model Control System
 * @version 1.0 (Remote-Side-01)
 * @date 2026
 *
 * @note
 * - Update rate: 10 Hz (setiap 100ms)
 * - Data dikirim dalam format fixed-point (× 100) untuk efisiensi
 * - Struktur data harus sama dengan ESP-Now_ESP32-S3_User-Side
 * - PENDING: penerimaan waypoint dari User-Side (belum di-port)
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
 * @brief MAC Address ESP32-S3 User-Side (peer ESP-NOW)
 *
 * @note UBAH MAC ADDRESS INI sesuai MAC User-Side Anda.
 * Penerimaan waypoint dari User-Side: PENDING (belum diimplementasi).
 */
// uint8_t user_side_Address[] = {0x10, 0x20, 0xba, 0x4c, 0x53, 0xfc};
uint8_t user_side_Address[] = {0x94, 0xa9, 0x90, 0x30, 0xab, 0xc0};

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
// Rotary Encoder Configuration (RPM Measurement)
// ============================================================================

/**
 * @brief Konfigurasi rotary encoder untuk mengukur RPM motor propeller
 * 
 * @details
 * Rotary encoder menghasilkan pulse setiap kali motor berputar.
 * RPM dihitung dengan menghitung jumlah pulse per interval waktu.
 * Menggunakan moving average untuk smoothing data.
 */
#define PULSE_PIN_Motor_prop_1 9     ///< GPIO pin untuk encoder motor propeller 1
#define PULSE_PIN_Motor_prop_2 10    ///< GPIO pin untuk encoder motor propeller 2
volatile uint32_t pulse_counter_motor_prop_1 = 0;  ///< Counter kumulatif untuk motor 1
volatile uint32_t pulse_counter_motor_prop_2 = 0;  ///< Counter kumulatif untuk motor 2

/**
 * @brief Moving Average Configuration untuk smoothing RPM data
 * 
 * @details
 * Moving average digunakan untuk mengurangi noise pada pengukuran RPM.
 * Buffer menyimpan N sampel terakhir, kemudian dihitung rata-ratanya.
 */
#define RPM_SAMPLES_motor_prop_1 10  ///< Jumlah sampel untuk moving average motor 1
#define RPM_SAMPLES_motor_prop_2 10  ///< Jumlah sampel untuk moving average motor 2
uint32_t pulse_buffer_motor_prop_1[RPM_SAMPLES_motor_prop_1] = {0};  ///< Buffer untuk motor 1
uint32_t pulse_buffer_motor_prop_2[RPM_SAMPLES_motor_prop_2] = {0};  ///< Buffer untuk motor 2
uint8_t buffer_index_motor_prop_1 = 0;  ///< Index saat ini dalam buffer motor 1 (circular buffer)
uint8_t buffer_index_motor_prop_2 = 0;  ///< Index saat ini dalam buffer motor 2 (circular buffer)

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
const long intervaltime = 100;       ///< Interval waktu dalam ms (100ms = 10 Hz)

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
    uint16_t rudder;           ///< Channel 1: Kontrol rudder / Turning mode
    uint16_t propSpeed;        ///< Channel 3: Kecepatan motor propeller
    uint16_t propDirection;    ///< Channel 5: Arah motor propeller
    uint16_t mode_auto_manual; ///< Channel 6: Mode auto/manual (≥1750: Auto, <1750: Manual)
    uint16_t mode_zigzag;      ///< Channel 2: Mode zigzag (≤1250: Zigzag 10°, ≥1750: Zigzag 20°)
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

/**
 * @brief Flag untuk reset step turning/zigzag functions
 * 
 * @details
 * Flag ini digunakan untuk mereset state machine ketika beralih mode.
 * Set ke true ketika masuk mode manual atau beralih mode auto.
 */
bool g_reset_turning_left = false;   ///< Flag reset untuk turning left
bool g_reset_turning_right = false; ///< Flag reset untuk turning right
bool g_reset_zigzag_left = false;   ///< Flag reset untuk zigzag left
bool g_reset_zigzag_right = false;  ///< Flag reset untuk zigzag right

/**
 * @brief Variable kontrol interval step rudder
 * 
 * @details
 * Menentukan berapa iterasi yang diperlukan sebelum step rudder di-update.
 * Default: 2 iterasi per step (setiap 200ms @ 100ms interval).
 * 
 * @note
 * Nilai lebih besar = rudder bergerak lebih lambat
 * Nilai lebih kecil = rudder bergerak lebih cepat
 */
uint8_t rudder_step_interval = 2;  ///< Interval iterasi per step (default: 2)

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

// ADC Pin Configuration (for feedback reading)
// Jangan gunakan grup ADC2 jika akan menggunakan fitur WIFI ESP32-S3
uint8_t ADC_PIN_SERVO_1 = 8;  // GPIO8 = ADC1_7 pin for analog input (ESP32-S3)
uint8_t ADC_PIN_SERVO_2 = 3;  // GPIO3 = ADC1_2 pin for analog input (ESP32-S3)
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
  double speedMps;
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
  double timestamp;       // detik sejak boot (millis()/1000.0)
  double latitude;        // dari latestGpsData.latitude
  double longitude;       // dari latestGpsData.longitude
  uint16_t speedMps;      // dari latestGpsData.speedMps (× 100, range 0-655.35 m/s)
  int16_t Calc_deg_servo_1; // derajat hasil kalkulasi feedback servo 1 (× 100)
  int16_t Calc_deg_servo_2; // derajat hasil kalkulasi feedback servo 2 (× 100)
  int16_t roll;           // derajat roll (× 100)
  int16_t pitch;          // derajat pitch (× 100)
  uint16_t yaw;           // derajat yaw (× 100, 0-360°)
  //*****settingan state untuk zigzag********//
  int16_t zigzag_yaw;     // zigzag yaw (× 100)
  uint16_t rpm_prop_1;    // rpm motor propeller 1 (× 100)
  uint16_t rpm_prop_2;    // rpm motor propeller 2 (× 100)
  uint16_t battery_1;     // batere for ESP32-S3, Servo, HWT905TTL, Receiver RC, GNSS, Rotary Encoder (× 100)
  uint16_t battery_2;     // batere for motor propeller (× 100)
  uint8_t mode_auto;      // 0: manual, 1: turning left, 2: turning right, 3: zigzag 10, 4: zigzag 20
};

DatatoSend dataToSend;

  //*****settingan state untuk zigzag********//
  float zigzag_setpoint;
/*
Identify which states are Transitional (T) and
which are Static (S). A
Static state is one in which the FSM is waiting for stimulus, and is
taking no actions.
A Transitional State is a state which causes actions, but doesn't look
for stimulus.
Use names for states so it's easier to understand what the code is doing */

const int belok_kanan_1a = 0;   // State 0: belok_kanan_1a rudder belok 20 derajat ke kanan (T)
const int belok_kanan_1b = 1;   // State 1: belok_kanan_1b tunggu heading >= 20 derajat (S)
const int belok_kiri_1a = 2;    // State 2: belok_kiri_1a rudder belok 20 derajat ke kiri  (T)
const int belok_kiri_1b = 3;    // State 3: belok_kiri_1b tunggu heading <= -20 derajat (S)
const int belok_kanan_2a = 4;   // State 4: belok_kanan_2a rudder belok 20 derajat ke kanan (T)
const int belok_kanan_2b = 5;   // State 5: belok_kanan_2b tunggu heading >= 20 derajat (S)
const int belok_kiri_2a = 6;    // State 6: belok_kiri_2a rudder belok 20 derajat ke kiri  (T)
const int belok_kiri_2b = 7;    // State 6: belok_kiri_2b tunggu heading <= -20 derajat (S)
const int belok_kanan_3a = 8;  // State 7: belok_kanan_3a rudder belok 20 derajat ke kanan (T)
const int belok_kanan_3b = 9;  // State 8: belok_kanan_3b tunggu heading >= 20 derajat (S)
const int lurus_akhir = 10;     // State 10: lurus_akhir rudder lurus (T)

static int state; 

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

/**
 * @brief Interrupt handler untuk rotary encoder motor propeller 1
 * 
 * @details
 * Function ini dipanggil setiap kali terjadi rising edge pada PULSE_PIN_Motor_prop_1.
 * Increment counter untuk menghitung jumlah pulse (putaran motor).
 * 
 * @note
 * Function ini harus dideklarasikan dengan IRAM_ATTR karena dipanggil dari interrupt.
 * Counter harus volatile karena diakses dari interrupt dan main loop.
 */
void IRAM_ATTR pulse_interrupt_handler_motor_prop_1() {
  pulse_counter_motor_prop_1++;  // Increment counter pada rising edge
}

/**
 * @brief Interrupt handler untuk rotary encoder motor propeller 2
 * 
 * @details
 * Function ini dipanggil setiap kali terjadi rising edge pada PULSE_PIN_Motor_prop_2.
 * Increment counter untuk menghitung jumlah pulse (putaran motor).
 * 
 * @note
 * Function ini harus dideklarasikan dengan IRAM_ATTR karena dipanggil dari interrupt.
 * Counter harus volatile karena diakses dari interrupt dan main loop.
 */
void IRAM_ATTR pulse_interrupt_handler_motor_prop_2() {
  pulse_counter_motor_prop_2++;  // Increment counter pada rising edge
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
 * 
 * @note
 * - Reset semua flag turning/zigzag ketika masuk mode manual
 * - Zigzag setpoint di-update dari yaw saat ini
 * - Zigzag yaw di-set ke 0 karena tidak ada zigzag di mode manual
 */
void rudder_manual() {
  // ========== Reset State Machine ==========
  // Reset semua flag turning/zigzag ketika masuk mode manual
  g_reset_turning_left = true;
  g_reset_turning_right = true;
  g_reset_zigzag_left = true;
  g_reset_zigzag_right = true;
  state = belok_kanan_1a;  // Reset state ke awal
  
  // ========== Map RC Input ke Servo Offset ==========
  // Map rudder (1000..1992) ke servo_angle_current_offset (-40..40 derajat)
  servo_angle_current_offset = mapFloat(controlInput.rudder, 1000, 1992, -40.0f, 40.0f);
  servo_angle_current_offset = constrain(servo_angle_current_offset, -40.0f, 40.0f);

  // ========== Calculate dan Set Servo Position ==========
  float calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
  servo_duty = angleToDuty(calculated_angle);
  bool write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);

  // ========== Update Zigzag Setpoint ==========
  // Set zigzag setpoint dari yaw saat ini (untuk digunakan saat masuk mode zigzag)
  // dataToSend.yaw dalam uint16_t (× 100), jadi bagi 100 dulu
  zigzag_setpoint = dataToSend.yaw / 100.0;
  // zigzag_yaw = 0 saat manual (tidak ada zigzag)
  dataToSend.zigzag_yaw = (int16_t)(((dataToSend.yaw / 100.0) - zigzag_setpoint ) * 100);
}

void rudder_turning_right() {
  static float current_step = 0.0f;
  static uint8_t iteration_counter = 0;  // Counter untuk menghitung iterasi
  
  // Check if reset was requested
  if (g_reset_turning_left) {
    current_step = 0.0f;
    iteration_counter = 0;  // Reset counter juga
    g_reset_turning_left = false;
  }
  
  // Set offset dengan step saat ini
  servo_angle_current_offset = current_step;
  
  // Increment counter
  iteration_counter++;
  
  // Hanya update step jika counter mencapai interval
  if (iteration_counter >= rudder_step_interval) {
    if (current_step < 35.0f) {
      current_step += 5.0f;
      if (current_step > 35.0f) {
        current_step = 35.0f;
      }
    }
    iteration_counter = 0;  // Reset counter
  }
  
  float calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

  // Set duty cycle to be write for servo rudder degree move
  servo_duty = angleToDuty(calculated_angle);
  bool write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);

  // Set zigzag setpoint dan hitung zigzag_yaw
  zigzag_setpoint = dataToSend.yaw / 100.0;
  dataToSend.zigzag_yaw = (int16_t)(((dataToSend.yaw / 100.0) - zigzag_setpoint) * 100);  // tidak ada zigzag saat turning
}

void rudder_turning_left() {
  static float current_step = 0.0f;
  static uint8_t iteration_counter = 0;  // Counter untuk menghitung iterasi
  
  // Check if reset was requested
  if (g_reset_turning_right) {
    current_step = 0.0f;
    iteration_counter = 0;  // Reset counter juga
    g_reset_turning_right = false;
  }
  
  // Set offset dengan step saat ini
  servo_angle_current_offset = current_step;
  
  // Increment counter
  iteration_counter++;
  
  // Hanya update step jika counter mencapai interval
  if (iteration_counter >= rudder_step_interval) {
    if (current_step > -35.0f) {
      current_step -= 5.0f;
      if (current_step < -35.0f) {
        current_step = -35.0f;
      }
    }
    iteration_counter = 0;  // Reset counter
  }
  
  float calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
  // Serial.println(calculated_angle);

  // Set duty cycle to be write for servo rudder degree move
  servo_duty = angleToDuty(calculated_angle);
  bool write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);

  // Set zigzag setpoint dan hitung zigzag_yaw
  zigzag_setpoint = dataToSend.yaw / 100.0;
  dataToSend.zigzag_yaw = (int16_t)(((dataToSend.yaw / 100.0) - zigzag_setpoint) * 100);  // tidak ada zigzag saat turning
}

void rudder_zigzag_10() {
 
  // Hitung zigzag_yaw: yaw_current - zigzag_setpoint (dalam derajat float)
  float yaw_float = dataToSend.yaw / 100.0;  // Konversi uint16_t ke float
  float zigzag_yaw_float = yaw_float - zigzag_setpoint;
  dataToSend.zigzag_yaw = (int16_t)(zigzag_yaw_float * 100);  // Konversi ke int16_t (× 100)
  // Serial.print(dataToSend.zigzag_yaw); Serial.print(",");
  // Serial.print(zigzag_setpoint); Serial.print(",");
  float calculated_angle;
  bool write_result;
  static float current_step = 0.0f;
  static uint8_t iteration_counter = 0;  // Counter untuk menghitung iterasi
  // Serial.print(current_step); 
  // Serial.print(",");
  // Serial.print(state);
  // Serial.print(",");

  switch (state)
  {
    
    case belok_kanan_1a: //rudder belok kanan 10 derajat
      if (g_reset_zigzag_right) {
        current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_right = false;
      }
      
     
      // Increment counter
      iteration_counter++;
      Serial.print(iteration_counter);
      Serial.print(",");
      
      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step < 10.0f) {
          current_step += 1.0f;
          if (current_step >= 10.0f) {
            current_step = 10.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }
      
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
      // Serial.print(calculated_angle); Serial.print(",");
    
      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      // Serial.println(servo_duty); 
      //Serial.print(",");  
      
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step >= 10.0f) {
        state = belok_kanan_1b;
      }
    
    break;

    /*************************************************/
    
    case belok_kanan_1b:
      // Serial.println("DEBUG_Zigzag: belok_kanan_1b");
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
      // Serial.print(calculated_angle);
      // Serial.print(",");

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      // Serial.println(servo_duty);
      
      if (dataToSend.zigzag_yaw <= -1000) //menunggu heading zigzag lebih dari 10 derajat (× 100 = -1000)
      {
      state = belok_kiri_1a;
      g_reset_zigzag_right = true;
      } 
     break;

    /*************************************************/
    
    case belok_kiri_1a: //rudder belok kiri 10 derajat
    // Serial.print("belok_kiri_1a"); Serial.print(",");

      if (g_reset_zigzag_left) {
        // current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_left = false;
      }
          
      // Increment counter
      iteration_counter++;
      
      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step > -10.0f) {  // Increment selama belum mencapai target 10
          current_step -= 1.0f;
          if (current_step <= -10.0f) {
            current_step = -10.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }
      
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
    
      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
        
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step <= -10.0f) {
        state = belok_kiri_1b;
      }

    break;

    /*************************************************/
    
    case belok_kiri_1b:
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw >= 1000) //menunggu heading zigzag lebih dari 10 derajat (× 100 = 1000)
      {
      state = belok_kanan_2a;
      g_reset_zigzag_left = true;
      } 
    break;

     /*************************************************/

    case belok_kanan_2a: //rudder belok kanan 10 derajat
    // Serial.println("DEBUG_Zigzag: belok_kanan_2a");
    // static float current_step = 0.0f;
    // static uint8_t iteration_counter = 0;
    
      // Check if reset was requested
      if (g_reset_zigzag_right) {
        // current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_right = false;
      }



      // Increment counter
      iteration_counter++;

      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step < 10.0f) {
          current_step += 1.0f;
          if (current_step >= 10.0f) {
            current_step = 10.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }

      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step >= 10.0f) {
        state = belok_kanan_2b;
      }
    break;

    /*************************************************/
    
    case belok_kanan_2b:
    // Serial.println("DEBUG_Zigzag: belok_kanan_2b");
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw <= -1000) //menunggu heading zigzag lebih dari 10 derajat (× 100 = -1000)
      {
      state = belok_kiri_2a;
      g_reset_zigzag_right = true;
      }   
    break;

     /*************************************************/

    
    case belok_kiri_2a: //rudder belok kiri 10 derajat
      // Serial.println("DEBUG_Zigzag: belok_kiri_2a");
      // static float current_step = 0.0f;
      // static uint8_t iteration_counter = 0;
      
      // Check if reset was requested
      if (g_reset_zigzag_left) {
        //current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_left = false;
      }
      

      
      // Increment counter
      iteration_counter++;
      
      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step > -10.0f) {
          current_step -= 1.0f;
          if (current_step <= -10.0f) {
            current_step = -10.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }
      
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
    
      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
        
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step <= -10.0f) {
        state = belok_kiri_2b;
      } 
    
    break;

    /*************************************************/

    case belok_kiri_2b:
      // Serial.println("DEBUG_Zigzag: belok_kiri_2b");
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw >= 1000) //menunggu heading zigzag lebih dari 10 derajat (× 100 = 1000)
      {
      state = belok_kanan_3a;
      g_reset_zigzag_left = true;
      }  
    break;

     /*************************************************/
     
    case belok_kanan_3a:
    // Serial.println("DEBUG_Zigzag: belok_kanan_3a");
    // static float current_step = 0.0f;
    // static uint8_t iteration_counter = 0;
    
      // Check if reset was requested
      if (g_reset_zigzag_right) {
        //current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_right = false;
      }


      // Increment counter
      iteration_counter++;

      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step < 10.0f) {
          current_step += 1.0f;
          if (current_step >= 10.0f) {
            current_step = 10.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }

      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step >= 10.0f) {
        state = belok_kanan_3b;
      }
    
    break;

   /*************************************************/
   
    case belok_kanan_3b:
    // Serial.println("DEBUG_Zigzag: belok_kanan_3b");
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw <= -1000) //menunggu heading zigzag lebih dari 10 derajat (× 100 = -1000)
      {
      state = lurus_akhir;
      g_reset_zigzag_right = true;
      }
    break;

     /*************************************************/

    case lurus_akhir:
     g_reset_zigzag_right = true;
     g_reset_zigzag_left = true;

     calculated_angle = REFERENCE_ANGLE;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    break;
     
  }
}

void rudder_zigzag_20() {
 
  // Hitung zigzag_yaw: yaw_current - zigzag_setpoint (dalam derajat float)
  float yaw_float = dataToSend.yaw / 100.0;  // Konversi uint16_t ke float
  float zigzag_yaw_float = yaw_float - zigzag_setpoint;
  dataToSend.zigzag_yaw = (int16_t)(zigzag_yaw_float * 100);  // Konversi ke int16_t (× 100)
  // Serial.print(dataToSend.zigzag_yaw); Serial.print(",");
  // Serial.print(zigzag_setpoint); Serial.print(",");
  float calculated_angle;
  bool write_result;
  static float current_step = 0.0f;
  static uint8_t iteration_counter = 0;  // Counter untuk menghitung iterasi
  Serial.print(current_step); 
  Serial.print(",");
  Serial.print(state);
  Serial.print(",");

  switch (state)
  {
    
    case belok_kanan_1a: //rudder belok kanan 20 derajat
      if (g_reset_zigzag_right) {
        current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_right = false;
      }
      
     
      // Increment counter
      iteration_counter++;
      Serial.print(iteration_counter);
      Serial.print(",");
      
      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step < 20.0f) {
          current_step += 2.0f;
          if (current_step >= 20.0f) {
            current_step = 20.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }
      
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
      // Serial.print(calculated_angle); Serial.print(",");
    
      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      // Serial.println(servo_duty); 
      //Serial.print(",");  
      
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step >= 20.0f) {
        state = belok_kanan_1b;
      }
    
    break;

    /*************************************************/
    
    case belok_kanan_1b:
      // Serial.println("DEBUG_Zigzag: belok_kanan_1b");
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
      // Serial.print(calculated_angle);
      // Serial.print(",");

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      // Serial.println(servo_duty);
      
      if (dataToSend.zigzag_yaw <= -2000) //menunggu heading zigzag lebih dari 20 derajat (× 100 = -2000)
      {
      state = belok_kiri_1a;
      g_reset_zigzag_right = true;
      } 
     break;

    /*************************************************/
    
    case belok_kiri_1a: //rudder belok kiri 20 derajat
    // Serial.print("belok_kiri_1a"); Serial.print(",");

      if (g_reset_zigzag_left) {
        // current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_left = false;
      }
          
      // Increment counter
      iteration_counter++;
      
      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step > -20.0f) {  // Increment selama belum mencapai target 20
          current_step -= 2.0f;
          if (current_step <= -20.0f) {
            current_step = -20.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }
      
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
    
      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
        
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step <= -20.0f) {
        state = belok_kiri_1b;
      }

    break;

    /*************************************************/
    
    case belok_kiri_1b:
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw >= 2000) //menunggu heading zigzag lebih dari 20 derajat (× 100 = 2000)
      {
      state = belok_kanan_2a;
      g_reset_zigzag_left = true;
      } 
    break;

     /*************************************************/

    case belok_kanan_2a: //rudder belok kanan 20 derajat
    // Serial.println("DEBUG_Zigzag: belok_kanan_2a");
    // static float current_step = 0.0f;
    // static uint8_t iteration_counter = 0;
    
      // Check if reset was requested
      if (g_reset_zigzag_right) {
        // current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_right = false;
      }



      // Increment counter
      iteration_counter++;

      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step < 20.0f) {
          current_step += 2.0f;
          if (current_step >= 20.0f) {
            current_step = 20.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }

      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step >= 20.0f) {
        state = belok_kanan_2b;
      }
    break;

    /*************************************************/
    
    case belok_kanan_2b:
    // Serial.println("DEBUG_Zigzag: belok_kanan_2b");
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw <= -2000) //menunggu heading zigzag lebih dari 20 derajat (× 100 = -2000)
      {
      state = belok_kiri_2a;
      g_reset_zigzag_right = true;
      }   
    break;

     /*************************************************/

    
    case belok_kiri_2a: //rudder belok kiri 20 derajat
      // Serial.println("DEBUG_Zigzag: belok_kiri_2a");
      // static float current_step = 0.0f;
      // static uint8_t iteration_counter = 0;
      
      // Check if reset was requested
      if (g_reset_zigzag_left) {
        //current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_left = false;
      }
      

      
      // Increment counter
      iteration_counter++;
      
      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step > -20.0f) {
          current_step -= 2.0f;
          if (current_step <= -20.0f) {
            current_step = -20.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }
      
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;
    
      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
        
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step <= -20.0f) {
        state = belok_kiri_2b;
      } 
    
    break;

    /*************************************************/

    case belok_kiri_2b:
      // Serial.println("DEBUG_Zigzag: belok_kiri_2b");
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw >= 2000) //menunggu heading zigzag lebih dari 20 derajat (× 100 = 2000)
      {
      state = belok_kanan_3a;
      g_reset_zigzag_left = true;
      }  
    break;

     /*************************************************/
     
    case belok_kanan_3a:
    // Serial.println("DEBUG_Zigzag: belok_kanan_3a");
    // static float current_step = 0.0f;
    // static uint8_t iteration_counter = 0;
    
      // Check if reset was requested
      if (g_reset_zigzag_right) {
        //current_step = 0.0f;
        iteration_counter = 0;  // Reset counter juga
        g_reset_zigzag_right = false;
      }


      // Increment counter
      iteration_counter++;

      // Hanya update step jika counter mencapai interval
      if (iteration_counter >= rudder_step_interval) {
        if (current_step < 20.0f) {
          current_step += 2.0f;
          if (current_step >= 20.0f) {
            current_step = 20.0f;
          }
        }
        iteration_counter = 0;  // Reset counter
      }

      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      // Set offset dengan step saat ini
      servo_angle_current_offset = current_step;
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
      
      // HANYA TRANSISI KE STATE BERIKUTNYA JIKA SUDAH MENCAPAI TARGET
      if (current_step >= 20.0f) {
        state = belok_kanan_3b;
      }
    
    break;

   /*************************************************/
   
    case belok_kanan_3b:
    // Serial.println("DEBUG_Zigzag: belok_kanan_3b");
      // SELALU LAKUKAN KALKULASI DAN PENGATURAN SERVO
      calculated_angle = REFERENCE_ANGLE + servo_angle_current_offset;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    
      if (dataToSend.zigzag_yaw <= -2000) //menunggu heading zigzag lebih dari 20 derajat (× 100 = -2000)
      {
      state = lurus_akhir;
      g_reset_zigzag_right = true;
      }
    break;

     /*************************************************/

    case lurus_akhir:
     g_reset_zigzag_right = true;
     g_reset_zigzag_left = true;

     calculated_angle = REFERENCE_ANGLE;

      // Set duty cycle to be write for servo rudder degree move
      servo_duty = angleToDuty(calculated_angle);
      write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
    break;
     
  }
}

/**
 * @brief Mode Neutral - Set servo rudder ke posisi neutral (90°)
 * 
 * @details
 * Function ini digunakan ketika input RC berada di dead zone atau konflik.
 * Servo rudder akan di-set ke posisi neutral (90°).
 * 
 * @note
 * Digunakan untuk handle kasus:
 * - modeturning di dead zone (1250-1750) && modezigzag di dead zone (1250-1750)
 * - modeturning >= 1750 && modezigzag >= 1750 (konflik)
 * - modeturning >= 1750 && modezigzag <= 1250 (konflik)
 * - modeturning <= 1250 && modezigzag >= 1750 (konflik)
 * - modeturning <= 1250 && modezigzag <= 1250 (konflik)
 */
void rudder_neutral() {
  // Reset semua flag turning/zigzag
  g_reset_turning_left = true;
  g_reset_turning_right = true;
  g_reset_zigzag_left = true;
  g_reset_zigzag_right = true;
  state = belok_kanan_1a;  // Reset state ke awal
  
  // Set servo ke posisi neutral (90°)
  servo_angle_current_offset = 0.0f;
  float calculated_angle = REFERENCE_ANGLE;  // 90.0°
  servo_duty = angleToDuty(calculated_angle);
  bool write_result = ledcWrite(SERVO_RUDDER_pin, servo_duty);
  
  // Update zigzag setpoint dari yaw saat ini
  zigzag_setpoint = dataToSend.yaw / 100.0;
  dataToSend.zigzag_yaw = (int16_t)(((dataToSend.yaw / 100.0) - zigzag_setpoint) * 100);
}

/**
 * @brief Check dan eksekusi mode kontrol berdasarkan input RC
 * 
 * @param modeautomanual Channel 6: Mode auto/manual (≥1750: Auto, <1750: Manual)
 * @param modezigzag Channel 2: Mode zigzag (≤1250: Zigzag 10°, ≥1750: Zigzag 20°)
 * @param modeturning Channel 1: Mode turning (≥1750: Left, ≤1250: Right)
 * 
 * @details
 * Function ini menentukan mode kontrol berdasarkan kombinasi input dari receiver RC:
 * 
 * Mode Manual (modeautomanual < 1750):
 *   - Kontrol rudder langsung dari channel 1
 *   - mode_auto = 0
 * 
 * Mode Auto (modeautomanual >= 1750):
 *   - Turning Left (modeturning >= 1750 && modezigzag di dead zone): mode_auto = 1
 *   - Turning Right (modeturning <= 1250 && modezigzag di dead zone): mode_auto = 2
 *   - Zigzag 10° (modeturning di dead zone && modezigzag <= 1250): mode_auto = 3
 *   - Zigzag 20° (modeturning di dead zone && modezigzag >= 1750): mode_auto = 4
 *   - Neutral (konflik atau dead zone): mode_auto = 5
 * 
 * @note
 * Jika modeturning dan modezigzag keduanya aktif (konflik), maka masuk ke neutral
 * Dead zone: 1250-1750 (range antara aktif)
 */
void check_mode_auto_manual(uint16_t modeautomanual, uint16_t modezigzag, uint16_t modeturning) {
  if (modeautomanual >= 1750) {
    // ========== Mode Auto ==========
    
    // Check apakah modeturning aktif (>= 1750)
    if (modeturning >= 1750) {
      // Check apakah modezigzag juga aktif (konflik)
      if (modezigzag <= 1250 || modezigzag >= 1750) {
        // Konflik: modeturning aktif DAN modezigzag aktif → Neutral
        rudder_neutral();
        // Serial.println("Mode Neutral (Konflik: Turning Left + Zigzag)");
        dataToSend.mode_auto = 5;
      }
      else {
        // modeturning >= 1750 && modezigzag di dead zone (1250-1750) → Turning Left
        rudder_turning_right();
        // Serial.println("Mode Turning Left");
        dataToSend.mode_auto = 1;
      }
    } 
    // Check apakah modeturning aktif (<= 1250)
    else if (modeturning <= 1250) {
      // Check apakah modezigzag juga aktif (konflik)
      if (modezigzag <= 1250 || modezigzag >= 1750) {
        // Konflik: modeturning aktif DAN modezigzag aktif → Neutral
        rudder_neutral();
        // Serial.println("Mode Neutral (Konflik: Turning Right + Zigzag)");
        dataToSend.mode_auto = 5;
      }
      else {
        // modeturning <= 1250 && modezigzag di dead zone (1250-1750) → Turning Right
        rudder_turning_left();
        // Serial.println("Mode Turning Right");
        dataToSend.mode_auto = 2;
      }
    }
    // modeturning di dead zone (1250-1750)
    else {
      // Cek modezigzag
      if (modezigzag <= 1250) {
        // Mode: Zigzag 10°
        rudder_zigzag_10();
        // Serial.println("Mode Zigzag 10");
        dataToSend.mode_auto = 3;
      } 
      else if (modezigzag >= 1750) {
        // Mode: Zigzag 20°
        rudder_zigzag_20();
        // Serial.println("Mode Zigzag 20");
        dataToSend.mode_auto = 4;
      }
      else {
        // modeturning di dead zone (1250-1750) && modezigzag di dead zone (1250-1750)
        // Set servo ke neutral
        rudder_neutral();
        // Serial.println("Mode Neutral (Dead Zone)");
        dataToSend.mode_auto = 5;
      }
    }
  }
  else {
    // ========== Mode Manual ==========
    rudder_manual();
    //  Serial.println("Mode Auto Manual");
    dataToSend.mode_auto = 0;
  }
}

/**
 * @brief Setup function - Inisialisasi semua hardware dan komunikasi
 * 
 * @details
 * Function ini melakukan inisialisasi:
 * 1. Serial Monitor (115200 baud)
 * 2. PPM interrupt handler untuk receiver RC
 * 3. Pulse counter interrupt untuk rotary encoder
 * 4. ADC configuration untuk feedback dan battery monitoring
 * 5. Servo PWM configuration (rudder, propeller speed/direction)
 * 6. GNSS module (u-blox GPS) via Serial1
 * 7. IMU (HWT905TTL) via Serial2
 * 8. ESP-NOW untuk wireless communication
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
    
    // Setup Pulse Counter for Motor Prop 1
    pinMode(PULSE_PIN_Motor_prop_1, INPUT);
    attachInterrupt(digitalPinToInterrupt(PULSE_PIN_Motor_prop_1), pulse_interrupt_handler_motor_prop_1, RISING);
    
    // Setup Pulse Counter for Motor Prop 2
    pinMode(PULSE_PIN_Motor_prop_2, INPUT);
    attachInterrupt(digitalPinToInterrupt(PULSE_PIN_Motor_prop_2), pulse_interrupt_handler_motor_prop_2, RISING);
    
    // Configure ADC settings
    analogReadResolution(12);     
    
    dataToSend.zigzag_yaw = 0;

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
    memcpy(peerInfo.peer_addr, user_side_Address, 6);
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
 *    - Update data GPS (latitude, longitude, speed)
 *    - Update data IMU (roll, pitch, yaw)
 *    - Map PPM values ke range 1000-2000
 *    - Check mode auto/manual dan eksekusi fungsi kontrol
 *    - Baca feedback servo dari ADC
 *    - Kontrol motor propeller (speed dan direction)
 *    - Hitung RPM dari rotary encoder
 *    - Baca tegangan baterai
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

    // ========== Main Loop - Run setiap 100ms (10 Hz) ==========
    if (currentMillis - previousMillis >= intervaltime) {
        // Serial.print(currentMillis);
        // Serial.print(" ");
        previousMillis = currentMillis;
        dataToSend.timestamp = (double)currentMillis / 1000.0;

        // Snapshot to struct every interval, regardless of isUpdated()
        latestGpsData.latitude = gps.location.lat();
        latestGpsData.longitude = gps.location.lng();
        latestGpsData.speedMps = gps.speed.mps();

        // latestGpsData.locationValid = gps.location.isValid();
        // latestGpsData.timeValid = gps.time.isValid();
        // latestGpsData.locationAgeMs = gps.location.age();
        // latestGpsData.timeAgeMs = gps.time.age();
        // latestGpsData.altitudeM = gps.altitude.meters();
        // latestGpsData.hdop = gps.hdop.value() / 100.0;
        // latestGpsData.satellites = gps.satellites.value();
        // if (latestGpsData.locationValid) {
        // }
        dataToSend.latitude = latestGpsData.latitude;
        dataToSend.longitude = latestGpsData.longitude;
        // Konversi speedMps ke uint16_t (× 100)
        dataToSend.speedMps = (uint16_t)(latestGpsData.speedMps * 100);

        // Hitung roll, pitch, yaw sebagai float dulu
        float roll = (float)JY901.stcAngle.Angle[0]/32768*180;
        float pitch = (float)JY901.stcAngle.Angle[1]/32768*180;
        float rawYaw = (float)JY901.stcAngle.Angle[2]/32768*180;
        
        // Modifikasi: jika yaw < 0, jadikan 360 + yaw
        float yaw;
        if (rawYaw < 0) {
          yaw = 360.0 + rawYaw;
        } else {
          yaw = rawYaw;
        }
        
        // Konversi ke int16_t/uint16_t (× 100)
        dataToSend.roll = (int16_t)(roll * 100);
        dataToSend.pitch = (int16_t)(pitch * 100);
        dataToSend.yaw = (uint16_t)(yaw * 100);
        
        for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
            // Convert raw PPM FS-iA6B values (600-1600) to standard servo range (1000-2000)
            ppm_mapped[i] = map(ppm_values[i], PPM_MIN_CHANNEL_VALUE, PPM_MAX_CHANNEL_VALUE, 1000, 2000);
        }
        
        /*
        controlInput.rudder = ppm_mapped[0]; CH1 tuas kanan bergerak kanan kiri
        controlInput.mode_zigzag = ppm_mapped[1]; // CH2 tuas kanan bergerak atas bawah
        controlInput.propSpeed = ppm_mapped[2];   // CH3 CH2 tuas kiri bergerak atas bawah
        controlInput.propDirection = ppm_mapped[4]; // CH5 Switch A bergerak atas bawah
        controlInput.mode_auto_manual = ppm_mapped[5]; CH6 Switch B bergerak atas bawah
        
        */
        controlInput.mode_auto_manual = ppm_mapped[5];  // CH6
        controlInput.mode_zigzag = ppm_mapped[1];        // CH2
        controlInput.rudder = ppm_mapped[0];              // CH1
        
        check_mode_auto_manual (controlInput.mode_auto_manual, controlInput.mode_zigzag, controlInput.rudder);

        
        // Read analog values of servo potensiometer output in millivolts
        uint32_t adc_millivolts_servo_1 = analogReadMilliVolts(ADC_PIN_SERVO_1);
        uint32_t adc_millivolts_servo_2 = analogReadMilliVolts(ADC_PIN_SERVO_2);
  
        float Calc_deg_servo_1 = adc_millivolts_servo_1 * 0.0595 - 98.848;
        float Calc_deg_servo_2 = adc_millivolts_servo_2 * 0.0594 - 98.801;

        // Set PWM for PropSpeed dan PropDirection
        controlInput.propSpeed = ppm_mapped[2];           // CH3
        controlInput.propDirection = ppm_mapped[4];       // CH5
        
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

        // Buat payload ringkas untuk pengiriman/logging
        // Konversi servo angle ke int16_t (× 100)
        dataToSend.Calc_deg_servo_1 = (int16_t)(Calc_deg_servo_1 * 100);
        dataToSend.Calc_deg_servo_2 = (int16_t)(Calc_deg_servo_2 * 100);
        (void)dataToSend; // hindari peringatan variabel tidak terpakai jika belum digunakan

        // ========== Calculate RPM for Motor Prop 1 ==========
        // Calculate pulses per loop (100ms) using delta method
        static uint32_t previous_pulse_counter_motor_prop_1 = 0;
        uint32_t pulses_per_loop_motor_prop_1 = pulse_counter_motor_prop_1 - previous_pulse_counter_motor_prop_1;
        previous_pulse_counter_motor_prop_1 = pulse_counter_motor_prop_1;
        
        // Add to moving average buffer (circular buffer)
        pulse_buffer_motor_prop_1[buffer_index_motor_prop_1] = pulses_per_loop_motor_prop_1;
        buffer_index_motor_prop_1 = (buffer_index_motor_prop_1 + 1) % RPM_SAMPLES_motor_prop_1;  // Circular index
        
        // Calculate moving average
        uint32_t pulse_sum_motor_prop_1 = 0;
        for (uint8_t i = 0; i < RPM_SAMPLES_motor_prop_1; i++) {
            pulse_sum_motor_prop_1 += pulse_buffer_motor_prop_1[i];
        }
        float avg_pulses_per_loop_motor_prop_1 = pulse_sum_motor_prop_1 / (float)RPM_SAMPLES_motor_prop_1;
        
        // Calculate RPM and cast directly to uint16_t (PPR = 1, loop interval = 100ms = 0.1s)
        // RPM = (avg_pulses_per_loop / 0.1s) * 60 = avg_pulses_per_loop * 600
        dataToSend.rpm_prop_1 = (uint16_t)(avg_pulses_per_loop_motor_prop_1 * 600.0);  //rpm motor propeller 1
        
        // ========== Calculate RPM for Motor Prop 2 ==========
        // Calculate pulses per loop (100ms) using delta method
        static uint32_t previous_pulse_counter_motor_prop_2 = 0;
        uint32_t pulses_per_loop_motor_prop_2 = pulse_counter_motor_prop_2 - previous_pulse_counter_motor_prop_2;
        previous_pulse_counter_motor_prop_2 = pulse_counter_motor_prop_2;
        
        // Add to moving average buffer (circular buffer)
        pulse_buffer_motor_prop_2[buffer_index_motor_prop_2] = pulses_per_loop_motor_prop_2;
        buffer_index_motor_prop_2 = (buffer_index_motor_prop_2 + 1) % RPM_SAMPLES_motor_prop_2;  // Circular index
        
        // Calculate moving average
        uint32_t pulse_sum_motor_prop_2 = 0;
        for (uint8_t i = 0; i < RPM_SAMPLES_motor_prop_2; i++) {
            pulse_sum_motor_prop_2 += pulse_buffer_motor_prop_2[i];
        }
        float avg_pulses_per_loop_motor_prop_2 = pulse_sum_motor_prop_2 / (float)RPM_SAMPLES_motor_prop_2;
        
        // Calculate RPM and cast directly to uint16_t (PPR = 1, loop interval = 100ms = 0.1s)
        // RPM = (avg_pulses_per_loop / 0.1s) * 60 = avg_pulses_per_loop * 600
        dataToSend.rpm_prop_2 = (uint16_t)(avg_pulses_per_loop_motor_prop_2 * 600.0);  //rpm motor propeller 2
        
        // Print RPM motor prop 1 and 2
        // Serial.print("RPM_prop_1: ");
        // Serial.print(dataToSend.rpm_prop_1);
        // Serial.print(" | RPM_prop_2: ");
        // Serial.print(dataToSend.rpm_prop_2);
        // Serial.print(" | Total_pulses_1: ");
        // Serial.print(pulse_counter_motor_prop_1);
        // Serial.print(" | Total_pulses_2: ");
        // Serial.println(pulse_counter_motor_prop_2); 

        // Baca ADC dalam miliVolt dari GPIO1 (ADC1_CH0)
        uint32_t adc_millivolts_batt_1 = analogReadMilliVolts(ADC_PIN_BATT_1);
        // Konversi mV ke V, lalu kalikan dengan faktor skala 5 (voltage divider)
        float volt_batt_1 = (adc_millivolts_batt_1 / 1000.0) * 5.0;
        // Konversi ke uint16_t (× 100) untuk pengiriman
        dataToSend.battery_1 = (uint16_t)(volt_batt_1 * 100); // batere for ESP32-S3, Servo, HWT905TTL, Receiver RC, GNSS, Rotary Encoder
        
        // Baca ADC dalam miliVolt dari GPIO2 (ADC1_CH1)
        uint32_t adc_millivolts_batt_2 = analogReadMilliVolts(ADC_PIN_BATT_2);
        // Konversi mV ke V, lalu kalikan dengan faktor skala 5 (voltage divider)
        float volt_batt_2 = (adc_millivolts_batt_2 / 1000.0) * 5.0;
        // Konversi ke uint16_t (× 100) untuk pengiriman
        dataToSend.battery_2 = (uint16_t)(volt_batt_2 * 100); // batere for motor propeller

        // ========== Serial Print Debugging (SETELAH semua proses kontrol selesai) ==========
        // Print dilakukan SETELAH check_mode_auto_manual() selesai (yang sudah mengupdate zigzag_setpoint dan zigzag_yaw)
        // dan SEBELUM mengirim data via ESP-NOW, sehingga nilai yang di-print adalah nilai final yang akan dikirim
        //Print CSV: timestamp,lat,lon,speed,deg1,deg2
        // Konversi fixed-point kembali ke float untuk display (÷ 100)
        // 
        Serial.print(dataToSend.timestamp, 3); Serial.println("");
        // Serial.print(dataToSend.latitude, 6); Serial.println(""); //Serial.print(",");
        // Serial.print(dataToSend.longitude, 6); Serial.print(",");
        // Serial.print(dataToSend.speedMps / 100.0, 2); Serial.print(",");
        Serial.print(dataToSend.Calc_deg_servo_1 / 100.0, 2); Serial.print(",");
        Serial.print(dataToSend.Calc_deg_servo_2 / 100.0, 2); Serial.print(",");
        // Serial.print(dataToSend.roll / 100.0, 2); Serial.print(",");
        // Serial.print(dataToSend.pitch / 100. 0, 2); Serial.print(",");
        Serial.print(dataToSend.yaw / 100.0, 2); Serial.print(",");
        Serial.print(zigzag_setpoint, 2); Serial.print(",");
        Serial.print(dataToSend.zigzag_yaw / 100.0, 2); Serial.print(","); //Serial.println(""); 
        // Serial.print(dataToSend.rpm_prop_1 / 100.0, 2); Serial.print(",");
        // Serial.print(dataToSend.rpm_prop_2 / 100.0, 2); Serial.print(",");
        // Serial.print(dataToSend.battery_1 / 100.0, 2); Serial.print(",");
        // Serial.println(dataToSend.battery_2 / 100.0, 2);
        Serial.println(dataToSend.mode_auto);

        // ========== Send message via ESP-NOW ==========
        esp_err_t result = esp_now_send(user_side_Address, (uint8_t *) &dataToSend, sizeof(dataToSend));
        
        if (result == ESP_OK) {
          //Serial.println("Sent with success");
        }
        else {
          //Serial.println("Error sending the data");
        }

        // Serial.println(millis()/1000.000,3);
    } // end of if interval
}

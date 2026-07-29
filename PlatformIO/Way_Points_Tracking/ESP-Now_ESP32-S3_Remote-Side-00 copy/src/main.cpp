/*
  ESP32-S3 No 1 Remote-Side (di kapal model)

  Peran:
  - Mengirim telemetry 15-kolom @10 Hz ke User-Side via ESP-NOW.
  - Menerima waypoints (Home + max 10 waypoint navigasi) dari User-Side
    via ESP-NOW dan menyimpannya untuk dipakai logic kontrol (saat ini
    hanya disimpan ke variabel global + dicetak ke Serial untuk debug).
*/

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// MAC address ESP32-S3 DevKitC-1 user-side (peer ESP-NOW)
//uint8_t user_side_Address[] = {0x10, 0x20, 0xba, 0x4c, 0x53, 0xfc};
// uint8_t user_side_Address[] = {0x98, 0xa3, 0x16, 0xf5, 0x01, 0xa0};
uint8_t user_side_Address[] = {0x80, 0xb5, 0x4e, 0xc1, 0xd5, 0xac};

// =====================================================================
// Waypoints struct (diterima dari User-Side). HARUS identik byte-per-byte
// dengan struct waypoints_payload di firmware User-Side.
//
// Layout (total 180 byte, < 250 byte limit ESP-NOW):
//   1 byte  msg_type     (0xA1)
//   1 byte  home_valid   (0/1)
//   1 byte  wp_count     (0..WP_MAX_COUNT)
//   1 byte  reserved     (padding alignment)
//   8 byte  home_lat
//   8 byte  home_lon
//   8 byte * 10 wp_lat[]
//   8 byte * 10 wp_lon[]
// =====================================================================
#define WP_MAX_COUNT 10
#define WP_MSG_TYPE  0xA1

typedef struct waypoints_payload {
  uint8_t  msg_type;
  uint8_t  home_valid;
  uint8_t  wp_count;
  uint8_t  reserved;
  double   home_lat;
  double   home_lon;
  double   wp_lat[WP_MAX_COUNT];
  double   wp_lon[WP_MAX_COUNT];
} waypoints_payload;

// Storage global untuk waypoints terakhir yang diterima.
// Logic kontrol kapal (di iterasi berikutnya) tinggal membaca variable
// ini untuk navigasi: g_lastWaypoints.home_lat/lon, g_lastWaypoints.wp_*.
static waypoints_payload g_lastWaypoints;
static bool              g_hasWaypoints = false;

// =====================================================================
// Telemetry struct (dikirim ke User-Side). 15 field, urutan & tipe
// HARUS sama dengan struct receivedfromremoteside di firmware User-Side.
// =====================================================================
typedef struct send_to_user_side {
  double timestamp;
  double latitude;
  double longitude;
  int16_t speedMps;
  int16_t Calc_deg_servo_1;
  int16_t Calc_deg_servo_2;
  int16_t roll;
  int16_t pitch;
  int16_t yaw;
  int16_t zigzag_yaw;
  int16_t rpm_prop_1;
  int16_t rpm_prop_2;
  int16_t battery_1;
  int16_t battery_2;
  uint8_t mode_auto;
} send_to_user_side;

send_to_user_side mysend_to_user_sideData;

esp_now_peer_info_t peerInfo;

// =====================================================================
// Cetak isi waypoints ke Serial (untuk debug / monitoring di sisi kapal).
// =====================================================================
static void printWaypoints(const waypoints_payload &wp) {
  Serial.print("[WP] msg_type=0x");
  Serial.print(wp.msg_type, HEX);
  Serial.print(" home_valid=");
  Serial.print(wp.home_valid);
  Serial.print(" count=");
  Serial.println(wp.wp_count);

  if (wp.home_valid) {
    Serial.print("[WP] Home: ");
    Serial.print(wp.home_lat, 6); Serial.print(", ");
    Serial.println(wp.home_lon, 6);
  } else {
    Serial.println("[WP] Home: <none>");
  }

  uint8_t n = wp.wp_count;
  if (n > WP_MAX_COUNT) n = WP_MAX_COUNT;
  for (uint8_t i = 0; i < n; i++) {
    Serial.print("[WP] #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(wp.wp_lat[i], 6); Serial.print(", ");
    Serial.println(wp.wp_lon[i], 6);
  }
}

// =====================================================================
// Callback ESP-NOW. Dispatch berdasarkan panjang paket.
// =====================================================================
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Paket waypoints (cek msg_type juga untuk memastikan, agar paket lain
  // dengan panjang kebetulan sama tidak salah di-parse).
  if (len == (int)sizeof(waypoints_payload) && incomingData[0] == WP_MSG_TYPE) {
    memcpy(&g_lastWaypoints, incomingData, sizeof(g_lastWaypoints));
    g_hasWaypoints = true;
    Serial.print("Bytes received from User Side: ");
    Serial.println(len);
    printWaypoints(g_lastWaypoints);
    Serial.println();
    return;
  }

  // Paket dengan panjang tidak dikenal -> log saja agar mudah debug.
  Serial.print("[WARN] Unknown ESP-NOW payload length: ");
  Serial.println(len);
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 No 1 Remote-Side");

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, user_side_Address, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop() {
  static unsigned long lastSendMs = 0;
  const unsigned long sendIntervalMs = 100;
  unsigned long now = millis();

  if (now - lastSendMs < sendIntervalMs) {
    return;
  }
  lastSendMs = now;

  // Set values to send (mengikuti format Generate_15_random_data_ASCII)
  mysend_to_user_sideData.timestamp = now / 1000.0;
  mysend_to_user_sideData.latitude = -7.281500 + (random(-500, 501) / 1000000.0);
  mysend_to_user_sideData.longitude = 112.798900 + (random(-500, 501) / 1000000.0);
  mysend_to_user_sideData.speedMps = (int16_t)random(0, 351);
  mysend_to_user_sideData.Calc_deg_servo_1 = (int16_t)random(-4000, 4001);
  mysend_to_user_sideData.Calc_deg_servo_2 = (int16_t)random(-4000, 4001);
  mysend_to_user_sideData.roll = (int16_t)random(-18000, 18001);
  mysend_to_user_sideData.pitch = (int16_t)random(-18000, 18001);
  mysend_to_user_sideData.yaw = (int16_t)random(0, 36001);
  mysend_to_user_sideData.zigzag_yaw = (int16_t)random(-3500, 3501);
  mysend_to_user_sideData.rpm_prop_1 = (int16_t)random(0, 30001);
  mysend_to_user_sideData.rpm_prop_2 = (int16_t)random(0, 30001);
  mysend_to_user_sideData.battery_1 = (int16_t)random(1000, 1260);
  mysend_to_user_sideData.battery_2 = (int16_t)random(1000, 1260);
  mysend_to_user_sideData.mode_auto = (uint8_t)random(0, 5);

  esp_now_send(user_side_Address, (uint8_t *) &mysend_to_user_sideData, sizeof(mysend_to_user_sideData));
}

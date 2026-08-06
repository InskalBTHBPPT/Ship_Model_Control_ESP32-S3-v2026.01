/*
  ESP32-S3 User-Side-03 (gateway USB <-> ESP-NOW)

  Peran:
  - Menerima telemetry 23-kolom dari Remote-Side-03 via ESP-NOW dan
    meneruskannya ke PC sebagai CSV via Serial (115200 baud).
  - Menerima command "$WPSET,..." dari PC (dashboard PySide6) via Serial
    dan meneruskannya ke Remote-Side via ESP-NOW dalam bentuk
    struct waypoints_payload.

  Protokol kontrol PC -> User-Side (ASCII, diakhiri '\n'):
    $WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>

  Balasan User-Side -> PC:
    $WACK,OK
    $WACK,ERR,<reason>
*/

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// MAC address ESP32-S3 DevKitC-1 remote-side (peer ESP-NOW)
//uint8_t remote_side_Address[] = {0x94, 0xa9, 0x90, 0x30, 0xab, 0xc0};
uint8_t remote_side_Address[] = {0x98, 0xa3, 0x16, 0xf5, 0x01, 0xa0};

// =====================================================================
// Telemetry struct (diterima dari Remote-Side-03). 23 field, urutan & tipe
// HARUS sama dengan struct DatatoSend di firmware Remote-Side-03.
// Total sizeof = 64 byte (62 data + 2 padding).
// =====================================================================
typedef struct receivedfromremoteside {
  double timestamp;
  double latitude;
  double longitude;
  uint16_t speedMps;
  int16_t Calc_deg_servo_1;
  int16_t Calc_deg_servo_2;
  uint16_t yaw;
  uint16_t heading_setpoint;
  int16_t heading_error;
  int16_t rudder_cmd;
  uint8_t track_wp_index;
  uint16_t distance_to_wp;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  uint16_t rpm_prop_1;
  uint16_t rpm_prop_2;
  uint16_t battery_1;
  uint16_t battery_2;
  uint8_t mode_auto;
} receivedfromremoteside;

receivedfromremoteside myReceivedFromremoteSideData;

// =====================================================================
// Waypoints struct (dikirim ke Remote-Side). HARUS identik byte-per-byte
// dengan struct waypoints_payload di firmware Remote-Side.
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

waypoints_payload myWaypointsPayload;

esp_now_peer_info_t peerInfo;

// =====================================================================
// Callback ESP-NOW: telemetry dari Remote-Side -> CSV ke PC
// =====================================================================
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != (int)sizeof(receivedfromremoteside)) {
    return;
  }
  memcpy(&myReceivedFromremoteSideData, incomingData, sizeof(myReceivedFromremoteSideData));
  // Print CSV 23 kolom (raw fixed-point) agar sama format dengan generator
  Serial.print(myReceivedFromremoteSideData.timestamp, 3); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.latitude, 6); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.longitude, 6); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.speedMps); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.Calc_deg_servo_1); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.Calc_deg_servo_2); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.yaw); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.heading_setpoint); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.heading_error); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.rudder_cmd); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.track_wp_index); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.distance_to_wp); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.accel_x); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.accel_y); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.accel_z); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.gyro_x); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.gyro_y); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.gyro_z); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.rpm_prop_1); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.rpm_prop_2); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.battery_1); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.battery_2); Serial.print(",");
  Serial.println(myReceivedFromremoteSideData.mode_auto);
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 User-Side-03");

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, remote_side_Address, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

// =====================================================================
// Strict parsers (token harus angka penuh, tanpa karakter sisa)
// =====================================================================
static bool parseIntStrict(const String& s, long &out) {
  String t = s; t.trim();
  if (t.length() == 0) return false;
  char* endp = nullptr;
  long v = strtol(t.c_str(), &endp, 10);
  if (endp == t.c_str() || endp == nullptr || *endp != '\0') return false;
  out = v;
  return true;
}

static bool parseDoubleStrict(const String& s, double &out) {
  String t = s; t.trim();
  if (t.length() == 0) return false;
  char* endp = nullptr;
  double v = strtod(t.c_str(), &endp);
  if (endp == t.c_str() || endp == nullptr || *endp != '\0') return false;
  out = v;
  return true;
}

// =====================================================================
// Parser $WPSET. Format yang diharapkan (sudah tanpa newline):
//   $WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...,<latN>,<lonN>
// dengan wp_count = 0..WP_MAX_COUNT, dan jumlah pasangan lat,lon
// SETELAH count harus sama persis dengan wp_count.
// =====================================================================

// Token buffer cukup untuk: home_lat, home_lon, count, + 2*WP_MAX_COUNT pair
// = 3 + 20 = 23. Beri 1 slot ekstra untuk deteksi error "kelebihan".
#define WP_TOKEN_BUF_SIZE 24

static int tokenizeByComma(const String& payload, String tokens[], int maxTokens) {
  int count = 0;
  int start = 0;
  int len = (int)payload.length();
  while (start <= len && count < maxTokens) {
    int comma = payload.indexOf(',', start);
    if (comma < 0) {
      tokens[count++] = payload.substring(start);
      break;
    }
    tokens[count++] = payload.substring(start, comma);
    start = comma + 1;
  }
  return count;
}

static void processSerialLine(const String& line) {
  if (!line.startsWith("$WPSET,")) {
    return;  // baris non-protokol diabaikan tanpa balasan
  }
  String payload = line.substring(7);

  String tokens[WP_TOKEN_BUF_SIZE];
  int n = tokenizeByComma(payload, tokens, WP_TOKEN_BUF_SIZE);

  if (n < 3) {
    Serial.println("$WACK,ERR,FORMAT");
    return;
  }

  double home_lat = 0.0, home_lon = 0.0;
  long wp_count = 0;
  if (!parseDoubleStrict(tokens[0], home_lat)) { Serial.println("$WACK,ERR,HOME_LAT"); return; }
  if (!parseDoubleStrict(tokens[1], home_lon)) { Serial.println("$WACK,ERR,HOME_LON"); return; }
  if (!parseIntStrict(tokens[2],   wp_count))  { Serial.println("$WACK,ERR,COUNT_NOT_INT"); return; }

  if (wp_count < 0 || wp_count > WP_MAX_COUNT) {
    Serial.print("$WACK,ERR,COUNT_RANGE,");
    Serial.println(wp_count);
    return;
  }

  long expected = 3 + 2 * wp_count;
  if (n != expected) {
    Serial.print("$WACK,ERR,COUNT_MISMATCH,");
    Serial.print(n);
    Serial.print(",exp,");
    Serial.println(expected);
    return;
  }

  if (home_lat < -90.0 || home_lat > 90.0)   { Serial.println("$WACK,ERR,LAT_RANGE,home"); return; }
  if (home_lon < -180.0 || home_lon > 180.0) { Serial.println("$WACK,ERR,LON_RANGE,home"); return; }

  double wp_lat_local[WP_MAX_COUNT];
  double wp_lon_local[WP_MAX_COUNT];
  for (int i = 0; i < wp_count; i++) {
    if (!parseDoubleStrict(tokens[3 + 2 * i], wp_lat_local[i])) {
      Serial.print("$WACK,ERR,WP_LAT,"); Serial.println(i + 1);
      return;
    }
    if (!parseDoubleStrict(tokens[3 + 2 * i + 1], wp_lon_local[i])) {
      Serial.print("$WACK,ERR,WP_LON,"); Serial.println(i + 1);
      return;
    }
    if (wp_lat_local[i] < -90.0 || wp_lat_local[i] > 90.0) {
      Serial.print("$WACK,ERR,LAT_RANGE,"); Serial.println(i + 1);
      return;
    }
    if (wp_lon_local[i] < -180.0 || wp_lon_local[i] > 180.0) {
      Serial.print("$WACK,ERR,LON_RANGE,"); Serial.println(i + 1);
      return;
    }
  }

  // Susun struct dan kirim via ESP-NOW
  memset(&myWaypointsPayload, 0, sizeof(myWaypointsPayload));
  myWaypointsPayload.msg_type   = WP_MSG_TYPE;
  myWaypointsPayload.home_valid = 1;
  myWaypointsPayload.wp_count   = (uint8_t)wp_count;
  myWaypointsPayload.home_lat   = home_lat;
  myWaypointsPayload.home_lon   = home_lon;
  for (int i = 0; i < wp_count; i++) {
    myWaypointsPayload.wp_lat[i] = wp_lat_local[i];
    myWaypointsPayload.wp_lon[i] = wp_lon_local[i];
  }

  esp_err_t result = esp_now_send(
      remote_side_Address,
      (uint8_t *)&myWaypointsPayload,
      sizeof(myWaypointsPayload));

  if (result == ESP_OK) {
    Serial.println("$WACK,OK");
  } else {
    Serial.println("$WACK,ERR,SEND_FAIL");
  }
}

void loop() {
  // Non-blocking serial reader: kumpulkan byte sampai '\n', lalu parse.
  // Buffer diperbesar agar muat $WPSET dengan 1 home + 10 waypoint
  // (panjang maksimum praktis ~270 karakter).
  static String rxBuf;
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      processSerialLine(rxBuf);
      rxBuf = "";
    } else {
      if (rxBuf.length() < 350) {
        rxBuf += ch;
      } else {
        rxBuf = "";
        Serial.println("$WACK,ERR,LINE_TOO_LONG");
      }
    }
  }
}

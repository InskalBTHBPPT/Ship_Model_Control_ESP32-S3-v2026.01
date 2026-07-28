/**
 * @file main.cpp
 * @brief ESP32-S3 User-Side-01 — gateway USB-serial ↔ ESP-NOW
 *
 * @description
 * Meneruskan perintah dashboard ke Remote via ESP-NOW dan relay telemetry
 * CSV 23-kolom ke PC. ACK WP/TUN hanya setelah Remote membalas 0xC1.
 *
 * PC → User-Side (ASCII, '\n'):
 *   $WPSET,<home_lat>,<home_lon>,<wp_count>,<lat1>,<lon1>,...
 *   $TUNSET,<alg>[,<kp>,<kd>,<arrive_m>,<rudder_max>]
 *   $TUNGET
 *
 * User-Side → PC:
 *   $WACK,OK,WP | $WACK,OK,TUN | $WACK,ERR,<kind>,<reason>
 *   $TACK,<alg>,<kp>,<kd>,<arrive_m>,<rudder_max>
 *   $TACK,ERR,<reason>
 *
 * @author Chandra P - Ship Model Control System
 * @version 1.0
 * @date 2026
 *
 * @note Lihat README.md di root proyek untuk detail protokol.
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

uint8_t remote_side_Address[] = {0x10, 0x20, 0xba, 0x4c, 0x53, 0xfc};

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

#define TUN_MSG_TYPE_SET   0xA2
#define TUN_MSG_TYPE_RESP  0xA3
#define TUN_MSG_TYPE_GET   0xB1
#define ACK_MSG_TYPE       0xC1
#define ACK_KIND_WP        1
#define ACK_KIND_TUN       2
#define ACK_STATUS_OK      0
#define ACK_STATUS_ERR     1
#define TUN_PARAM_COUNT_ALG1 4

typedef struct track_config_payload {
  uint8_t msg_type;
  uint8_t active_alg;
  uint8_t param_count;
  uint8_t reserved;
  float params[4];
} track_config_payload;

typedef struct remote_ack_payload {
  uint8_t msg_type;
  uint8_t ack_kind;
  uint8_t status;
  uint8_t err_code;
} remote_ack_payload;

typedef struct tun_get_request {
  uint8_t msg_type;
} tun_get_request;

waypoints_payload myWaypointsPayload;
track_config_payload myTrackConfigPayload;

esp_now_peer_info_t peerInfo;

static unsigned long g_ack_deadline_ms = 0;
static uint8_t g_waiting_ack_kind = 0;
static bool g_waiting_tack = false;
static const unsigned long ACK_TIMEOUT_MS = 2500;

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

static void printTelemetryCsv() {
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

static void handleRemoteAck(const remote_ack_payload &ack) {
  g_ack_deadline_ms = 0;
  g_waiting_ack_kind = 0;
  const char *kind_str = (ack.ack_kind == ACK_KIND_WP) ? "WP" : "TUN";
  if (ack.status == ACK_STATUS_OK) {
    Serial.print("$WACK,OK,");
    Serial.println(kind_str);
  } else {
    Serial.print("$WACK,ERR,");
    Serial.print(kind_str);
    Serial.print(",");
    Serial.println(ack.err_code);
  }
}

static void handleTrackConfigResp(const track_config_payload &resp) {
  g_waiting_tack = false;
  g_ack_deadline_ms = 0;
  Serial.print("$TACK,");
  Serial.print(resp.active_alg);
  Serial.print(",");
  Serial.print(resp.params[0], 4);
  Serial.print(",");
  Serial.print(resp.params[1], 4);
  Serial.print(",");
  Serial.print(resp.params[2], 2);
  Serial.print(",");
  Serial.println(resp.params[3], 2);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len == (int)sizeof(remote_ack_payload) && incomingData[0] == ACK_MSG_TYPE) {
    remote_ack_payload ack;
    memcpy(&ack, incomingData, sizeof(ack));
    handleRemoteAck(ack);
    return;
  }
  if (len == (int)sizeof(track_config_payload) && incomingData[0] == TUN_MSG_TYPE_RESP) {
    track_config_payload resp;
    memcpy(&resp, incomingData, sizeof(resp));
    handleTrackConfigResp(resp);
    return;
  }
  if (len != (int)sizeof(receivedfromremoteside)) {
    return;
  }
  memcpy(&myReceivedFromremoteSideData, incomingData, sizeof(myReceivedFromremoteSideData));
  printTelemetryCsv();
}

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

static void processWpSetLine(const String& line) {
  String payload = line.substring(7);
  String tokens[WP_TOKEN_BUF_SIZE];
  int n = tokenizeByComma(payload, tokens, WP_TOKEN_BUF_SIZE);

  if (n < 3) {
    Serial.println("$WACK,ERR,WP,FORMAT");
    return;
  }

  double home_lat = 0.0, home_lon = 0.0;
  long wp_count = 0;
  if (!parseDoubleStrict(tokens[0], home_lat)) { Serial.println("$WACK,ERR,WP,HOME_LAT"); return; }
  if (!parseDoubleStrict(tokens[1], home_lon)) { Serial.println("$WACK,ERR,WP,HOME_LON"); return; }
  if (!parseIntStrict(tokens[2],   wp_count))  { Serial.println("$WACK,ERR,WP,COUNT_NOT_INT"); return; }

  if (wp_count < 0 || wp_count > WP_MAX_COUNT) {
    Serial.println("$WACK,ERR,WP,COUNT_RANGE");
    return;
  }

  long expected = 3 + 2 * wp_count;
  if (n != expected) {
    Serial.println("$WACK,ERR,WP,COUNT_MISMATCH");
    return;
  }

  if (home_lat < -90.0 || home_lat > 90.0)   { Serial.println("$WACK,ERR,WP,LAT_RANGE,home"); return; }
  if (home_lon < -180.0 || home_lon > 180.0) { Serial.println("$WACK,ERR,WP,LON_RANGE,home"); return; }

  double wp_lat_local[WP_MAX_COUNT];
  double wp_lon_local[WP_MAX_COUNT];
  for (int i = 0; i < wp_count; i++) {
    if (!parseDoubleStrict(tokens[3 + 2 * i], wp_lat_local[i])) {
      Serial.print("$WACK,ERR,WP,WP_LAT,"); Serial.println(i + 1);
      return;
    }
    if (!parseDoubleStrict(tokens[3 + 2 * i + 1], wp_lon_local[i])) {
      Serial.print("$WACK,ERR,WP,WP_LON,"); Serial.println(i + 1);
      return;
    }
    if (wp_lat_local[i] < -90.0 || wp_lat_local[i] > 90.0) {
      Serial.print("$WACK,ERR,WP,LAT_RANGE,"); Serial.println(i + 1);
      return;
    }
    if (wp_lon_local[i] < -180.0 || wp_lon_local[i] > 180.0) {
      Serial.print("$WACK,ERR,WP,LON_RANGE,"); Serial.println(i + 1);
      return;
    }
  }

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

  if (result != ESP_OK) {
    Serial.println("$WACK,ERR,WP,SEND_FAIL");
    return;
  }
  g_waiting_ack_kind = ACK_KIND_WP;
  g_ack_deadline_ms = millis() + ACK_TIMEOUT_MS;
}

static void processTunSetLine(const String& line) {
  String payload = line.substring(8);
  String tokens[8];
  int n = tokenizeByComma(payload, tokens, 8);
  if (n < 1) {
    Serial.println("$WACK,ERR,TUN,FORMAT");
    return;
  }

  long alg = 0;
  if (!parseIntStrict(tokens[0], alg)) {
    Serial.println("$WACK,ERR,TUN,ALG");
    return;
  }
  if (alg != 1 && alg != 2) {
    Serial.println("$WACK,ERR,TUN,ALG_RANGE");
    return;
  }

  memset(&myTrackConfigPayload, 0, sizeof(myTrackConfigPayload));
  myTrackConfigPayload.msg_type = TUN_MSG_TYPE_SET;
  myTrackConfigPayload.active_alg = (uint8_t)alg;

  if (alg == 1) {
    if (n != 5) {
      Serial.println("$WACK,ERR,TUN,PARAM_COUNT");
      return;
    }
    double v0, v1, v2, v3;
    if (!parseDoubleStrict(tokens[1], v0)) { Serial.println("$WACK,ERR,TUN,KP"); return; }
    if (!parseDoubleStrict(tokens[2], v1)) { Serial.println("$WACK,ERR,TUN,KD"); return; }
    if (!parseDoubleStrict(tokens[3], v2)) { Serial.println("$WACK,ERR,TUN,ARRIVE"); return; }
    if (!parseDoubleStrict(tokens[4], v3)) { Serial.println("$WACK,ERR,TUN,RUDMAX"); return; }
    myTrackConfigPayload.param_count = TUN_PARAM_COUNT_ALG1;
    myTrackConfigPayload.params[0] = (float)v0;
    myTrackConfigPayload.params[1] = (float)v1;
    myTrackConfigPayload.params[2] = (float)v2;
    myTrackConfigPayload.params[3] = (float)v3;
  } else {
    myTrackConfigPayload.param_count = 0;
  }

  esp_err_t result = esp_now_send(
      remote_side_Address,
      (uint8_t *)&myTrackConfigPayload,
      sizeof(myTrackConfigPayload));

  if (result != ESP_OK) {
    Serial.println("$WACK,ERR,TUN,SEND_FAIL");
    return;
  }
  g_waiting_ack_kind = ACK_KIND_TUN;
  g_ack_deadline_ms = millis() + ACK_TIMEOUT_MS;
}

static void processTunGetLine() {
  tun_get_request req = {TUN_MSG_TYPE_GET};
  esp_err_t result = esp_now_send(
      remote_side_Address,
      (uint8_t *)&req,
      sizeof(req));
  if (result != ESP_OK) {
    Serial.println("$TACK,ERR,SEND_FAIL");
    return;
  }
  g_waiting_tack = true;
  g_ack_deadline_ms = millis() + ACK_TIMEOUT_MS;
}

static void processSerialLine(const String& line) {
  if (line.startsWith("$WPSET,")) {
    processWpSetLine(line);
    return;
  }
  if (line.startsWith("$TUNSET,")) {
    processTunSetLine(line);
    return;
  }
  if (line == "$TUNGET") {
    processTunGetLine();
    return;
  }
}

static void checkAckTimeout() {
  if (g_ack_deadline_ms == 0 || millis() < g_ack_deadline_ms) {
    return;
  }
  g_ack_deadline_ms = 0;
  if (g_waiting_tack) {
    g_waiting_tack = false;
    Serial.println("$TACK,ERR,TIMEOUT");
    return;
  }
  if (g_waiting_ack_kind == ACK_KIND_WP) {
    g_waiting_ack_kind = 0;
    Serial.println("$WACK,ERR,WP,TIMEOUT");
    return;
  }
  if (g_waiting_ack_kind == ACK_KIND_TUN) {
    g_waiting_ack_kind = 0;
    Serial.println("$WACK,ERR,TUN,TIMEOUT");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 User-Side-01");

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

void loop() {
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
  checkAckTimeout();
}

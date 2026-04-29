/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

//uint8_t remote_side_Address[] = {0x94, 0xa9, 0x90, 0x30, 0xab, 0xc0}; // ESP32-S3 DevKitC-1 remote-side
uint8_t remote_side_Address[] = {0x10, 0x20, 0xba, 0x4c, 0x53, 0xfc}; // ESP32-S3 DevKitC-1 remote-side
// Structure example to send data
// Must match the receiver structure
typedef struct receivedfromremoteside {
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
} receivedfromremoteside;

// Create a struct_message called myData
receivedfromremoteside myReceivedFromremoteSideData;

// callback function that will be executed when data is receivedfromremoteside
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myReceivedFromremoteSideData, incomingData, sizeof(myReceivedFromremoteSideData));
  // Print CSV 15 kolom (raw fixed-point) agar sama format dengan generator
  Serial.print(myReceivedFromremoteSideData.timestamp, 3); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.latitude, 6); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.longitude, 6); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.speedMps); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.Calc_deg_servo_1); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.Calc_deg_servo_2); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.roll); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.pitch); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.yaw); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.zigzag_yaw); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.rpm_prop_1); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.rpm_prop_2); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.battery_1); Serial.print(",");
  Serial.print(myReceivedFromremoteSideData.battery_2); Serial.print(",");
  Serial.println(myReceivedFromremoteSideData.mode_auto);
} 

 // Must match the receiver structure
typedef struct send_to_remote_side {
  char a[32];
  int b;
  float c;
  bool d;
} send_to_remote_side;

// Create a struct_message called myData
send_to_remote_side mysend_to_remote_sideData;

esp_now_peer_info_t peerInfo;

// // callback when data is send_to_remote_side
// void OnDatasend_to_remote_side(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("\r\nLast Packet Send Status:\t");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("ESP32-S3 No 1 User-Side");
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register peer
  memcpy(peerInfo.peer_addr, remote_side_Address, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}
 
// =====================================================================
// Serial command parser untuk parameter dari Python (Local Monitor Dashboard)
// Format yang diharapkan (diakhiri newline):
//   $PARAM,<a>,<b>,<c>,<d>
// dengan tipe:
//   a : char[32]  (string, max 31 karakter; boleh berisi koma)
//   b : int       (parsed sebagai long, lalu di-cast ke int)
//   c : float
//   d : bool      ("true"/"false" / "1"/"0", case-insensitive)
// Balasan ke host:
//   $PACK,OK                 -> berhasil dikirim via ESP-NOW
//   $PACK,ERR,<reason>       -> validasi/transmit gagal
// =====================================================================

// strict integer parser: seluruh token harus angka (boleh tanda)
static bool parseIntStrict(const String& s, long &out) {
  String t = s; t.trim();
  if (t.length() == 0) return false;
  char* endp = nullptr;
  long v = strtol(t.c_str(), &endp, 10);
  if (endp == t.c_str() || endp == nullptr || *endp != '\0') return false;
  out = v;
  return true;
}

// strict float parser
static bool parseFloatStrict(const String& s, float &out) {
  String t = s; t.trim();
  if (t.length() == 0) return false;
  char* endp = nullptr;
  float v = strtof(t.c_str(), &endp);
  if (endp == t.c_str() || endp == nullptr || *endp != '\0') return false;
  out = v;
  return true;
}

// bool parser: terima true/false/1/0 (case-insensitive)
static bool parseBoolStrict(const String& s, bool &out) {
  String t = s; t.trim(); t.toLowerCase();
  if (t == "true"  || t == "1") { out = true;  return true; }
  if (t == "false" || t == "0") { out = false; return true; }
  return false;
}

static void processSerialLine(const String& line) {
  if (!line.startsWith("$PARAM,")) {
    return;  // ignore non-parameter lines (jaga stream CSV tetap bersih)
  }
  // payload setelah prefix "$PARAM,"
  String payload = line.substring(7);

  // Cari 3 koma terakhir sebagai delimiter agar field 'a'
  // (char[32]) boleh mengandung koma.
  int c3 = payload.lastIndexOf(',');
  int c2 = (c3 > 0) ? payload.lastIndexOf(',', c3 - 1) : -1;
  int c1 = (c2 > 0) ? payload.lastIndexOf(',', c2 - 1) : -1;
  if (c1 < 0 || c2 < 0 || c3 < 0) {
    Serial.println("$PACK,ERR,FORMAT");
    return;
  }

  String sa = payload.substring(0, c1);
  String sb = payload.substring(c1 + 1, c2);
  String sc = payload.substring(c2 + 1, c3);
  String sd = payload.substring(c3 + 1);

  // Validasi panjang 'a' agar muat di char[32] (sisain 1 byte untuk '\0')
  if (sa.length() >= sizeof(mysend_to_remote_sideData.a)) {
    Serial.println("$PACK,ERR,A_TOO_LONG");
    return;
  }

  long bVal;
  float cVal;
  bool dVal;
  if (!parseIntStrict(sb, bVal))   { Serial.println("$PACK,ERR,B_NOT_INT");   return; }
  if (!parseFloatStrict(sc, cVal)) { Serial.println("$PACK,ERR,C_NOT_FLOAT"); return; }
  if (!parseBoolStrict(sd, dVal))  { Serial.println("$PACK,ERR,D_NOT_BOOL");  return; }

  // Range check 'b' agar pas di tipe int (ESP32 = 32-bit int)
  if (bVal < INT32_MIN || bVal > INT32_MAX) {
    Serial.println("$PACK,ERR,B_OUT_OF_RANGE");
    return;
  }

  // Isi struct sesuai tipe yang dideklarasikan
  strncpy(mysend_to_remote_sideData.a, sa.c_str(), sizeof(mysend_to_remote_sideData.a) - 1);
  mysend_to_remote_sideData.a[sizeof(mysend_to_remote_sideData.a) - 1] = '\0';
  mysend_to_remote_sideData.b = (int)bVal;
  mysend_to_remote_sideData.c = cVal;
  mysend_to_remote_sideData.d = dVal;

  // Kirim sekali via ESP-NOW (event-driven, bukan periodik)
  esp_err_t result = esp_now_send(
      remote_side_Address,
      (uint8_t *)&mysend_to_remote_sideData,
      sizeof(mysend_to_remote_sideData));

  if (result == ESP_OK) {
    Serial.println("$PACK,OK");
  } else {
    Serial.println("$PACK,ERR,SEND_FAIL");
  }
}

void loop() {
  // Non-blocking serial reader: kumpulkan byte sampai '\n', lalu parse.
  static String rxBuf;
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      processSerialLine(rxBuf);
      Serial.println("Data sent to remote side");
      rxBuf = "";
    } else {
      // Guard panjang buffer untuk antisipasi input rusak/runaway
      if (rxBuf.length() < 200) {
        rxBuf += ch;
      } else {
        rxBuf = "";
        Serial.println("$PACK,ERR,LINE_TOO_LONG");
      }
    }
  }
}
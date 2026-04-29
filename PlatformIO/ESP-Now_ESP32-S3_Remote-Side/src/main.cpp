#include <Arduino.h>

/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <esp_now.h>
#include <WiFi.h>

//uint8_t user_side_Address[] = {0x10, 0x20, 0xba, 0x4c, 0x53, 0xfc}; // ESP32-S3 DevKitC-1 user-side
uint8_t user_side_Address[] = {0x94, 0xa9, 0x90, 0x30, 0xab, 0xc0}; // ESP32-S3 DevKitC-1 user-side
// Structure example to send data
// Must match the receiver structure
typedef struct receivedfromuserside {
  char a[32];
  int b;
  float c;
  bool d;
} receivedfromuserside;

// Create a struct_message called myData
receivedfromuserside myReceivedFromUserSideData;

// callback function that will be executed when data is receivedfromuserside
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myReceivedFromUserSideData, incomingData, sizeof(myReceivedFromUserSideData));
  Serial.print("Bytes received from User Side: ");
  Serial.println(len);
  Serial.print("Char from User Side: ");
  Serial.println(myReceivedFromUserSideData.a);
  Serial.print("Int from User Side: ");
  Serial.println(myReceivedFromUserSideData.b);
  Serial.print("Float from User Side: ");
  Serial.println(myReceivedFromUserSideData.c);
  Serial.print("Bool: ");
  Serial.println(myReceivedFromUserSideData.d);
  Serial.println();
} 

 // Must match the receiver structure
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

// Create a struct_message called myData
send_to_user_side mysend_to_user_sideData;

esp_now_peer_info_t peerInfo;

// // callback when data is send_to_user_side
// void OnDatasend_to_user_side(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("\r\nLast Packet Send Status:\t");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("ESP32-S3 No 1 Remote-Side");
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register peer
  memcpy(peerInfo.peer_addr, user_side_Address, 6);
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
 
void loop() {
  // Set values to send (mengikuti format Generate_15_random_data_ASCII)
  mysend_to_user_sideData.timestamp = millis() / 1000.0;
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
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(user_side_Address, (uint8_t *) &mysend_to_user_sideData, sizeof(mysend_to_user_sideData));
   
  // if (result == ESP_OK) {
  //   Serial.println("send_to_user_side with success");
  // }
  // else {
  //   Serial.println("Error sending the data");
  // }
  delay(2000);
}
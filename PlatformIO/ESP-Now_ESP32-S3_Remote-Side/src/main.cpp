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
  Serial.print("Bytes receivedfromuserside from User Side: ");
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
  char a[32];
  int b;
  float c;
  bool d;
} send_to_user_side;

// Create a struct_message called myData
send_to_user_side mysend_to_user_sideData;

esp_now_peer_info_t peerInfo;

// callback when data is send_to_user_side
void OnDatasend_to_user_side(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
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
  // Set values to send
  strcpy(mysend_to_user_sideData.a, "THIS IS A CHAR");
  mysend_to_user_sideData.b = random(1,20);
  mysend_to_user_sideData.c = 1.2;
  mysend_to_user_sideData.d = false;
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(user_side_Address, (uint8_t *) &mysend_to_user_sideData, sizeof(mysend_to_user_sideData));
   
  if (result == ESP_OK) {
    Serial.println("send_to_user_side with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(2000);
}
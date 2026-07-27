#include <Arduino.h>
#include <math.h>

#pragma pack(1)  // Disable struct padding
struct DatatoSend {
  double timestamp;       // detik sejak boot (millis()/1000.0)
  double latitude;        // latitude
  double longitude;       // longitude
  int16_t speedMps;       // kecepatan (m/s) × 100 (0..350 untuk 0.00..3.50)
  int16_t Calc_deg_servo_1; // derajat hasil kalkulasi feedback servo 1 × 100 (-4000..4000 untuk -40.00..40.00)
  int16_t Calc_deg_servo_2; // derajat hasil kalkulasi feedback servo 2 × 100 (-4000..4000 untuk -40.00..40.00)
  int16_t roll;           // derajat roll × 100 (-18000..18000 untuk -180..180)
  int16_t pitch;          // derajat pitch × 100 (-18000..18000 untuk -180..180)
  int16_t yaw;            // derajat yaw × 100 (0..36000 untuk 0..360)
  int16_t zigzag_yaw;     // derajat zigzag yaw × 100 (-3500..3500 untuk -35..35)
  int16_t rpm_prop_1;     // rpm motor propeller 1 (integer 0..30000)
  int16_t rpm_prop_2;     // rpm motor propeller 2 (integer 0..30000)
  int16_t battery_1;      // tegangan batere 1 (V) × 100 (1000..1259 untuk 10.00..12.59)
  int16_t battery_2;      // tegangan batere 2 (V) × 100 (1000..1259 untuk 10.00..12.59)
  uint8_t mode_auto;      // mode auto (0: manual, 1: turning left, 2: turning right, 3: zigzag 10, 4: zigzag 20)
};
#pragma pack()  // Reset to default packing
// Total size: 3*8 (double) + 11*2 (int16_t) + 1*1 (uint8_t) = 24 + 22 + 1 = 47 bytes
DatatoSend data;

unsigned long lastPrintTime = 0;
// const unsigned long INTERVAL = 100; // 100 milliseconds
// const unsigned long INTERVAL = 10; // 10 milliseconds ~ 100 data per second
const unsigned long INTERVAL = 5; // 5 milliseconds ~ 200 data per second

void setup() {
  // put your setup code here, to run once:
  Serial.begin(230400);  // Increased to support 200 data/sec (5ms interval)
  
  // Verify struct size (should be 49 bytes)
  // Serial.print("Struct size: "); Serial.println(sizeof(DatatoSend));
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis();
  
  if (currentTime - lastPrintTime >= INTERVAL) {
        
    // Calculate and store timestamp
    data.timestamp = (double)currentTime / 1000.0;  // timestamp detik
    lastPrintTime = currentTime;
    
    // Generate data sesuai tipe pada DatatoSend
    // Latitude/Longitude sekitar referensi, variasi hanya pada digit ke-5 dan ke-6
    const double REF_LAT = -7.286692;
    const double REF_LON = 112.796092;
    // Gerak spiral meniru Python: angle += 45 deg tiap 2 detik, radius += 0.0002 tiap 2 detik
    // Dengan INTERVAL=100ms -> angle_step = 2.25 deg per langkah, radius_step = 0.00001 per langkah
    static double angleDeg = 0.0;
    static double radius = 0.001; // awal sama seperti Python
    const double ANGLE_STEP = 2.25;      // derajat per 100ms
    const double RADIUS_STEP = 0.00001;  // per 100ms

    angleDeg += ANGLE_STEP;
    if (angleDeg >= 360.0) {
      angleDeg -= 360.0;
      radius = 0.001; // reset radius setiap satu putaran penuh
    }
    radius += RADIUS_STEP;

    double angleRad = angleDeg * M_PI / 180.0;
    // Samakan pusat dengan Python
    const double BASE_LAT = -7.281500;
    const double BASE_LON = 112.798900;

    double spiral_lat = BASE_LAT + (radius * cos(angleRad));
    double spiral_lon = BASE_LON + (radius * sin(angleRad));

    // Noise acak ±0.0005
    double random_lat_offset = (double) random(-500, 501) / 1000000.0;
    double random_lon_offset = (double) random(-500, 501) / 1000000.0;

    data.latitude  = spiral_lat + random_lat_offset;
    data.longitude = spiral_lon + random_lon_offset;
    // Heading/yaw akan di-set setelah ini (×100 untuk int16_t)

    // Kecepatan 0.00..3.50 m/s (×100 untuk int16_t)
    data.speedMps = (int16_t)random(0L, 351L);

    // Sudut servo sekitar -40.00..40.00 derajat (×100 untuk int16_t)
    data.Calc_deg_servo_1 = (int16_t)random(-4000, 4001);
    data.Calc_deg_servo_2 = (int16_t)random(-4000, 4001);

    // Roll dan pitch -180..180 (×100 untuk int16_t)
    data.roll  = (int16_t)random(-18000, 18001);
    data.pitch = (int16_t)random(-18000, 18001);
    
    // Yaw mengikuti arah spiral (×100 untuk int16_t)
    data.yaw = (int16_t)(fmod(angleDeg, 360.0) * 100.0);

    // Zigzag yaw -35..35 derajat (×100 untuk int16_t)
    data.zigzag_yaw = (int16_t)random(-3500, 3501);

    // RPM propeller 0..30000 (integer, tidak perlu skala)
    data.rpm_prop_1 = (int16_t)random(0L, 30001L);
    data.rpm_prop_2 = (int16_t)random(0L, 30001L);

    // Tegangan batere 10.00..12.59 V (×100 untuk int16_t)
    data.battery_1 = (int16_t)random(1000, 1260);
    data.battery_2 = (int16_t)random(1000, 1260);

    // Mode auto 0..4 (0: manual, 1: turning left, 2: turning right, 3: zigzag 10, 4: zigzag 20)
    data.mode_auto = (uint8_t)random(0, 5);
    
    // Kirim data sebagai ASCII text dengan format CSV (mengirim integer untuk efisiensi)
    Serial.print(data.timestamp, 3);          // timestamp (3 desimal) - tetap float karena double
    Serial.print(",");
    Serial.print(data.latitude, 6);           // latitude (6 desimal) - tetap float karena double
    Serial.print(",");
    Serial.print(data.longitude, 6);          // longitude (6 desimal) - tetap float karena double
    Serial.print(",");
    Serial.print(data.speedMps);              // speedMps (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.Calc_deg_servo_1);      // Calc_deg_servo_1 (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.Calc_deg_servo_2);      // Calc_deg_servo_2 (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.roll);                  // roll (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.pitch);                 // pitch (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.yaw);                   // yaw (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.zigzag_yaw);            // zigzag_yaw (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.rpm_prop_1);            // rpm_prop_1 (integer)
    Serial.print(",");
    Serial.print(data.rpm_prop_2);            // rpm_prop_2 (integer)
    Serial.print(",");
    Serial.print(data.battery_1);             // battery_1 (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.battery_2);             // battery_2 (integer × 100, lebih hemat)
    Serial.print(",");
    Serial.print(data.mode_auto);             // mode_auto (integer)
    Serial.println();                         // newline untuk memisahkan setiap baris data

  }
}

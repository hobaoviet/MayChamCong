#include <Adafruit_Fingerprint.h>

Adafruit_Fingerprint finger(&Serial2);

void setup() {
  Serial.begin(115200);
  Serial2.begin(57600, SERIAL_8N1, 16, 17); // ⚠️ đổi lại RX/TX đúng với phần cứng bạn dùng

  Serial.println("Testing AS608...");

  delay(1000);
  if (finger.verifyPassword()) {
    Serial.println("✅ Sensor found & verified!");
  } else {
    Serial.println("❌ Sensor not found or wrong password!");
    while (1) delay(100);
  }
}

void loop() {
  // không cần gì thêm
}

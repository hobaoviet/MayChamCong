#include "..\src\A1_HAL\HAL.hpp"

void setup() {
  Serial.begin(115200);
  Serial.println("=== TEST A1_HAL ===");

  // 1. Khởi tạo toàn bộ phần cứng
  hal::initAll();
  Serial.println("HAL init done");

  // 2. Test Relay
  Serial.println("Test Relay: ON 1s, OFF 1s");
  hal::relayOn();
  delay(1000);
  hal::relayOff();
  delay(1000);

  // 3. Test Buzzer
  Serial.println("Test Buzzer: Beep 500ms");
  hal::buzzerOn();
  delay(500);
  hal::buzzerOff();
  delay(500);

  // 4. Test LCD
  Serial.println("Test LCD: Hello World");
  hal::lcdShow("Hello", "World!");

  // 5. Test RFID
  if (hal::rfid().PCD_PerformSelfTest()) {
    Serial.println("RFID module OK");
  } else {
    Serial.println("RFID module FAIL");
  }

  // 6. Test Fingerprint
  if (hal::finger().verifyPassword()) {
    Serial.println("Fingerprint sensor OK");
  } else {
    Serial.println("Fingerprint sensor FAIL");
  }
}

void loop() {
  // cập nhật beep pattern (nếu có)
  hal::beepUpdate();
}

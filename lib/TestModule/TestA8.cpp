#include <Arduino.h>
#include "..\src\A8_power\power.hpp"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TEST A8_power (Monitor only, no isBusy) ===");

  // 1. Cấu hình Power Manager
  power::Config cfg;
  cfg.idleTimeoutMs = 10000;             // sau 10s không hoạt động sẽ sleep
  cfg.mutePeripheralsBeforeSleep = true;
  cfg.ext1Mask = (1ULL << GPIO_NUM_25) | (1ULL << GPIO_NUM_26); 
  cfg.ext1Mode = ESP_EXT1_WAKEUP_ANY_HIGH;
  power::begin(cfg);

  Serial.printf("[POWER] Init done. Idle timeout = %lu ms\n", cfg.idleTimeoutMs);

  // 2. Feed activity ngay lúc boot
  Serial.println("[TEST] Feed activity at boot");
  power::feedActivity();
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.printf("[POWER] Tick at %lu ms\n", millis());
  }

  // 3. Sau 5s: setBusy(true)
  static bool busySet = false;
  if (millis() > 5000 && !busySet) {
    busySet = true;
    Serial.println("[TEST] Set busy mode (simulating enroll/operation)");
    power::setBusy(true);
  }

  // 4. Sau 8s: setBusy(false)
  static bool busyClear = false;
  if (millis() > 8000 && !busyClear) {
    busyClear = true;
    Serial.println("[TEST] Clear busy mode (done)");
    power::setBusy(false);
  }

  // 5. Gọi update() để quản lý auto-sleep
  power::update();

  // 6. Sau 15s, in log giả lập sleep
  if (millis() > 15000) {
    Serial.println("[TEST] Idle timeout reached → would go to sleep now!");
    delay(5000); // giữ log, không thực sự sleep
  }
}
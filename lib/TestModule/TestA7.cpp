#include "..\src\A7_door\door.hpp"
#include "../src/A1_HAL/HAL.hpp"
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TEST A7_door ===");

  // 1. Khởi tạo door (relay OFF)
  door::begin();

  // 2. Mở cửa 5 giây
  Serial.println("[TEST] Opening door for 5s...");
  door::open(5000);
  Serial.printf("State = %s\n",
                door::getState() == door::State::DOOR_OPEN ? "OPEN" : "CLOSED");
}

void loop() {
  // 3. Luôn gọi update() để auto-close hoạt động
  door::update();

  // 4. Sau 7 giây, mở cửa không hẹn giờ (chờ close() thủ công)
  static bool once = false;
  if (millis() > 7000 && !once) {
    once = true;
    Serial.println("[TEST] Opening door indefinitely (until close() called)...");
    door::open(0); // mở vô hạn
    Serial.printf("State = %s\n",
                  door::getState() == door::State::DOOR_OPEN ? "OPEN" : "CLOSED");
  }

  // 5. Sau 12 giây, đóng cửa thủ công
  static bool closed = false;
  if (millis() > 12000 && !closed) {
    closed = true;
    Serial.println("[TEST] Closing door manually...");
    door::close();
    Serial.printf("State = %s\n",
                  door::getState() == door::State::DOOR_OPEN ? "OPEN" : "CLOSED");
  }
}
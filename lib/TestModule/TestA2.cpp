#include "..\src\A2_KeypadMode\A2_keypad_mode.hpp"

void setup() {
  Serial.begin(115200);
  Serial.println("=== TEST A2_KeypadMode ===");

  // Khởi tạo keypad, debounce 50ms
  keypad_mode::begin(50);
}

void loop() {
  // Cập nhật quét keypad
  keypad_mode::update();

  // Đọc sự kiện từ hàng đợi
  keypad_mode::Event e;
  if (keypad_mode::consumeEvent(e)) {
    switch (e.type) {
      case keypad_mode::EventType::ModeChanged:
        Serial.printf("ModeChanged -> %s\n", keypad_mode::modeName(e.newMode));
        break;
      case keypad_mode::EventType::EnrollEmployee:
        Serial.println("Event: Enroll Employee (*)");
        break;
      case keypad_mode::EventType::DeleteEmployee:
        Serial.println("Event: Delete Employee (#)");
        break;
      case keypad_mode::EventType::StatusRequested:
        Serial.println("Event: Status Requested (C)");
        break;
      case keypad_mode::EventType::SleepRequested:
        Serial.println("Event: Sleep Requested (D)");
        break;
      case keypad_mode::EventType::RawKey:
        Serial.printf("RawKey pressed: %c\n", e.key);
        break;
      default:
        Serial.println("Unknown event");
        break;
    }
  }
}

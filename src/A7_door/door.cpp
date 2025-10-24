#include "door.hpp"

namespace door {

static State    st = State::DOOR_CLOSED;
static uint32_t tClose = 0; // millis khi phải đóng, 0 = không hẹn

void begin() {
  close(); // đảm bảo khởi động thì relay OFF (cửa đóng)
}

void open(uint32_t durationMs) {
  hal::relayOn();
  st = State::DOOR_OPEN;
  if (durationMs > 0) {
    tClose = millis() + durationMs; // hẹn giờ đóng
  } else {
    tClose = 0; // giữ mở đến khi close()
  }
  Serial.printf("[DOOR] Open (auto-close=%lu ms)\n", (unsigned long)durationMs);
}

void close() {
  hal::relayOff();
  st = State::DOOR_CLOSED;
  tClose = 0;
  Serial.println("[DOOR] Closed");
}

void update() {
  if (st == State::DOOR_OPEN && tClose > 0 && millis() >= tClose) {
    close(); // tự động đóng khi hết thời gian
  }
}

State getState() { return st; }

uint32_t scheduledCloseAt() { return tClose; }

} // namespace door



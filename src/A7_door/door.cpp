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


// #include "door.hpp"

// namespace door {

// static Config cfg_;
// static bool isOpen = false;
// static uint32_t openUntil = 0;

// void begin(const Config& cfg) {
//   cfg_ = cfg;
//   pinMode(cfg_.pin, OUTPUT);

//   // đảm bảo cửa đóng khi khởi động
//   close();
//   Serial.printf("[DOOR] Initialized at pin %u (activeLow=%d)\n", cfg_.pin, cfg_.activeLow);
// }

// void open(uint32_t ms) {
//   if (cfg_.activeLow) {
//     digitalWrite(cfg_.pin, LOW);   // active
//   } else {
//     digitalWrite(cfg_.pin, HIGH);  // active
//   }

//   isOpen = true;
//   openUntil = millis() + ms;

//   Serial.printf("[DOOR] Open for %u ms\n", ms);
// }

// void close() {
//   if (cfg_.activeLow) {
//     digitalWrite(cfg_.pin, HIGH);  // inactive
//   } else {
//     digitalWrite(cfg_.pin, LOW);   // inactive
//   }

//   isOpen = false;
//   openUntil = 0;

//   Serial.println("[DOOR] Closed");
// }

// void update() {
//   if (isOpen && millis() > openUntil && openUntil > 0) {
//     close();
//   }
// }

// State getState() {
//   return isOpen ? State::DOOR_OPEN : State::DOOR_CLOSED;
// }

// } // namespace door
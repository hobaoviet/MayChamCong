// #pragma once
// /**
//  * ===========================================================
//  *  A7 — Door Manager (Relay control)
//  *  -----------------------------------------------------------
//  *  - Điều khiển relay cửa (active-low) qua hal::relayOn/Off
//  *  - Non-blocking: open(ms) sẽ tự đóng sau ms
//  *  - Có thể query trạng thái (OPEN/CLOSED)
//  * ===========================================================
//  */

// #include <Arduino.h>
// #include "../A1_HAL/HAL.hpp"

// namespace door {

// enum class State : uint8_t {  DOOR_CLOSED = 0, DOOR_OPEN = 1  };

// /// Khởi tạo (relay OFF để chắc chắn cửa đóng)
// void begin();

// /// Mở cửa. Nếu durationMs > 0 → sẽ auto-close sau durationMs.
// /// Nếu durationMs = 0 → cửa mở cho đến khi gọi close().
// void open(uint32_t durationMs = 0);

// /// Đóng cửa ngay lập tức.
// void close();

// /// Gọi trong loop() thường xuyên để xử lý auto-close.
// void update();

// /// Trạng thái hiện tại.
// State getState();

// /// Thời điểm millis() khi cửa sẽ tự đóng (0 = không hẹn giờ).
// uint32_t scheduledCloseAt();

// } // namespace door


#pragma once
#include <Arduino.h>
#include "../A1_HAL/HAL.hpp"

namespace door {

enum class State {
  DOOR_CLOSED,
  DOOR_OPEN
};

struct Config {
  uint8_t pin;            // GPIO của relay
  bool activeLow;         // true nếu relay kích bằng LOW

};

void begin();
void open(uint32_t ms = 5000);  // mở cửa trong ms mili-giây, default 5s
void close();                   // đóng cửa ngay
void update();                  // kiểm tra auto-close
State getState();

} // namespace door
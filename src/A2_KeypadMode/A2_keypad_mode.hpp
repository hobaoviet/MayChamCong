// #pragma once
// /**
//  * ===========================================================
//  *  A2 — Keypad & Mode Manager
//  *  -----------------------------------------------------------
//  *  - Non-blocking, dựa trên thư viện Keypad.h
//  *  - Gắn liền với sơ đồ pin từ hal.hpp
//  *  - Quản lý chế độ điểm danh: IN / OUT
//  *  - Sinh event: ModeChanged, StatusRequested, SleepRequested,
//  *    EnrollEmployee, DeleteEmployee, RawKey
//  * ===========================================================
//  */

// #include <Arduino.h>
// #include <Keypad.h>
// #include "../A1_HAL/HAL.hpp"

// namespace keypad_mode {

// // =====================
// // Các chế độ điểm danh
// // =====================
// enum class Mode : uint8_t { IN = 0, OUT = 1 };

// // =====================
// // Các loại sự kiện
// // =====================
// enum class EventType : uint8_t {
//   None = 0,
//   ModeChanged,      // đổi chế độ IN/OUT
//   StatusRequested,  // phím C
//   SleepRequested,   // phím D
//   EnrollEmployee,   // phím *
//   DeleteEmployee,   // phím #
//   RawKey            // phím số, nếu được bật
// };

// // =====================
// // Cấu trúc sự kiện
// // =====================
// struct Event {
//   EventType type {EventType::None};
//   char      key  {0};           // phím nhấn
//   Mode      newMode {Mode::IN}; // hợp lệ khi ModeChanged
//   uint32_t  t_ms {0};           // millis() tại thời điểm nhấn
// };

// // =====================
// // API chính
// // =====================

// /** Gọi trong setup(). debounceMs = thời gian chống dội (mặc định 35ms) */
// void begin(uint16_t debounceMs = 35);

// /** Gọi trong loop() liên tục để xử lý keypad */
// void update();

// /** Lấy chế độ hiện tại */
// Mode getMode();

// /** Đặt chế độ bằng code (emitEvent=true nếu muốn sinh event ModeChanged) */
// void setMode(Mode m, bool emitEvent = false);

// /** Kiểm tra và lấy 1 event từ queue, trả về true nếu có */
// bool consumeEvent(Event& out);

// /** Có event pending hay không */
// bool available();

// /** Cho phép nhận RawKey (0..9 * #). Mặc định tắt */
// void setRawKeyAllowed(bool enable);

// /** Tiện ích: tên chế độ */
// const char* modeName(Mode m);

// } // namespace keypad_mode




////////////////////////////////////////////////////////

#pragma once
/**
 * ===========================================================
 *  A2 — Keypad & Mode Manager (Final - ESP32 verified)
 *  -----------------------------------------------------------
 *  - Dùng thư viện Keypad.h
 *  - Quản lý mode: IN / OUT
 *  - Sinh event: ModeChanged, Enroll, Delete, Sleep, Status
 *  - Có queue FIFO lưu sự kiện
 * ===========================================================
 */

#include <Arduino.h>
#include <Keypad.h>
#include "../A1_HAL/HAL.hpp"

namespace keypad_mode {

// =====================================================
// ENUM: Chế độ điểm danh (IN / OUT)
// =====================================================
enum class Mode : uint8_t {
  IN = 0,
  OUT = 1
};

// =====================================================
// ENUM: Loại sự kiện
// =====================================================
enum class EventType : uint8_t {
  None = 0,
  ModeChanged,      // A/B → đổi mode
  StatusRequested,  // C → yêu cầu trạng thái
  SleepRequested,   // D → yêu cầu sleep
  EnrollEmployee,   // * → đăng ký nhân viên mới
  DeleteEmployee,   // # → xóa nhân viên
  RawKey            // Các phím 0–9, nếu bật cho phép
};

// =====================================================
// STRUCT: Sự kiện (ghi lại hành động từ bàn phím)
// =====================================================
struct Event {
  EventType type {EventType::None};  // loại sự kiện
  char      key  {0};                // ký tự phím nhấn
  Mode      newMode {Mode::IN};      // mode mới (nếu có)
  uint32_t  t_ms {0};                // thời điểm millis() khi nhấn
};

// =====================================================
// API chính
// =====================================================

/**
 * @brief Khởi tạo keypad, thiết lập debounce và pin.
 * @param debounceMs Thời gian chống dội phím (mặc định 35 ms)
 */
void begin(uint16_t debounceMs = 35);

/**
 * @brief Gọi liên tục trong loop() để đọc keypad.
 */
void update();

/**
 * @brief Trả về chế độ hiện tại (IN / OUT).
 */
Mode getMode();

/**
 * @brief Đặt chế độ bằng tay (emitEvent = true nếu muốn sinh event ModeChanged).
 */
void setMode(Mode m, bool emitEvent = false);

/**
 * @brief Lấy 1 event từ queue, trả về true nếu có.
 */
bool consumeEvent(Event& out);

/**
 * @brief Kiểm tra xem có event chờ xử lý không.
 */
bool available();

/**
 * @brief Bật/tắt cho phép nhận các phím số (RawKey 0–9, * hoặc #).
 */
void setRawKeyAllowed(bool enable);

/**
 * @brief Trả về tên dạng chữ của mode (“IN” hoặc “OUT”).
 */
const char* modeName(Mode m);

} // namespace keypad_mode
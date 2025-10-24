// #pragma once
// /**
//  * ===========================================================
//  *  A4 — RFID Engine (MFRC522 / SPI)
//  *  -----------------------------------------------------------
//  *  - Non-blocking đọc thẻ RFID qua hal::rfid()
//  *  - UID → user_id dạng HEX string "RFID_xxxx"
//  *  - Có cooldown (ms) để tránh spam liên tục
//  * ===========================================================
//  */

// #include <Arduino.h>
// #include "../A1_HAL/HAL.hpp"

// namespace rfid_engine {

// // Kết quả xác thực RFID
// struct AuthResult {
//   bool    hasResult {false};
//   bool    ok {false};          // true = đọc thẻ thành công
//   String  user_id;             // ví dụ "RFID_1234ABCD"
//   String  uid;                 // UID thô dạng HEX (không prefix RFID_)
//   String  reason;              // "no_card", "read_error", ...
//   uint32_t t_ms {0};
// };

// // =====================
// // API
// // =====================

// /** Khởi tạo (đã init SPI trong hal::initAll, chỉ reset lại MFRC522) */
// void begin();

// /** Poll non-blocking, trả về AuthResult khi có thẻ mới */
// AuthResult poll();

// /** Đặt thời gian cooldown (ms). Mặc định 2000 ms */
// void setCooldown(uint32_t ms);

// /** Hàm tiện ích: đọc UID thẻ hiện tại, trả về HEX string hoặc "" nếu không có */
// String getUIDString();
// // bổ sung 


// } // namespace rfid_engine

//////////////////////////////////////////////////////////////////////

#pragma once
/**
 * ===========================================================
 *  A4 — RFID Engine (MFRC522 / SPI)
 *  -----------------------------------------------------------
 *  - Non-blocking đọc thẻ RFID qua hal::rfid()
 *  - UID → user_id dạng HEX string "RFID_xxxx"
 *  - Có cooldown (ms) để tránh spam liên tục
 * ===========================================================
 */

#include <Arduino.h>
#include "../A1_HAL/HAL.hpp"

namespace rfid_engine
{

  // Kết quả xác thực RFID
  struct AuthResult
  {
    bool hasResult{false};
    bool ok{false}; // true = đọc thẻ thành công
    String user_id; // ví dụ "RFID_1234ABCD"
    String uid;     // UID thô dạng HEX (không prefix RFID_)
    String reason;  // "no_card", "read_error", ...
    uint32_t t_ms{0};
  };

  // =====================
  // API
  // =====================

  /** Khởi tạo (đã init SPI trong hal::initAll, chỉ reset lại MFRC522) */
  void begin();

  /** Poll non-blocking, trả về AuthResult khi có thẻ mới */
  AuthResult poll();

  /** Đặt thời gian cooldown (ms). Mặc định 2000 ms */
  void setCooldown(uint32_t ms);

  /** Hàm tiện ích: đọc UID thẻ hiện tại, trả về HEX string hoặc "" nếu không có */
  String getUIDString();
  // bổ sung
  void ignoreFor(uint32_t ms);

} // namespace rfid_engine

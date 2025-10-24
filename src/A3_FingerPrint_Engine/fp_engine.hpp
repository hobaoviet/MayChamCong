// #pragma once
// /**
//  * ===========================================================
//  *  A3 — Fingerprint Engine (AS608 / UART2) + ENROLL
//  *  -----------------------------------------------------------
//  *  - Xác thực: poll() non-blocking → AuthResult
//  *  - Đăng ký (enroll): state machine 2 lần quét non-blocking
//  *    + startEnroll(slotId) → updateEnroll() → (OK/FAIL/CANCELLED/TIMEOUT)
//  * ===========================================================
//  */

// #include <Arduino.h>
// #include "../A1_HAL/HAL.hpp"

// namespace fp_engine {

// // ====== AUTH (đã có) ======
// struct AuthResult {
//   bool    hasResult {false};
//   bool    ok {false};
//   int     fingerId {-1};        // ID của vân tay (slot)
//   int     confidence {-1};
//   String  user_id;              // "FP_<ID>" hoặc "UNKNOWN"
//   String  reason;               // "not_found", "image_error", ...
//   uint32_t t_ms {0};
// };

// // ====== ENROLL ======
// enum class EnrollState : uint8_t {
//   Idle = 0,
//   WaitFinger1,        // chờ đặt ngón lần 1
//   Capture1,           // getImage lần 1
//   Image2Tz1,          // image2Tz(buffer#1)
//   RemoveFinger1,      // yêu cầu nhấc ngón (đảm bảo lần 2 là ảnh khác)
//   WaitFinger2,        // chờ đặt ngón lần 2
//   Capture2,           // getImage lần 2
//   Image2Tz2,          // image2Tz(buffer#2)
//   CreateModel,        // tạo model từ buffer1+buffer2
//   StoreModel,         // lưu vào slot
//   DoneOK,             // hoàn tất OK
//   DoneFail,           // lỗi
//   Cancelled,          // user cancel
//   Timeout             // quá thời gian cho phép
// };

// struct EnrollStatus {
//   bool         changed {false};    // true nếu state thay đổi để UI cập nhật
//   EnrollState  state {EnrollState::Idle};
//   uint16_t     slot {0};           // slot đang đăng ký
//   String       message;            // mô tả ngắn cho UI
//   String       reason;             // nếu lỗi
// };

// // ====== AUTH API ======
// bool       begin();
// AuthResult poll();                 // non-blocking auth
// void       setPollInterval(uint32_t ms);
// /** Blocking verify tiện cho test: trả về fingerId hoặc -1 */
// int verifyFingerprint();

// // ====== ENROLL API ======
// /** Bắt đầu quy trình ENROLL cho slot (1..N). Trả về false nếu đang bận hoặc slot không hợp lệ */
// bool startEnroll(uint16_t slotId);
// /** Cập nhật state machine ENROLL. Gọi thường xuyên trong loop(). */
// EnrollStatus updateEnroll();
// /** Hủy quy trình ENROLL hiện tại (nếu đang chạy). */
// void cancelEnroll();
// /** Có đang trong ENROLL không? */
// bool isEnrolling();
// /** Timeout mỗi giai đoạn (ms). Mặc định 20s mỗi bước chờ. */
// void setEnrollStepTimeout(uint32_t ms);
// /** Kiểm tra tổng số template đang có (nếu sensor hỗ trợ). Trả -1 nếu lỗi. */

// // int16_t templateCount();

// bool deleteFingerprint(uint16_t id);

// // bổ sung thêm

// // int findFreeId(int maxSlots = 200); // AS608 hỗ trợ tối đa ~200 mẫu

// // dùng để test

// Adafruit_Fingerprint& finger();

// bool verifySensor();

// } // namespace fp_engine

#pragma once
#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include "..\src\A1_HAL\HAL.hpp"

namespace fp_engine
{

  // ===============================
  // ENUM STATE – mô tả quá trình ENROLL
  // ===============================
  enum class EnrollState
  {
    Idle,
    WaitFinger1,
    Capture1,
    RemoveFinger1,
    WaitFinger2,
    Capture2,
    CreateModel,
    StoreModel,
    DoneOK,
    DoneFail,
    Cancelled,
    Timeout
  };

  // ===============================
  // CẤU TRÚC DỮ LIỆU KHI NHẬN DẠNG
  // ===============================
  struct AuthResult
  {
    bool hasResult = false; // true khi có kết quả
    bool ok = false;        // true nếu tìm thấy vân tay hợp lệ
    int fingerId = -1;      // ID trong database
    int confidence = -1;    // độ tin cậy (0–255)
    String user_id = "";    // tên user hoặc mã ID
    String reason = "";     // mô tả lỗi nếu có
    uint32_t t_ms = 0;      // thời điểm xảy ra
  };

  // ===============================
  // CẤU TRÚC DỮ LIỆU KHI ENROLL
  // ===============================
  struct EnrollStatus
  {
    bool changed = false; // true nếu trạng thái thay đổi
    EnrollState state;    // trạng thái hiện tại
    uint16_t slot = 0;    // ID đang lưu
    String message = "";  // thông báo hiện tại
    String reason = "";   // nếu fail
  };

  // ===============================
  // HÀM KHỞI TẠO / QUẢN LÝ SENSOR
  // ===============================

  /**
   * @brief Khởi tạo cảm biến AS608.
   * @return true nếu kết nối & verify password thành công.
   */
  bool begin();

  /**
   * @brief Xác minh cảm biến đang hoạt động (verify password).
   */
  bool verifySensor();

  /**
   * @brief Lấy đối tượng Adafruit_Fingerprint để dùng trực tiếp (nếu cần).
   */
  Adafruit_Fingerprint &finger();

  // ===============================
  // HÀM NHẬN DẠNG (AUTHENTICATION)
  // ===============================

  /**
   * @brief Gọi định kỳ trong loop() để phát hiện và nhận dạng vân tay.
   * @return AuthResult chứa kết quả (hasResult==true khi có sự kiện mới).
   */
  AuthResult poll();

  /**
   * @brief Xác minh vân tay kiểu blocking (chờ đến khi có kết quả).
   * @return ID nếu hợp lệ, -1 nếu không khớp hoặc lỗi.
   */
  int verifyFingerprint();

  /**
   * @brief Đặt khoảng thời gian giữa 2 lần poll() (mặc định 100ms).
   */
  void setPollInterval(uint32_t ms);

  // ===============================
  // HÀM ENROLL (ĐĂNG KÝ MẪU MỚI)
  // ===============================

  /**
   * @brief Bắt đầu quá trình đăng ký vân tay mới.
   *
   * @param slotId ID slot muốn lưu.
   *        Nếu truyền 0, module sẽ tự động chọn ID trống đầu tiên.
   * @return true nếu bắt đầu thành công, false nếu đang bận hoặc hết chỗ.
   */
  bool startEnroll(uint16_t slotId = 0);

  /**
   * @brief Cập nhật trạng thái enroll (gọi liên tục trong loop()).
   *
   * @return EnrollStatus chứa thông tin state, message, reason, ...
   */
  EnrollStatus updateEnroll();

  /**
   * @brief Hủy quá trình enroll hiện tại.
   */
  void cancelEnroll();

  /**
   * @brief Kiểm tra xem có đang trong quá trình enroll hay không.
   */
  bool isEnrolling();

  /**
   * @brief Đặt timeout cho từng bước enroll (mặc định 20 giây).
   */
  void setEnrollStepTimeout(uint32_t ms);

  // ===============================
  // HÀM XÓA MẪU
  // ===============================

  /**
   * @brief Xóa mẫu vân tay theo ID.
   * @param id Slot ID muốn xóa.
   * @return true nếu xóa thành công.
   */
  bool deleteFingerprint(uint16_t id);
  void clearAllFingerprints();
} // namespace fp_engine
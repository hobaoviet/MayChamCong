#pragma once
/**
 * ===========================================================
 *  B1 — Local Log Queue (SPIFFS JSONL)
 *  -----------------------------------------------------------
 *  - Lưu sự kiện vào file /logs.jsonl (append)
 *  - Cho phép đọc từng bản ghi từ đầu (peek), đánh dấu pop (đã gửi)
 *  - Có thêm tiện ích logAttendance() và logManagement()
 * ===========================================================
 */

#include <Arduino.h>

namespace log_queue
{

  // Loại log
  enum class LogType : uint8_t
  {
    Attendance = 0,      // sự kiện chấm công
    ManagementAdd = 1,   // thêm nhân viên
    ManagementDelete = 2 // xóa nhân viên
  };

  struct Entry
  {
    uint64_t ts;   // timestamp (millis hoặc epoch nếu có NTP)
    String uid;    // raw RFID UID (HEX) hoặc ""
    int fp_id;     // Fingerprint slot hoặc -1
    String mode;   // "IN" / "OUT" (chỉ cho Attendance)
    bool ok;       // thành công/thất bại
    String reason; // lý do / mô tả thêm
    LogType type;  // loại log
    String data;
  };

  // =====================
  // API chính
  // =====================

  // Init: mount SPIFFS (nếu chưa), tạo file nếu chưa có
  bool begin(const char *path = "/logs.jsonl");

  /// Push 1 log (append cuối file). Trả về true nếu OK
  bool push(const Entry &e);

  /// Peek 1 log đầu tiên chưa gửi. Trả false nếu hết
  bool peek(Entry &e);

  /// Pop (xóa log đầu tiên). Thực hiện bằng cách copy file còn lại
  bool pop();

  /// Số bản ghi trong file (đếm nhanh)
  size_t count();

  /// Xóa sạch queue
  bool clear();

  // =====================
  // API tiện ích
  // =====================

  /// Log sự kiện chấm công
  bool logAttendance(const String &uid, int fp_id, const String &mode, bool ok, const String &reason);

  /// Log sự kiện quản lý nhân viên (thêm/xóa)
  // bool logManagement(const String& uid, int fp_id, bool ok, const String& reason);
  /// Log sự kiện thêm nhân viên
  bool logManagementAdd(const String &uid, int fp_id, bool ok, const String &reason);

  /// Log sự kiện xóa nhân viên
  bool logManagementDelete(const String &uid, int fp_id, bool ok, const String &reason);

} // namespace log_queue
#pragma once
/**
 * ===========================================================
 *  A5 — Identity Store (SPIFFS JSON) for ESP32 Attendance
 *  -----------------------------------------------------------
 *  - Lưu danh sách user ở file /users.json (SPIFFS)
 *  - Mỗi user chỉ cần: rfid_uid (HEX string) + fp_id (int)
 *  - Dùng cho kiểm tra chấm công và thêm/xoá nhân viên
 *
 *  JSON schema:
 *  [
 *    {"rfid_uid":"DEADBEEF","fp_id":12},
 *    {"rfid_uid":"A1B2C3D4","fp_id":23}
 *  ]
 * ===========================================================
 */

#include <Arduino.h>
#include <vector>

namespace identity_store {

struct User {
  String rfid_uid;   // UID HEX (uppercase, ví dụ "DEADBEEF")
  int    fp_id;      // Finger ID (slot trong AS608), -1 nếu không dùng
};

// =====================
// API
// =====================

/// Mount SPIFFS và load /users.json (nếu chưa có sẽ tạo rỗng).
bool begin();

/// Load toàn bộ từ file JSON vào RAM. Trả về số record đọc được, -1 nếu lỗi.
int  loadFromFlash();

/// Save danh sách hiện tại xuống file JSON. Trả về true nếu OK.
bool saveToFlash();

/// Xuất toàn bộ danh sách thành JSON string.
String toJsonAll();

/// Thêm nhân viên mới (rfid_uid + fp_id). Trả false nếu UID hoặc FP đã tồn tại.
bool addEmployee(const String& uid, int fp_id);

/// Xoá nhân viên theo UID. Trả true nếu xoá thành công.
bool removeByUid(const String& uid);

/// Kiểm tra tồn tại theo UID.
bool existsUid(const String& uid);

/// Kiểm tra tồn tại theo FingerID.
bool existsFinger(int fp_id);

/// Lấy FingerID từ UID (trả -1 nếu không có).
int getFingerByUid(const String& uid);

/// Lấy UID từ FingerID (trả "" nếu không có).
String getUidByFinger(int fp_id);

/// Lấy toàn bộ danh sách trong RAM.
const std::vector<User>& all();

/// Chuẩn hoá UID: chuyển về uppercase HEX, bỏ khoảng trắng.
String normalizeUID(String s);

/// Đổi file path cơ sở (mặc định "/users.json").
void setFilePath(const char* path);



} // namespace identity_store
#include <Arduino.h>
#include "esp_sleep.h"
#include "../A1_HAL/HAL.hpp"
#include "../A7_door/door.hpp"

namespace power {

// Hàm callback để quyết định có được phép ngủ không (ngoài các điều kiện mặc định).
// Trả về true nếu OK để sleep. Mặc định = nullptr (không có chặn ngoài).
using CanSleepCallback = bool (*)();

struct Config {
  // Nguồn đánh thức EXT1 (bitmask của GPIOs)
  // Mặc định: GPIO32 & GPIO33 (cột keypad) — ANY_HIGH
  uint64_t ext1Mask;                        // mak GPIO
  esp_sleep_ext1_wakeup_mode_t ext1Mode;    // mode wake-up
  uint32_t idleTimeoutMs;                   // ms trước khi sleep
  bool mutePeripheralsBeforeSleep = true; 
  // Có tắt LCD backlight và buzzer trước khi sleep không?
};

// ===== API chính =====

// Gọi trong setup()
void begin(const Config& cfg = Config{});

// Gọi khi có hoạt động (nhấn phím, quét thẻ/vân tay thành công/thất bại, publish xong...)
void feedActivity();

// Đặt lại idle timeout
void setIdleTimeout(uint32_t ms);

// Đăng ký/huỷ callback kiểm tra điều kiện sleep bên ngoài
void setCanSleepCallback(CanSleepCallback fn);

// Đặt/huỷ “bận” tạm thời (ví dụ: đang publish cloud). bBusy=true sẽ chặn sleep.
void setBusy(bool bBusy);

// Hẹn sleep sau ms kể từ hiện tại (ghi đè idleTimeout tạm thời)
void armSleepAfter(uint32_t ms);

// Huỷ hẹn sleep (để giữ máy thức)
void cancelSleep();

// Yêu cầu ngủ ngay lập tức (không đợi idle). Reason chỉ để log.
void goSleepNow(const char* reason = "manual");

// Gọi thường xuyên trong loop() để kiểm tra & vào sleep khi đủ điều kiện
void update();

// Trạng thái
bool     isArmed();              // đã hẹn ngủ chưa?
uint32_t timeToSleepAt();        // millis() khi sẽ ngủ (0 = không hẹn)
uint32_t lastActivityAt();       // millis() lần hoạt động gần nhất
uint32_t idleTimeout();          // ms timeout hiện tại

} // namespace power
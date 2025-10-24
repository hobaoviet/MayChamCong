#include "power.hpp"

namespace power {

static Config cfg_;
static uint32_t lastActivityMs = 0;
static uint32_t sleepAtMs = 0;          // 0 = không hẹn
static bool     busy_ = false;
static CanSleepCallback canSleepFn_ = nullptr;

static inline bool _basicCanSleep() {
  // Không ngủ nếu cửa còn mở
  if (door::getState() == door::State::DOOR_OPEN) return false;
  // Có thể bổ sung thêm điều kiện mặc định ở đây nếu cần
  return true;
}

static inline bool _canSleepNow() {
  if (busy_) return false;
  if (!_basicCanSleep()) return false;
  if (canSleepFn_ && !canSleepFn_()) return false;
  return true;
}

void begin(const Config& cfg) {
  cfg_ = cfg;
  lastActivityMs = millis();
  sleepAtMs = lastActivityMs + cfg_.idleTimeoutMs;
  busy_ = false;
  canSleepFn_ = nullptr;
}

void feedActivity() {
  lastActivityMs = millis();
  // Nếu chưa set thời điểm ngủ, đặt theo idleTimeout
  if (cfg_.idleTimeoutMs > 0) {
    sleepAtMs = lastActivityMs + cfg_.idleTimeoutMs;
  }
}

void setIdleTimeout(uint32_t ms) {
  cfg_.idleTimeoutMs = ms;
  // cập nhật lại lịch ngủ nếu đã vạch sẵn
  if (ms == 0) {
    sleepAtMs = 0; // tắt auto-sleep
  } else {
    sleepAtMs = millis() + ms;
  }
}

void setCanSleepCallback(CanSleepCallback fn) {
  canSleepFn_ = fn;
}

void setBusy(bool bBusy) {
  busy_ = bBusy;
  if (bBusy) {
    // Khi bận, huỷ hẹn ngủ tạm thời, sẽ hẹn lại khi feedActivity() hoặc hết bận
    sleepAtMs = 0;
  } else {
    // Khi hết bận, đặt lại hẹn ngủ từ bây giờ
    if (cfg_.idleTimeoutMs > 0) {
      sleepAtMs = millis() + cfg_.idleTimeoutMs;
    }
  }
}

void armSleepAfter(uint32_t ms) {
  sleepAtMs = millis() + ms;
}

void cancelSleep() {
  sleepAtMs = 0;
}

void goSleepNow(const char* reason) {
  Serial.printf("[POWER] Deep sleep NOW (%s)\n", reason ? reason : "");

  // In trạng thái COL để debug
  for (int i = 0; i < 4; i++) {
    int pin = hal::HAL_KEYPAD_COLS[i];
    Serial.printf("COL pin %d = %d\n", pin, digitalRead(pin));
  }
  // Tắt ngoại vi trước khi ngủ
  if (cfg_.mutePeripheralsBeforeSleep) {
    hal::buzzerOff();
    hal::lcdBacklight(false);
    door::close();
  }
  // Cấu hình wake EXT1

  delay(1000);  // cho người dùng nhả phím
  //esp_sleep_enable_ext1_wakeup(cfg_.ext1Mask, cfg_.ext1Mode);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
  esp_sleep_enable_ext1_wakeup(cfg_.ext1Mask, cfg_.ext1Mode);
  // (Tùy chọn) Bạn có thể thêm timer wake hoặc touch wake ở đây nếu cần
  esp_deep_sleep_start();
  // không bao giờ trở lại đây
}


void update() {
  // Nếu không bật auto-sleep
  if (cfg_.idleTimeoutMs == 0 && sleepAtMs == 0) return;

  uint32_t now = millis();

  // Nếu đang không hẹn -> thử đặt theo idle (trường hợp vừa feedActivity() xong)
  if (sleepAtMs == 0 && cfg_.idleTimeoutMs > 0) {
    sleepAtMs = lastActivityMs + cfg_.idleTimeoutMs;
  }

  // Chưa đến giờ
  if (sleepAtMs == 0 || now < sleepAtMs) return;

  // Đến giờ -> kiểm tra điều kiện
  if (_canSleepNow()) {
    goSleepNow("idle-timeout");
  } else {
    // Bị chặn -> lùi lịch ngủ thêm một nhịp idleTimeout để kiểm tra lại sau
    if (cfg_.idleTimeoutMs > 0) {
      sleepAtMs = now + cfg_.idleTimeoutMs;
    } else {
      sleepAtMs = 0;
    }
  }
}

bool isArmed()        { return sleepAtMs != 0; }
uint32_t timeToSleepAt(){ return sleepAtMs; }
uint32_t lastActivityAt(){ return lastActivityMs; }
uint32_t idleTimeout()  { return cfg_.idleTimeoutMs; }

} // namespace power
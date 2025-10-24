#pragma once
/**
 * ===========================================================
 *  A1 — HAL & Pin Map for ESP32 Attendance Project
 *  (AS608 + MFRC522 + Keypad + LCD + Buzzer + Relay)
 *  -----------------------------------------------------------
 *  - Định nghĩa sơ đồ chân tập trung, dễ tra cứu & thay đổi.
 *  - Cung cấp hàm khởi tạo phần cứng: hal::initAll()
 *  - Truy cập LCD/Buzzer/Relay/RFID/AS608 qua API gọn.
 *  - Beep pattern non-blocking (không chặn loop).
 *  - Self-test nhanh: LCD / Buzzer / Relay / RFID / Fingerprint.
 *  -----------------------------------------------------------
 *  Lưu ý:
 *   • Relay active-LOW (mặc định): LOW = ON (đóng mạch), HIGH = OFF.
 *   • LCD I2C addr mặc định 0x27; nếu màn hình bạn là 0x3F, đổi ở macro.
 *   • Thư viện cần: LiquidCrystal_I2C, MFRC522, Adafruit_Fingerprint.
 *   • ESP32 core: Wire/SPI/Preferences/…
 * ===========================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>

#ifndef ARDUINO_ARCH_ESP32
#  error "This HAL is dành cho ESP32 (Arduino core)."
#endif

namespace hal {



// ============================
// 1) PIN MAP (CÓ THỂ ĐIỀU CHỈNH)
// ============================

// I2C cho LCD 16x2
#ifndef HAL_LCD_I2C_ADDR
#  define HAL_LCD_I2C_ADDR  0x27   // đổi sang 0x3F nếu LCD khác địa chỉ
#endif
constexpr int HAL_I2C_SDA = 21;
constexpr int HAL_I2C_SCL = 22;

// Keypad 4x4 (tham chiếu — module Keypad tự khởi tạo riêng)
constexpr uint8_t HAL_KEYPAD_ROWS[4] = {13, 12, 14, 27};
constexpr uint8_t HAL_KEYPAD_COLS[4] = {26, 25, 33, 32};

// RFID — MFRC522 (SPI: SCK=18, MISO=19, MOSI=23 mặc định ESP32)
constexpr uint8_t HAL_RFID_SS_PIN  = 5;
constexpr uint8_t HAL_RFID_RST_PIN = 4;

// Fingerprint — AS608 trên UART2
constexpr int HAL_FP_RX_PIN = 16;   // ESP32 RX2
constexpr int HAL_FP_TX_PIN = 17;   // ESP32 TX2
constexpr uint32_t HAL_FP_BAUD = 57600;

// Buzzer — PWM LEDC
constexpr int HAL_BUZZER_PIN      = 15;
constexpr int HAL_BUZZER_CHANNEL  = 3;
constexpr int HAL_BUZZER_FREQ_HZ  = 3000;
constexpr int HAL_BUZZER_RES_BITS = 8;

// // Relay — active LOW (LOW = ON)
constexpr int  HAL_RELAY_PIN        = 2;
constexpr bool HAL_RELAY_ACTIVE_LOW = true;

// // Relay – active LOW (LOW = ON)
// constexpr int  HAL_DOOR_PIN        = 2;
// constexpr bool HAL_DOOR_ACTIVE_LOW = true;

// (Tùy chọn) IRQ từ MFRC522 để đánh thức (nếu bạn có dây)
// #define HAL_RFID_IRQ_PIN 34

// =====================================
// 2) SINGLETON-LIKE ACCESSORS (đối tượng dùng chung)
// =====================================
LiquidCrystal_I2C& lcd();          // LCD 16x2
MFRC522&           rfid();         // MFRC522
HardwareSerial&    fpUart();       // UART2
Adafruit_Fingerprint& finger();    // AS608





// ============================
// 3) KHỞI TẠO & TIỆN ÍCH CƠ BẢN
// ============================
/** Khởi tạo toàn bộ ngoại vi: GPIO, LEDC(buzzer), I2C+LCD, SPI+RFID, UART2+AS608 */
void initAll();

/** Bật/tắt đèn nền LCD */
void lcdBacklight(bool on);
/** In nhanh 2 dòng lên LCD (xóa cũ) */
void lcdShow(const String& line1, const String& line2);
/** printf lên LCD (hàng 0/1, cột 0..15) */
void lcdPrintf(uint8_t row, uint8_t col, const char* fmt, ...);

/** Điều khiển relay (theo active-low) */
void relayOn();     // bật (đóng mạch)
void relayOff();    // tắt  (mở mạch)

/** Buzzer cơ bản */
void buzzerOn();
void buzzerOff();
void buzzerTone(uint32_t freqHz);  // 0 = OFF

// ============================
// 4) BEEP PATTERN NON-BLOCKING
// ============================
enum class BeepPat : uint8_t { None, Short, Double, Triple };
/** Bắt đầu một mẫu beep (không chặn loop). Gọi hal::beepUpdate() thường xuyên. */
void beepStart(BeepPat p);
/** Cập nhật state machine beep — gọi mỗi vòng loop() */
void beepUpdate();

// ============================
// 5) CHẨN ĐOÁN & TỰ KIỂM TRA
// ============================
/**
 * Quét I2C và in ra Serial địa chỉ thiết bị thấy được (debug).
 * Trả về số thiết bị phát hiện.
 */
uint8_t i2cScan();

/**
 * Self-test nhanh toàn hệ thống (có thể gọi ngay sau initAll).
 *  - bit0: LCD
 *  - bit1: Buzzer
 *  - bit2: Relay
 *  - bit3: SPI/RFID
 *  - bit4: FP UART/verify
 * patternMs: tổng thời gian mẫu beep hiển thị (mặc định ~1200 ms)
 */
uint32_t selfTest(uint32_t patternMs = 1200);

// ============================
// 6) THÔNG TIN PHIÊN BẢN
// ============================
const char* boardName();   // ví dụ: "ESP32 DevKitC"
const char* halVersion();  // ví dụ: "HAL-ESP32 v1.1.0"

} // namespace hal
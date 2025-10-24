/**
 * ===========================================================
 *  A1 — HAL implementation
 * ===========================================================
 */
#include "HAL.hpp"
#include <stdarg.h>

namespace hal {

// ---------------------------------
// Nội bộ: đối tượng dùng chung
// ---------------------------------
static LiquidCrystal_I2C _lcd(HAL_LCD_I2C_ADDR, 16, 2);
static MFRC522           _rfid(HAL_RFID_SS_PIN, HAL_RFID_RST_PIN);
static HardwareSerial    _fpSerial(2);
static Adafruit_Fingerprint _finger(&_fpSerial);

LiquidCrystal_I2C& lcd()        { return _lcd; }
MFRC522&           rfid()       { return _rfid; }
HardwareSerial&    fpUart()     { return _fpSerial; }
Adafruit_Fingerprint& finger()  { return _finger; }

// ---------------------------------
// Nội bộ: beep state machine
// ---------------------------------
struct BeepState {
  BeepPat   pat   = BeepPat::None;
  uint8_t   step  = 0;
  uint32_t  t0    = 0;
};
static BeepState _beep;

static inline void _relayWrite(bool on) {
  // on = yêu cầu bật (đóng), tùy active-low
  if (HAL_RELAY_ACTIVE_LOW) {
    digitalWrite(HAL_RELAY_PIN, on ? LOW : HIGH);
  } else {
    digitalWrite(HAL_RELAY_PIN, on ? HIGH : LOW);
  }
}

// ---------------------------------
// 3) INIT & UTIL
// ---------------------------------
void initAll() {
  // GPIO: Relay
  pinMode(HAL_RELAY_PIN, OUTPUT);
  relayOff();

  // Buzzer PWM (LEDC)
  ledcSetup(HAL_BUZZER_CHANNEL, HAL_BUZZER_FREQ_HZ, HAL_BUZZER_RES_BITS);
  ledcAttachPin(HAL_BUZZER_PIN, HAL_BUZZER_CHANNEL);
  buzzerOff();

  // I2C + LCD
  Wire.begin(HAL_I2C_SDA, HAL_I2C_SCL);
  // Nếu cần debug địa chỉ LCD: gọi i2cScan() trước
  _lcd.init();
  lcdBacklight(true);
  lcdShow("HAL init...", "LCD OK");

  // SPI + RFID
  SPI.begin();         // SCK=18, MISO=19, MOSI=23
  _rfid.PCD_Init();

  // UART2 + Fingerprint
  _fpSerial.begin(HAL_FP_BAUD, SERIAL_8N1, HAL_FP_RX_PIN, HAL_FP_TX_PIN);
  _finger.begin(HAL_FP_BAUD);
}

void lcdBacklight(bool on) {
  if (on) _lcd.backlight();
  else    _lcd.noBacklight();
}

void lcdShow(const String& l1, const String& l2) {
  _lcd.clear();
  _lcd.setCursor(0, 0); _lcd.print(l1);
  _lcd.setCursor(0, 1); _lcd.print(l2);
}

void lcdPrintf(uint8_t row, uint8_t col, const char* fmt, ...) {
  if (row > 1 || col > 15) return;
  char buf[64];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  _lcd.setCursor(col, row);
  _lcd.print(buf);
}

void relayOn()  { _relayWrite(true);  }
void relayOff() { _relayWrite(false); }

void buzzerOn()           { ledcWriteTone(HAL_BUZZER_CHANNEL, HAL_BUZZER_FREQ_HZ); }
void buzzerOff()          { ledcWriteTone(HAL_BUZZER_CHANNEL, 0); }
void buzzerTone(uint32_t fHz) { ledcWriteTone(HAL_BUZZER_CHANNEL, fHz); }

// ---------------------------------
// 4) BEEP PATTERNS (non-blocking)
// ---------------------------------
void beepStart(BeepPat p) {
  _beep.pat  = p;
  _beep.step = 0;
  _beep.t0   = millis();
  if (p == BeepPat::Short || p == BeepPat::Double || p == BeepPat::Triple) buzzerOn();
}

void beepUpdate() {
  if (_beep.pat == BeepPat::None) return;

  const uint32_t now = millis();
  constexpr uint16_t BEEP_SHORT_MS = 100;
  constexpr uint16_t BEEP_GAP_MS   = 120;

  switch (_beep.pat) {
    case BeepPat::Short:
      if (now - _beep.t0 >= BEEP_SHORT_MS) {
        buzzerOff();
        _beep.pat = BeepPat::None;
      }
      break;

    case BeepPat::Double:
      // ON(0) -> OFF(1) -> ON(2) -> OFF(end)
      if (_beep.step == 0 && now - _beep.t0 >= BEEP_SHORT_MS) {
        buzzerOff(); _beep.step = 1; _beep.t0 = now;
      } else if (_beep.step == 1 && now - _beep.t0 >= BEEP_GAP_MS) {
        buzzerOn();  _beep.step = 2; _beep.t0 = now;
      } else if (_beep.step == 2 && now - _beep.t0 >= BEEP_SHORT_MS) {
        buzzerOff(); _beep.pat = BeepPat::None;
      }
      break;

    case BeepPat::Triple:
      // ON(0) OFF(1) ON(2) OFF(3) ON(4) OFF(end)
      if ((_beep.step % 2) == 0) { // ON phases: 0,2,4
        if (now - _beep.t0 >= BEEP_SHORT_MS) {
          buzzerOff(); _beep.step++; _beep.t0 = now;
        }
      } else { // OFF gaps: 1,3,5
        if (now - _beep.t0 >= BEEP_GAP_MS) {
          if (_beep.step >= 5) {
            _beep.pat = BeepPat::None;
          } else {
            buzzerOn(); _beep.step++; _beep.t0 = now;
          }
        }
      }
      break;

    default: break;
  }
}

// ---------------------------------
// 5) I2C SCAN & SELF-TEST
// ---------------------------------
uint8_t i2cScan() {
  Serial.println(F("[HAL] I2C scan:"));
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  - Found device @ 0x%02X\n", addr);
      count++;
    }
  }
  if (count == 0) Serial.println(F("  (no I2C devices found)"));
  return count;
}

uint32_t selfTest(uint32_t patternMs) {
  uint32_t mask = 0;

  // LCD
  lcdShow("LCD: OK", "Running test...");
  mask |= (1u << 0);

  // Buzzer (short)
  beepStart(BeepPat::Short);
  uint32_t tStart = millis();
  // Cho beepUpdate chạy một nhịp ~patternMs/4 (ngắn)
  while (millis() - tStart < patternMs / 4) {
    beepUpdate();
    delay(1);
  }
  mask |= (1u << 1);

  // Relay (nháy nhẹ)
  relayOn(); delay(120);
  relayOff();
  mask |= (1u << 2);

  // RFID (đọc VersionReg ≠ 0x00/0xFF coi như OK)
  byte ver = rfid().PCD_ReadRegister(MFRC522::VersionReg);
  if (ver != 0x00 && ver != 0xFF) mask |= (1u << 3);

  // Fingerprint (verifyPassword)
  if (finger().verifyPassword()) mask |= (1u << 4);

  // LCD kết luận
  char buf[32];
  snprintf(buf, sizeof(buf), "RFID:%s FP:%s",
           (mask & (1u << 3)) ? "OK" : "NG",
           (mask & (1u << 4)) ? "OK" : "NG");
  lcdShow("SelfTest Done", String(buf));

  return mask;
}

// ---------------------------------
// 6) META
// ---------------------------------
const char* boardName()  { return "ESP32 DevKitC"; }
const char* halVersion() { return "HAL-ESP32 v1.1.0"; }

} // namespace hal
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

#define FP_RX 16
#define FP_TX 17
#define FP_BAUD 57600

HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger(&fpSerial);

unsigned long dbStartTime = 0;  // thời điểm bắt đầu đếm
bool dbActive = false;          // cờ DB có dữ liệu

void setup() {
  Serial.begin(115200);
  Serial.println("=== TEST AS608 AUTO RESET DB ===");

  fpSerial.begin(FP_BAUD, SERIAL_8N1, FP_RX, FP_TX);
  finger.begin(FP_BAUD);

  if (finger.verifyPassword()) {
    Serial.println("[FP] AS608 found & verified!");
  } else {
    Serial.println("[FP] Không tìm thấy AS608!");
    while (1) delay(1);
  }

  if (finger.getTemplateCount() == FINGERPRINT_OK) {
    Serial.printf("[FP] Database has %d fingerprints\n", finger.templateCount);
    if (finger.templateCount > 0) {
      dbActive = true;
      dbStartTime = millis(); // bắt đầu đếm reset
    }
  }
}

void loop() {
  // Reset DB sau 5 phút không hoạt động
  if (dbActive && millis() - dbStartTime > 5UL * 60UL * 1000UL) {
    Serial.println("[CMD] Hết 5 phút không hoạt động -> reset database...");
    if (finger.emptyDatabase() == FINGERPRINT_OK) {
      Serial.println("[FP] Database đã được xoá sạch!");
    } else {
      Serial.println("[FP] Lỗi khi xoá database!");
    }
    dbActive = false; // reset cờ
  }

  // Kiểm tra vân tay
  int p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) {
    delay(200);
    return;
  }

  if (p == FINGERPRINT_OK) {
    if (finger.image2Tz() == FINGERPRINT_OK) {
      if (finger.fingerFastSearch() == FINGERPRINT_OK) {
        Serial.printf("[FP] Đã tìm thấy! ID=%d (confidence=%d)\n",
                      finger.fingerID, finger.confidence);
        dbActive = true;               // DB có hoạt động
        dbStartTime = millis();        // reset lại timer 5 phút
      } else {
        Serial.println("[FP] Không tìm thấy trong database");
      }
    }
  }

  delay(200);
}
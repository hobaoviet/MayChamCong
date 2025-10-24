#include <Arduino.h>
#include "../src/A9_scheduler/scheduler.hpp"
#include "../src/A3_FingerPrint_Engine/fp_engine.hpp"
#include <SPIFFS.h>

//  CHỨC NĂNG FACTORY RESET
// - Đổi thành `true` và nạp code 1 LẦN để XÓA SẠCH toàn bộ dữ liệu (log, user, config, fingerprint).
// - Sau đó, đổi lại thành `false` và nạp lại code để hoạt động bình thường.
const bool PERFORM_FACTORY_RESET = false; //  bật reset 1 lần duy nhất

void setup()
{
  Serial.begin(115200);
  delay(200);

  Serial.println("\n=======================================");
  Serial.println("   ESP32 Attendance System Booting...  ");
  Serial.println("=======================================");

  if (PERFORM_FACTORY_RESET)
  {
    Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("!!!      FACTORY RESET REQUESTED      !!!");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

    // --- Xóa dữ liệu SPIFFS ---
    if (SPIFFS.begin(true))
    {
      Serial.println("[RESET] Wiping all data files...");
      SPIFFS.remove("/config.json");
      SPIFFS.remove("/users.json");
      SPIFFS.remove("/logs.jsonl");
      SPIFFS.remove("/fp_slots.bin");
      Serial.println("[RESET]  Data files wiped.");
    }
    else
    {
      Serial.println("[RESET] SPIFFS mount failed. Cannot wipe files.");
    }

    // --- Xóa vân tay trong cảm biến ---
    Serial.println("[RESET] Clearing all fingerprints from sensor...");
    if (fp_engine::verifySensor()) // kiểm tra kết nối trước
    {
      fp_engine::clearAllFingerprints();
    }
    else
    {
      Serial.println("[RESET]  Fingerprint sensor not detected!");
    }

    Serial.println("[RESET] --- RESET COMPLETE ---");
    Serial.println("Please set PERFORM_FACTORY_RESET to false and re-upload.");

    // Dừng lại sau khi reset xong
    while (true)
      delay(1000);
  }

  // --- Khởi tạo hệ thống bình thường ---
  scheduler::begin();
}

void loop()
{
  scheduler::update();
  delay(5);
}

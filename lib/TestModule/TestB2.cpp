#include <Arduino.h>
#include <WiFi.h>
#include "..\src\B1_ghiLog\ghiLog.hpp"          // B1 log_queue
#include "..\src\B2_sheets_client\sheet_client.hpp"    // B2 cloud

using namespace log_queue;
using namespace cloud;

// ==== WiFi Config (sửa theo mạng nhà bạn) ====
const char* WIFI_SSID = "test";
const char* WIFI_PASS = "123456789";

// ==== HTTPS Config (webhook.site) ====
#define USE_MQTT 0   // ép dùng HTTPS
const char* HTTPS_URL = "https://webhook.site/17f6c4b2-ca00-45ce-bdd7-12f8696c4627";  

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Test B2 with webhook.site ===");

  // 1. Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  // 2. Init log_queue (B1)
  if (!begin("/logs.jsonl")) {
    Serial.println("Failed to init log_queue!");
    return;
  }
  clear();
  logAttendance("rfid_1234", -1, "IN", true, "");
  logAttendance("rfid_5678", -1, "OUT", false, "unknown_uid");
  logManagement("rfid_9999", 5, true, "added user");
  Serial.printf("Prepared %u logs for test\n", count());

  // 3. Init cloud (B2) với HTTPS
  Config cfg;
  cfg.retryMs = 2000; // thử gửi lại mỗi 2s
  cfg.backend = Backend::HTTPS;
  cfg.httpsUrl = HTTPS_URL;

  begin(cfg);
  Serial.println("Cloud client initialized.");
}

void loop() {
  // Gọi update định kỳ để push log
  update();

  // In trạng thái cloud + số log còn lại mỗi 5s
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.println("Cloud status: " + status());
    Serial.printf("Remaining logs: %u\n", count());
    lastPrint = millis();
  }

  delay(100);
}
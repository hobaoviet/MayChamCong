#include <Arduino.h>
#include <WiFi.h>
#include "..\src\B1_ghiLog\ghiLog.hpp"        // B1 log queue
#include "..\src\B2_sheets_client\sheet_client.hpp"  // B2 sheet client

using namespace log_queue;
using namespace cloud;

// ===== WiFi config =====
const char* WIFI_SSID = "Oi";
const char* WIFI_PASS = "frightnight";

// ===== BACKEND SELECT =====
#define USE_MQTT  1     // 1 = dùng MQTT, 0 = dùng HTTPS

// ===== Google Apps Script (nếu dùng HTTPS) =====
const char* HTTPS_URL = "https://script.google.com/macros/s/AKfycbxH1lmXDca_mvQVPqYc1iUEMBz7h_urUOf0knQIbbeR2k2i-rnxOY1MxyUhjG9xZ3rPHw/exec";

// ===== MQTT Broker config (nếu dùng MQTT) =====
const char* MQTT_SERVER = "10.154.87.172";  // IP máy chạy Mosquitto
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "esp32/attendance/logs";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TEST MODULE B2: Push logs to Cloud (HTTPS / MQTT) ===");

  // 1️⃣ Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to WiFi %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // 2️⃣ Init log queue (B1)
  if (!begin("/logs.jsonl")) {
    Serial.println("Failed to init log_queue!");
    return;
  }
  clear();

  // --- Thêm vài log mẫu ---
  for (int i = 1; i <= 3; i++) {
    Entry e;
    e.ts     = millis();
    char buf[16];
    sprintf(buf, "rfid_%04d", i);   // tạo chuỗi rfid_0001, rfid_0002, ...
    e.uid = String(buf);
    e.fp_id  = i;
    e.mode   = "IN";
    e.ok     = true;
    e.reason = "scan ok";
    e.type   = LogType::Attendance;
    push(e);
  }
  Serial.printf("[B1] Prepared %u logs in queue\n", count());

  // 3️⃣ Cấu hình Cloud client (B2)
  Config cfg;
  cfg.retryMs = 3000;

#if USE_MQTT
  cfg.backend    = Backend::MQTT;
  cfg.mqttServer = MQTT_SERVER;
  cfg.mqttPort   = MQTT_PORT;
  cfg.mqttTopic  = MQTT_TOPIC;
  Serial.println("[B2] Mode: MQTT");
#else
  cfg.backend  = Backend::HTTPS;
  cfg.httpsUrl = HTTPS_URL;
  Serial.println("[B2] Mode: HTTPS");
#endif

  begin(cfg);   // init B2
  Serial.println("[B2] Cloud client initialized.");
}

void loop() {
  update();   // B2 lấy log từ B1 và gửi đi

  // In trạng thái định kỳ
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 4000) {
    Serial.println("---------------------------------------------------");
    Serial.println(status());
    Serial.printf("Remaining logs in queue: %u\n", count());
    lastPrint = millis();
  }
  delay(100);
}
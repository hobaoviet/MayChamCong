#include <Arduino.h>
#include <WiFi.h>

// ===== Include module B1 & B2 =====
#include "../src/B1_ghiLog/ghiLog.hpp"
#include "../src/B2_sheets_client/sheet_client.hpp"

using namespace log_queue;
using namespace cloud;

// ===== WiFi config =====
const char* WIFI_SSID = "D6-701";
const char* WIFI_PASS = "0985592743";

// ===== Google Apps Script URL =====
const char* HTTPS_URL = "https://script.google.com/macros/s/AKfycbxH1lmXDca_mvQVPqYc1iUEMBz7h_urUOf0knQIbbeR2k2i-rnxOY1MxyUhjG9xZ3rPHw/exec";

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TEST MODULE B2: Push logs to Google Sheets ===");

  // 1️. Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

  // 2️. Khởi tạo log_queue (SPIFFS)
  if (!begin("/logs.jsonl")) {
    Serial.println("[B1]  SPIFFS init failed!");
    return;
  }
  clear();  // Xóa log cũ
  Serial.println("[B1] Log queue initialized & cleared.");

  // 3️. Tạo log mẫu

  // --- Thêm 5 nhân viên (ManagementAdd) ---
  for (int i = 1; i <= 5; i++) {
    Entry e;
    e.ts = millis();
    char buf[16];
    sprintf(buf, "rfid_%04d", i);   // tạo chuỗi rfid_0001, rfid_0002, ...
    e.uid = String(buf);
    e.fp_id = i;
    e.mode = "";
    e.ok = true;
    e.reason = "added user " + String(i);
    e.type = LogType::ManagementAdd;
    push(e);
  }

  // --- Xóa nhân viên đầu tiên (ManagementDelete) ---
  {
    Entry e;
    e.ts = millis();
    e.uid = "rfid_0001";
    e.fp_id = 1;
    e.mode = "";
    e.ok = true;
    e.reason = "removed user 1";
    e.type = LogType::ManagementDelete;
    push(e);
  }

  // --- Chấm công nhân viên thứ 2 (Attendance) ---
  {
    Entry e;
    e.ts = millis();
    e.uid = "rfid_0002";
    e.fp_id = 2;
    e.mode = "IN";
    e.ok = true;
    e.reason = "scan ok";
    e.type = LogType::Attendance;
    push(e);
  }

  Serial.printf("[B1] Prepared %u logs in queue\n", count());

  // 4️. Khởi tạo cloud client (HTTPS)
  Config cfg;
  cfg.retryMs  = 3000;        // Gửi lại sau 3 giây nếu lỗi
  cfg.backend  = Backend::HTTPS;
  cfg.httpsUrl = HTTPS_URL;
  begin(cfg);

  Serial.println("[CLOUD] Client initialized (HTTPS mode).");
  Serial.println("-------------------------------------------------------");
}

// ================== LOOP ==================
void loop() {
  // B2 xử lý việc lấy log từ B1 & gửi lên Google Sheets
  update();

  // In trạng thái mỗi 5 giây
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.println("-------------------------------------------------------");
    Serial.println("Cloud status: " + status());
    Serial.printf("Remaining logs in queue: %u\n", count());
    Serial.println("-------------------------------------------------------");
    lastPrint = millis();
  }

  delay(100);
}
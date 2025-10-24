#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "..\src\B1_ghiLog\ghiLog.hpp"

using namespace log_queue;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Test B1_log_queue ===");

  // 1. Init SPIFFS + log_queue
  if (!begin("/logs.jsonl")) {
    Serial.println("Failed to init log_queue!");
    return;
  }
  Serial.println("log_queue initialized.");

  // 2. Clear log cũ
  clear();
  Serial.println("Cleared old logs.");

  // 3. Thêm một số log
  logAttendance("rfid_1234", -1, "IN", true, "");
  logAttendance("rfid_5678", -1, "OUT", false, "unknown_uid");
  logManagement("rfid_9999", 5, true, "added user");

  Serial.println("Pushed 3 logs. Count=" + String(count()));

  // 4. Peek log đầu tiên
  Entry e;
  if (peek(e)) {
    Serial.println("Peek first log:");
    String out = " ts=" + String(e.ts) +
                 " uid=" + e.uid +
                 " fp_id=" + String(e.fp_id) +
                 " mode=" + e.mode +
                 " ok=" + String(e.ok) +
                 " reason=" + e.reason +
                 " type=" + (e.type == LogType::Management ? "Management" : "Attendance");
    Serial.println(out);
  } else {
    Serial.println("Peek failed (no log).");
  }

  // 5. Pop log đầu tiên
  if (pop()) {
    Serial.println("Popped first log.");
  } else {
    Serial.println("Pop failed.");
  }

  Serial.println("Remaining logs count=" + String(count()));

  // 6. Clear tất cả log
  clear();
  Serial.println("Cleared. Final count=" + String(count()));
}

void loop() {
  // Không cần làm gì trong loop
}
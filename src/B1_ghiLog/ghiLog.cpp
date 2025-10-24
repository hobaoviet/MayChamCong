#include "ghiLog.hpp"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

namespace log_queue
{

  static String LOG_PATH = "/logs.jsonl";

  // ========== Core API ==========

  bool begin(const char *path)
  {
    LOG_PATH = path;
    bool ok = SPIFFS.begin(true);
    if (!ok)
      return false;

    if (!SPIFFS.exists(LOG_PATH.c_str()))
    {
      File f = SPIFFS.open(LOG_PATH.c_str(), FILE_WRITE);
      if (!f)
        return false;
      f.close();
    }
    return true;
  }

  bool push(const Entry &e)
  {
    File f = SPIFFS.open(LOG_PATH.c_str(), FILE_APPEND);
    if (!f)
      return false;

    DynamicJsonDocument doc(512);
    doc["ts"] = e.ts;
    doc["uid"] = e.uid;
    doc["fp_id"] = e.fp_id;
    doc["mode"] = e.mode;
    doc["ok"] = e.ok;
    doc["reason"] = e.reason;
    // doc["type"]   = (e.type == LogType::Attendance ? "attendance" : "management");

    if (e.type == LogType::Attendance)
    {
      doc["type"] = "attendance";
    }
    else if (e.type == LogType::ManagementAdd)
    {
      doc["type"] = "management_add";
    }
    else if (e.type == LogType::ManagementDelete)
    {
      doc["type"] = "management_delete";
    }

    bool ok = (serializeJson(doc, f) > 0);
    f.println(); // xuống dòng để JSONL
    f.close();
    return ok;
  }

  // ========== Peek (fixed) ==========
  bool peek(Entry &e)
  {
    File f = SPIFFS.open(LOG_PATH.c_str(), FILE_READ);
    if (!f)
      return false;

    if (f.available() <= 0)
    {
      f.close();
      return false;
    }

    String line = f.readStringUntil('\n');
    f.close();

    line.trim();
    if (line.length() == 0)
      return false;

    Serial.printf("[B1] Raw log line: %s\n", line.c_str());

    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, line);
    if (err)
    {
      Serial.printf("[B1] JSON parse error: %s\n", err.c_str());
      return false;
    }

    // Gán các field cơ bản
    e.ts = doc["ts"] | 0;
    e.fp_id = doc["fp_id"] | -1;
    e.ok = doc["ok"] | false;

    // Gán string an toàn (dùng as<String>())
    e.uid = doc["uid"].as<String>();
    e.mode = doc["mode"].as<String>();
    e.reason = doc["reason"].as<String>();

    // Gán type
    String t = doc["type"].as<String>();
    // e.type   = (t == "management") ? LogType::Management : LogType::Attendance;

    if (t == "attendance")
    {
      e.type = LogType::Attendance;
    }
    else if (t == "management_add")
    {
      e.type = LogType::ManagementAdd;
    }
    else if (t == "management_delete")
    {
      e.type = LogType::ManagementDelete;
    }
    else
    {
      e.type = LogType::Attendance; // fallback
    }
    e.data = line;
    return true;
  }

  // ========== Pop ==========
  bool pop()
  {
    File f = SPIFFS.open(LOG_PATH.c_str(), FILE_READ);
    if (!f)
      return false;

    File tmp = SPIFFS.open("/logs.tmp", FILE_WRITE);
    if (!tmp)
    {
      f.close();
      return false;
    }

    bool firstLineSkipped = false;
    while (f.available())
    {
      String line = f.readStringUntil('\n');
      if (!firstLineSkipped)
      {
        firstLineSkipped = true;
        continue;
      }
      tmp.println(line);
    }

    f.close();
    tmp.close();
    SPIFFS.remove(LOG_PATH.c_str());
    SPIFFS.rename("/logs.tmp", LOG_PATH.c_str());
    return true;
  }

  size_t count()
  {
    File f = SPIFFS.open(LOG_PATH.c_str(), FILE_READ);
    if (!f)
      return 0;
    size_t lines = 0;
    while (f.available())
    {
      f.readStringUntil('\n');
      lines++;
    }
    f.close();
    return lines;
  }

  bool clear()
  {
    return SPIFFS.remove(LOG_PATH.c_str());
  }

  // ========== Helper API ==========

  bool logAttendance(const String &uid, int fp_id, const String &mode, bool ok, const String &reason)
  {
    Entry e;
    e.ts = millis();
    e.uid = uid;
    e.fp_id = fp_id;
    e.mode = mode;
    e.ok = ok;
    e.reason = reason;
    e.type = LogType::Attendance;

    bool okWrite = push(e);

    if (okWrite)
    {
      DynamicJsonDocument doc(256);
      doc["ts"] = e.ts;
      doc["uid"] = e.uid;
      doc["fp_id"] = e.fp_id;
      doc["mode"] = e.mode;
      doc["ok"] = e.ok;
      doc["reason"] = e.reason;
      doc["type"] = "attendance";

      String jsonStr;
      serializeJson(doc, jsonStr);
      Serial.printf("[LOG][ATTENDANCE] %s\n", jsonStr.c_str());
    }

    return okWrite;
  }

  // bool logManagement(const String& uid, int fp_id, bool ok, const String& reason) {
  //   Entry e;
  //   e.ts     = millis();
  //   e.uid    = uid;
  //   e.fp_id  = fp_id;
  //   e.mode   = "";
  //   e.ok     = ok;
  //   e.reason = reason;
  //   e.type   = LogType::Management;
  //   return push(e);
  // }

  bool logManagementAdd(const String &uid, int fp_id, bool ok, const String &reason)
  {
    Entry e;
    e.ts = millis();
    e.uid = uid;
    e.fp_id = fp_id;
    e.mode = "";
    e.ok = ok;
    e.reason = reason;
    e.type = LogType::ManagementAdd;

    bool okWrite = push(e);

    if (okWrite)
    {
      DynamicJsonDocument doc(256);
      doc["ts"] = e.ts;
      doc["uid"] = e.uid;
      doc["fp_id"] = e.fp_id;
      doc["mode"] = e.mode;
      doc["ok"] = e.ok;
      doc["reason"] = e.reason;
      doc["type"] = "management_add";

      String jsonStr;
      serializeJson(doc, jsonStr);
      Serial.printf("[LOG][ADD] %s\n", jsonStr.c_str());
    }

    return okWrite;
  }

  bool logManagementDelete(const String &uid, int fp_id, bool ok, const String &reason)
  {
    Entry e;
    e.ts = millis();
    e.uid = uid;
    e.fp_id = fp_id;
    e.mode = "";
    e.ok = ok;
    e.reason = reason;
    e.type = LogType::ManagementDelete;

    bool okWrite = push(e);

    if (okWrite)
    {
      DynamicJsonDocument doc(256);
      doc["ts"] = e.ts;
      doc["uid"] = e.uid;
      doc["fp_id"] = e.fp_id;
      doc["mode"] = e.mode;
      doc["ok"] = e.ok;
      doc["reason"] = e.reason;
      doc["type"] = "management_delete";

      String jsonStr;
      serializeJson(doc, jsonStr);
      Serial.printf("[LOG][DELETE] %s\n", jsonStr.c_str());
    }

    return okWrite;
  }

} // namespace log_queue
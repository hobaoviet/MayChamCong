#include "identity_store.hpp"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

namespace identity_store
{

  // static const char* USERS_PATH = "/users.json";
  static String USERS_PATH = "/users.json";
  static std::vector<User> g_users;

  void setFilePath(const char *path)
  {
    USERS_PATH = path;
  }

  static String toUpperNoSpace(String s)
  {
    s.trim();
    s.toUpperCase();
    String out;
    out.reserve(s.length());
    for (size_t i = 0; i < s.length(); ++i)
    {
      char c = s[i];
      if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
        out += c;
    }
    return out;
  }

  String normalizeUID(String s)
  {
    if (s.startsWith("RFID_") || s.startsWith("rfid_"))
    {
      s = s.substring(5);
    }
    s.toLowerCase(); //  ép lowercase để đồng bộ với A4
    s.trim();        // phòng có khoảng trắng
    return s;
  }

  bool begin()
  {
    bool ok = SPIFFS.begin(true);
    if (!ok)
      return false;

    // if (!SPIFFS.exists(USERS_PATH)) {
    //   File f = SPIFFS.open(USERS_PATH, FILE_WRITE);
    if (!SPIFFS.exists(USERS_PATH.c_str()))
    {
      File f = SPIFFS.open(USERS_PATH.c_str(), FILE_WRITE);
      if (!f)
        return false;
      f.print("[]");
      f.close();
    }
    g_users.clear();
    loadFromFlash();
    return true;
  }

  int loadFromFlash()
  {
    // File f = SPIFFS.open(USERS_PATH, FILE_READ);
    File f = SPIFFS.open(USERS_PATH.c_str(), FILE_READ);
    if (!f)
      return -1;
    size_t sz = f.size();
    if (sz == 0)
    {
      f.close();
      g_users.clear();
      return 0;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err)
    {
      Serial.printf("[ID] JSON parse error: %s\n", err.c_str());
      return -1;
    }
    if (!doc.is<JsonArray>())
      return -1;

    g_users.clear();
    for (JsonObject o : doc.as<JsonArray>())
    {
      User u;
      u.rfid_uid = normalizeUID((const char *)(o["rfid_uid"] | ""));
      u.fp_id = o["fp_id"] | -1;
      if (u.rfid_uid.length() == 0 && u.fp_id < 0)
        continue;
      g_users.push_back(u);
    }
    return (int)g_users.size();
  }

  bool saveToFlash()
  {
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto &u : g_users)
    {
      JsonObject o = arr.createNestedObject();
      o["rfid_uid"] = u.rfid_uid;
      o["fp_id"] = u.fp_id;
    }

    // File f = SPIFFS.open(USERS_PATH, FILE_WRITE);
    File f = SPIFFS.open(USERS_PATH.c_str(), FILE_WRITE);
    if (!f)
      return false;
    bool ok = (serializeJson(doc, f) > 0);
    f.close();
    return ok;
  }

  String toJsonAll()
  {
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto &u : g_users)
    {
      JsonObject o = arr.createNestedObject();
      o["rfid_uid"] = u.rfid_uid;
      o["fp_id"] = u.fp_id;
    }
    String out;
    serializeJson(doc, out);
    return out;
  }

  // bool addEmployee(const String& uid, int fp_id) {
  //   String norm = normalizeUID(uid);
  //   if (existsUid(norm) || (fp_id >= 0 && existsFinger(fp_id))) {
  //     return false; // đã tồn tại
  //   }
  //   User u;
  //   u.rfid_uid = norm;
  //   u.fp_id    = fp_id;
  //   g_users.push_back(u);
  //   return true;
  // }

  bool addEmployee(const String &uid, int fp_id)
  {
    String norm = normalizeUID(uid);

    // --- load lại danh sách mới nhất trước khi check ---
    // loadFromFlash();

    // --- check UID ---
    if (existsUid(norm))
    {
      Serial.printf("[ID_STORE] Duplicate UID %s, not adding\n", norm.c_str());
      return false;
    }

    // --- check Finger ID ---
    if (fp_id >= 0 && existsFinger(fp_id))
    {
      Serial.printf("[ID_STORE] Duplicate FingerID %d, not adding\n", fp_id);
      return false;
    }

    // --- thêm mới ---
    User u;
    u.rfid_uid = norm;
    u.fp_id = fp_id;
    g_users.push_back(u);

    // --- ghi xuống Flash ngay ---
    saveToFlash();

    Serial.printf("[ID_STORE] ✅ Added user (uid=%s, fid=%d)\n", norm.c_str(), fp_id);
    return true;
  }

  // bool removeByUid(const String& uid) {
  //   String norm = normalizeUID(uid);
  //   for (size_t i = 0; i < g_users.size(); ++i) {
  //     if (g_users[i].rfid_uid == norm) {
  //       g_users.erase(g_users.begin() + i);
  //       return true;
  //     }
  //   }
  //   return false;
  // }

  // bool removeByUid(const String& uid) {
  //   String norm = normalizeUID(uid);
  //   for (auto it = g_users.begin(); it != g_users.end(); ++it) {
  //     if (it->rfid_uid == norm) {
  //       g_users.erase(it);
  //       saveToFlash();   // 🔹 cập nhật flash ngay sau khi xoá
  //       return true;
  //     }
  //   }
  //   return false;
  // }

  bool removeByUid(const String &uid)
  {
    String norm = normalizeUID(uid);

    for (auto it = g_users.begin(); it != g_users.end(); ++it)
    {
      if (it->rfid_uid == norm)
      {
        g_users.erase(it);

        // lưu lại ngay sau khi xoá
        saveToFlash();

        Serial.printf("[ID_STORE] Removed user (uid=%s)\n", norm.c_str());
        return true;
      }
    }

    Serial.printf("[ID_STORE] UID %s not found, cannot remove\n", norm.c_str());
    return false;
  }

  bool existsUid(const String &uid)
  {
    String norm = normalizeUID(uid);
    for (const auto &u : g_users)
    {
      if (u.rfid_uid == norm)
        return true;
    }
    return false;
  }

  bool existsFinger(int fp_id)
  {
    if (fp_id < 0)
      return false;
    for (const auto &u : g_users)
    {
      if (u.fp_id == fp_id)
        return true;
    }
    return false;
  }

  // int getFingerByUid(const String& uid) {
  //   String norm = normalizeUID(uid);
  //   for (const auto& u : g_users) {
  //     if (u.rfid_uid == norm) return u.fp_id;
  //   }
  //   return -1;
  // }

  int getFingerByUid(const String &uid)
  {
    String norm = normalizeUID(uid);
    for (auto &u : g_users)
    {
      if (u.rfid_uid == norm)
        return u.fp_id;
    }
    return -1;
  }

  String getUidByFinger(int fp_id)
  {
    for (const auto &u : g_users)
    {
      if (u.fp_id == fp_id)
        return u.rfid_uid;
    }
    return "";
  }

  const std::vector<User> &all() { return g_users; }

} // namespace identity_store
#include "config.hpp"
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

namespace config
{
  static const char *CONFIG_PATH = "/config.json";
  static WifiConfig wifiCfg;
  static MqttConfig mqttCfg;

  bool begin()
  {
    if (!SPIFFS.begin(true))
    {
      Serial.println("[CFG] SPIFFS mount failed");
      return false;
    }
    return loadConfig();
  }

  bool loadConfig()
  {
    if (!SPIFFS.exists(CONFIG_PATH))
    {
      // ✅ QUAN TRỌNG:  thay đổi thông tin WiFi mặc định
      wifiCfg = {"test1", "123456789"};
      mqttCfg = {"10.235.197.90", 1883, "", "", "esp32/attendance/logs", 5000};
      return saveConfig();
    }

    File f = SPIFFS.open(CONFIG_PATH, FILE_READ);
    if (!f)
      return false;

    DynamicJsonDocument doc(1024);
    if (deserializeJson(doc, f))
    {
      f.close();
      Serial.println("[CFG] parse error");
      return false;
    }
    f.close();

    wifiCfg.ssid = doc["wifi"]["ssid"] | "";
    wifiCfg.pass = doc["wifi"]["pass"] | "";
    mqttCfg.server = doc["mqtt"]["server"] | "";
    mqttCfg.port = doc["mqtt"]["port"] | 1883;
    mqttCfg.user = doc["mqtt"]["user"] | "";
    mqttCfg.pass = doc["mqtt"]["pass"] | "";
    mqttCfg.topic = doc["mqtt"]["topic"] | "esp32/attendance/logs";
    mqttCfg.retryMs = doc["mqtt"]["retryMs"] | 5000;
    return true;
  }

  bool saveConfig()
  {
    DynamicJsonDocument doc(1024);
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = wifiCfg.ssid;
    wifi["pass"] = wifiCfg.pass;
    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["server"] = mqttCfg.server;
    mqtt["port"] = mqttCfg.port;
    mqtt["user"] = mqttCfg.user;
    mqtt["pass"] = mqttCfg.pass;
    mqtt["topic"] = mqttCfg.topic;
    mqtt["retryMs"] = mqttCfg.retryMs;
    File f = SPIFFS.open(CONFIG_PATH, FILE_WRITE);
    if (!f)
      return false;
    bool ok = (serializeJsonPretty(doc, f) > 0);
    f.close();
    return ok;
  }

  WifiConfig getWifi() { return wifiCfg; }
  MqttConfig getMqtt() { return mqttCfg; }

  void setWifi(const String &ssid, const String &pass)
  {
    wifiCfg.ssid = ssid;
    wifiCfg.pass = pass;
    saveConfig();
  }

  void setMqtt(const MqttConfig &c)
  {
    mqttCfg = c;
    saveConfig();
  }

  bool connectWifi()
  {
    static bool wifiConnecting = false;
    static unsigned long lastAttempt = 0;
    static uint8_t retryCount = 0;
    const uint32_t retryInterval = 5000; // 5 giây
    const uint8_t maxRetries = 5;

    auto localWifiCfg = getWifi();

    // --- Nếu WiFi đã kết nối ---
    if (WiFi.status() == WL_CONNECTED)
      return true;

    // --- Nếu chưa kết nối và chưa bắt đầu ---
    if (!wifiConnecting)
    {
      if (localWifiCfg.ssid.isEmpty() || localWifiCfg.pass.isEmpty())
      {
        Serial.println("[WiFi] ❌ Missing SSID or password in config.json");
        return false;
      }

      Serial.printf("[WiFi] 🌐 Connecting to %s...\n", localWifiCfg.ssid.c_str());
      WiFi.mode(WIFI_STA);
      WiFi.begin(localWifiCfg.ssid.c_str(), localWifiCfg.pass.c_str());
      wifiConnecting = true;
      lastAttempt = millis();
      retryCount = 0;
      return false;
    }

    // --- Nếu đang chờ kết nối ---
    if (millis() - lastAttempt > retryInterval)
    {
      lastAttempt = millis();
      retryCount++;

      if (retryCount > maxRetries)
      {
        Serial.println("[WiFi] ❌ Failed to connect after several attempts. Going OFFLINE.");
        wifiConnecting = false;
        return false;
      }

      Serial.printf("[WiFi] ⚠️ Retry #%d...\n", retryCount);
      WiFi.disconnect();
      WiFi.begin(localWifiCfg.ssid.c_str(), localWifiCfg.pass.c_str());
    }

    // --- Nếu kết nối thành công ---
    if (WiFi.status() == WL_CONNECTED)
    {
      wifiConnecting = false;
      Serial.printf("[WiFi] ✅ Connected! IP: %s\n", WiFi.localIP().toString().c_str());
      return true;
    }

    return false; // đang kết nối (non-blocking)
  }

  void ensureWifi()
  {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck < 3000)
      return; // kiểm tra mỗi 3 giây
    lastCheck = millis();

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("[WiFi] Connection lost → reconnecting...");
      connectWifi();
    }
  }

  // ✅ Thêm hàm này để scheduler.cpp có thể gọi
  bool isWifiConnected()
  {
    return (WiFi.status() == WL_CONNECTED);
  }

} // namespace config

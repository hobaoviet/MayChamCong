#include "mqtt_client.hpp"
#include <WiFi.h>
#include <ArduinoJson.h>
#include "../src/A7_door/door.hpp"

namespace mqtt_client
{

  static AsyncMqttClient mqttClient;
  static Config cfg;
  static unsigned long lastReconnectAttempt = 0;
  static bool connected = false;
  static bool connecting = false;
  static const char *CLIENT_ID = "esp32_attendance_1";
  static const char *TOPIC_DOOR_CMD = "esp32/door/open_cmd";
  static const char *TOPIC_DOOR_STATUS = "esp32/door/status";

  // ============================== MQTT CORE ==============================
  static void _connectMQTT()
  {
    if (!WiFi.isConnected())
    {
      Serial.println("[MQTT] ⏸ WiFi not ready, skip connect");
      return;
    }

    if (connecting)
    {
      Serial.println("[MQTT] ⏳ Already connecting, skip duplicate");
      return;
    }

    connecting = true;
    Serial.println("[MQTT] Connecting...");
    mqttClient.connect();
  }

  static void onMqttConnect(bool sessionPresent)
  {
    connected = true;
    connecting = false;
    Serial.printf("[MQTT] ✅ Connected to %s:%d (sessionPresent=%d)\n",
                  cfg.server.c_str(), cfg.port, sessionPresent);

    mqttClient.subscribe(TOPIC_DOOR_CMD, 1);
    Serial.println("[MQTT] 📡 Subscribed to door control topic");
  }

  static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
  {
    connected = false;
    connecting = false;
    Serial.printf("[MQTT] ⚠️ Disconnected (reason=%d)\n", (int)reason);

    // Cho AsyncTCP dọn socket cũ
    delay(100);
  }

  // ============================== DOOR HANDLER ==============================
  void handleDoorMessage(char *topic, char *payload,
                         AsyncMqttClientMessageProperties properties,
                         size_t len, size_t index, size_t total)
  {
    String msg;
    for (size_t i = 0; i < len; i++)
      msg += (char)payload[i];

    Serial.printf("[MQTT] 🔔 Received on %s: %s\n", topic, msg.c_str());

    if (strcmp(topic, TOPIC_DOOR_CMD) == 0)
    {
      DynamicJsonDocument doc(256);
      if (deserializeJson(doc, msg))
      {
        Serial.println("[MQTT] ❌ JSON parse error");
        return;
      }

      String cmd = doc["cmd"] | "";
      String by = doc["by"] | "unknown";

      if (cmd == "open_door")
      {
        Serial.printf("[DOOR] 🔓 Remote open by %s\n", by.c_str());
        door::open(5000); // mở relay 5 giây

        // Gửi phản hồi trạng thái
        DynamicJsonDocument res(128);
        res["ok"] = true;
        res["by"] = by;
        res["status"] = "door_opened";

        String jsonOut;
        serializeJson(res, jsonOut);
        mqttClient.publish(TOPIC_DOOR_STATUS, 1, false, jsonOut.c_str());
        Serial.printf("[MQTT] ✅ Door status published: %s\n", jsonOut.c_str());
      }
    }
  }

  void setupDoorControl()
  {
    mqttClient.onMessage(handleDoorMessage);
  }

  // ============================== MAIN ==============================
  void begin(const Config &_cfg)
  {
    cfg = _cfg;
    mqttClient.setClientId(CLIENT_ID);
    mqttClient.setServer(cfg.server.c_str(), cfg.port);
    if (cfg.user.length() > 0)
      mqttClient.setCredentials(cfg.user.c_str(), cfg.pass.c_str());

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);

    setupDoorControl();

    Serial.printf("[MQTT] Init broker %s:%d\n", cfg.server.c_str(), cfg.port);
    _connectMQTT();
  }

  void update()
  {
    // ✅ Kiểm tra điều kiện reconnect an toàn
    if (WiFi.isConnected() && !connected && millis() - lastReconnectAttempt > cfg.retryMs)
    {
      lastReconnectAttempt = millis();
      _connectMQTT();
    }

    // --- Gửi log lên MQTT ---
    log_queue::Entry e;
    if (connected && log_queue::peek(e))
    {
      if (!e.ok)
      {
        Serial.println("[MQTT] ⚠️ Skip log (ok=false)");
        log_queue::pop();
        return;
      }

      String jsonStr = e.data;
      bool ok = mqttClient.publish(cfg.topic.c_str(), 1, false, jsonStr.c_str(), jsonStr.length());
      if (ok)
      {
        Serial.printf("[MQTT] ✅ Published log (%s)\n", cfg.topic.c_str());
        log_queue::pop();
      }
      else
      {
        Serial.println("[MQTT] ❌ Publish failed");
      }
    }
  }

  bool pushNow(const log_queue::Entry &e)
  {
    if (!connected)
      return false;
    String jsonStr = e.data;
    return mqttClient.publish(cfg.topic.c_str(), 1, false, jsonStr.c_str(), jsonStr.length());
  }

  String status()
  {
    return connected ? "Connected" : "Disconnected";
  }
  bool isConnected()
  {
    return connected;
  }

} // namespace mqtt_client

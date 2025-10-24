#pragma once
#include <Arduino.h>

namespace config
{

  struct WifiConfig
  {
    String ssid;
    String pass;
  };

  struct MqttConfig
  {
    String server;
    int port;
    String user;
    String pass;
    String topic;
    uint32_t retryMs;
  };

  // ===== API =====
  bool begin();
  bool loadConfig();
  bool saveConfig();

  WifiConfig getWifi();
  MqttConfig getMqtt();

  void setWifi(const String &ssid, const String &pass);
  void setMqtt(const MqttConfig &c);

  bool connectWifi();
  void ensureWifi();

  //  Thêm dòng này để scheduler.cpp có thể gọi kiểm tra trạng thái Wi-Fi
  bool isWifiConnected();

} // namespace config

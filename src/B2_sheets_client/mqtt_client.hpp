#pragma once
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include "../B1_ghiLog/ghiLog.hpp"

namespace mqtt_client
{

  struct Config
  {
    String server;
    uint16_t port = 1883;
    String user;
    String pass;
    String topic;
    uint32_t retryMs = 5000;
  };

  void begin(const Config &cfg);
  void update();
  bool pushNow(const log_queue::Entry &e);
  String status();
  void setupDoorControl();
  void handleDoorMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
  bool isConnected();

} // namespace mqtt_client

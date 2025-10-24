#include <WiFi.h>
#include <ESP32Ping.h>

const char* ssid = "Oi";          // WiFi SSID của bạn
const char* password = "frightnight"; // Mật khẩu WiFi
IPAddress targetIP(10, 154, 87, 172);    // Địa chỉ IP máy tính cần ping

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 Ping Test ===");
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi connect failed!");
    return;
  }

  Serial.printf("Pinging host %s ...\n", targetIP.toString().c_str());
  bool success = Ping.ping(targetIP, 5);  // ping 5 lần

  if (success) {
    Serial.println("✅ Ping successful! Host reachable.");
  } else {
    Serial.println("❌ Ping failed! Host unreachable.");
  }
}

void loop() {
  delay(5000); // Không làm gì thêm
}
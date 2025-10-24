#include "rfid_engine.hpp"

namespace rfid_engine
{

  static uint32_t lastSeenAt = 0;
  static String lastUidHex = "";
  static uint32_t cooldownMs = 2000;
  static uint32_t ignoreUntil = 0; // ✅ bỏ qua tạm sau khi enroll xong

  void begin()
  {
    hal::rfid().PCD_Init(); // re-init
    Serial.println(F("[RFID] MFRC522 initialized."));
  }

  AuthResult poll()
  {
    AuthResult r;

    // ✅ Bỏ qua đọc thẻ nếu đang trong thời gian ignore
    if (millis() < ignoreUntil)
      return r;

    MFRC522 &mfrc = hal::rfid();

    if (!mfrc.PICC_IsNewCardPresent())
      return r;
    if (!mfrc.PICC_ReadCardSerial())
    {
      r.hasResult = true;
      r.ok = false;
      r.user_id = "UNKNOWN";
      r.uid = "";
      r.reason = "read_error";
      r.t_ms = millis();
      return r;
    }

    // chuyển UID thành hex string
    char buf[32];
    String uidHex = "";
    for (byte i = 0; i < mfrc.uid.size; i++)
    {
      sprintf(buf, "%02X", mfrc.uid.uidByte[i]);
      uidHex += buf;
    }
    uidHex.toLowerCase(); // ép UID về lowercase

    uint32_t now = millis();

    // cooldown check
    if (uidHex == lastUidHex && (now - lastSeenAt < cooldownMs))
    {
      return r; // không trả về gì
    }

    lastUidHex = uidHex;
    lastSeenAt = now;

    r.hasResult = true;
    r.ok = true;
    r.user_id = "RFID_" + uidHex; // có prefix để log/debug
    r.uid = uidHex;               // UID thô để so sánh/lưu
    r.reason = "";
    r.t_ms = now;

    // Dừng giao tiếp với card để sẵn sàng lần sau
    mfrc.PICC_HaltA();
    mfrc.PCD_StopCrypto1();

    return r;
  }

  void setCooldown(uint32_t ms)
  {
    cooldownMs = ms;
  }

  // ✅ API mới: bỏ qua RFID trong thời gian ms
  void ignoreFor(uint32_t ms)
  {
    ignoreUntil = millis() + ms;
    Serial.printf("[RFID] Ignore for %lu ms\n", (unsigned long)ms);
  }

  // tiện ích: lấy UID cuối cùng đọc được
  String getUIDString()
  {
    return lastUidHex;
  }

} // namespace rfid_engine
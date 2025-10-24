#include <SPIFFS.h>
#include "..\src\A5_identity_store\identity_store.hpp"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TEST A5_identity_store FULL (with SPIFFS reset) ===");

  // 0. Format SPIFFS để xóa dữ liệu rác
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed!");
    return;
  }
  Serial.println("Formatting SPIFFS...");
  if (SPIFFS.format()) {
    Serial.println("SPIFFS formatted successfully!");
  } else {
    Serial.println("SPIFFS format FAILED!");
  }

  // 1. Khởi tạo
  identity_store::begin();
  Serial.println("Store init done");

  // 2. Thêm nhân viên
  identity_store::addEmployee("UID1234", 1);
  identity_store::addEmployee("UID5678", 2);
  identity_store::saveToFlash();
  Serial.println("Added 2 employees");

  // 3. In toàn bộ danh sách
  auto users = identity_store::all();
  Serial.println("All users:");
  for (auto &u : users) {
    Serial.printf("  UID=%s , FP=%d\n", u.rfid_uid.c_str(), u.fp_id);
  }

  // 4. Kiểm tra tồn tại
  Serial.printf("existsUid(UID1234) = %d\n", identity_store::existsUid("UID1234"));
  Serial.printf("existsFinger(2) = %d\n", identity_store::existsFinger(2));

  // 5. Tra cứu
  int fp = identity_store::getFingerByUid("UID1234");
  String uid = identity_store::getUidByFinger(2);
  Serial.printf("getFingerByUid(UID1234) = %d\n", fp);
  Serial.printf("getUidByFinger(2) = %s\n", uid.c_str());

  // 6. Xóa nhân viên
  bool removed = identity_store::removeByUid("UID1234");
  Serial.printf("removeByUid(UID1234) = %d\n", removed);
  identity_store::saveToFlash();

  // 7. In lại danh sách sau khi xóa
  users = identity_store::all();
  Serial.println("All users after remove:");
  for (auto &u : users) {
    Serial.printf("  UID=%s , FP=%d\n", u.rfid_uid.c_str(), u.fp_id);
  }

  // 8. Format SPIFFS lần nữa → sạch sẽ cho test module sau
  Serial.println("Cleaning SPIFFS for next test...");
  SPIFFS.format();
  Serial.println("Done. Ready for next module test.");
}

void loop() {}
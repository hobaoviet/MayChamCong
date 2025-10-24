#include <Arduino.h>
#include "..\src\A1_HAL\HAL.hpp"
#include "A3_FingerPrint_Engine/fp_engine.hpp"

using namespace hal;
using namespace fp_engine;

// ======================= CONFIG ==========================
constexpr uint16_t ENROLL_TIMEOUT_MS = 10000;  // 10s cho mỗi bước
constexpr uint16_t AUTH_INTERVAL_MS  = 500;    // 0.5s kiểm tra vân tay

// ======================= HELPER ==========================
void showEnrollStatus(const EnrollStatus& st) {
  if (!st.changed) return;
  lcdShow("Enroll", st.message);
  Serial.printf("[ENROLL] state=%d slot=%d msg=%s reason=%s\n",
                st.state, st.slot, st.message.c_str(), st.reason.c_str());

  switch (st.state) {
    case EnrollState::DoneOK:   beepStart(BeepPat::Double); break;
    case EnrollState::DoneFail: beepStart(BeepPat::Triple); break;
    case EnrollState::Cancelled:beepStart(BeepPat::Short); break;
    default: break;
  }
}

void showAuthResult(const AuthResult& a) {
  if (!a.hasResult) return;                             // đổi changed -> hasResult

  if (a.ok) {
    lcdShow("Access Granted", String("ID:") + String(a.fingerId)); // đổi a.id -> a.fingerId
    relayOn();
    beepStart(BeepPat::Double);
    delay(1000);
    relayOff();
  } else {
    lcdShow("Denied", a.reason);                        // a.reason là String -> OK
    beepStart(BeepPat::Triple);
  }

  // In debug an toàn theo đúng kiểu dữ liệu
  Serial.printf("[AUTH] ok=%d id=%d reason=%s\n",
                a.ok ? 1 : 0,
                (int)a.fingerId,
                a.reason.c_str());
}

// ======================= SETUP ==========================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== TEST MODULE A3 + AS608 ===");

  initAll();                    // HAL init
  lcdShow("HAL init done", "Testing A3...");
  delay(800);

  if (!fp_engine::begin()) {
    lcdShow("A3 init FAIL", "");
    while (true);
  }
  lcdShow("A3 init OK", "FP sensor ready");
  setPollInterval(AUTH_INTERVAL_MS);
  setEnrollStepTimeout(ENROLL_TIMEOUT_MS);

  Serial.println("Commands: ENROLL <id>, DELETE <id>, WIPE");
}

// ======================= LOOP ==========================
void loop() {
  beepUpdate();

  // liên tục poll để kiểm tra vân tay
  AuthResult a = poll();
  showAuthResult(a);

  // cập nhật quá trình enroll nếu có
  EnrollStatus st = updateEnroll();
  showEnrollStatus(st);

  // --- nhận lệnh serial ---
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.startsWith("ENROLL")) {
      int id = line.substring(6).toInt();
      if (id <= 0) id = 1;
      bool ok = startEnroll(id);
      lcdShow("Start Enroll", String("ID:") + String(id));
      Serial.printf("startEnroll(%d) -> %d\n", id, ok);
    }
    else if (line.startsWith("DELETE")) {
      int id = line.substring(6).toInt();
      if (id <= 0) id = 1;
      bool ok = deleteFingerprint(id);
      lcdShow("Delete ID", String(id) + (ok ? " OK" : " FAIL"));
      Serial.printf("deleteFingerprint(%d) -> %d\n", id, ok);
      beepStart(ok ? BeepPat::Double : BeepPat::Triple);
    }
    else if (line.equalsIgnoreCase("WIPE")) {
  // xóa hết database trên AS608
  uint8_t rc = hal::finger().emptyDatabase();
  bool ok = (rc == 0);                // 0 == FINGERPRINT_OK

  lcdShow("Wipe All", ok ? "OK" : "FAIL");
  Serial.printf("emptyDatabase() -> rc=%u (%s)\n", rc, ok ? "OK" : "FAIL");
  beepStart(ok ? BeepPat::Double : BeepPat::Triple);
}
    else if (line.equalsIgnoreCase("CANCEL")) {
      cancelEnroll();
      lcdShow("Enroll Cancel", "");
      beepStart(BeepPat::Short);
    }
    else {
      Serial.println("Cmd: ENROLL <id> | DELETE <id> | WIPE | CANCEL");
    }
  }

  delay(50);
}

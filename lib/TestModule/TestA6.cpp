#include "..\src\A6_ui\ui.hpp"
#include "..\src\A5_identity_store\identity_store.hpp"
#include "..\src\A3_FingerPrint_Engine\fp_engine.hpp"
#include "..\src\A2_KeypadMode\A2_keypad_mode.hpp"
#include "..\src\A1_HAL\HAL.hpp"   // thêm dòng này

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TEST A6_UI FULL ===");

  // Khởi tạo HAL trước (init I²C, LCD, relay, buzzer...)
  hal::initAll();
  // 1. Boot + Ready
  ui::showBoot();
  delay(2000);
  ui::showReady(keypad_mode::Mode::IN);
  delay(2000);

  // 2. Status
  ui::showStatus("MyWiFi", true, 5);
  delay(2000);

  // 3. Attendance
  identity_store::User u1{"UID1234", 7};
  ui::showGranted(u1, keypad_mode::Mode::IN);
  delay(2000);
  ui::showDenied("unknown_uid");
  delay(2000);

  // 4. Enroll
  ui::showAddPrompt();
  delay(2000);
  ui::showAddSuccess("UID5678", 8);
  delay(2000);
  ui::showAddFail("uid_exists");
  delay(2000);

  fp_engine::EnrollStatus st;
  st.slot = 9;

  // Bước 1: đặt ngón lần 1
  st.message = "Place finger 1";
  st.state = fp_engine::EnrollState::WaitFinger1;
  st.changed = true;
  ui::showEnroll(st);
  delay(2000);

  // Bước 2: nhấc ngón
  st.message = "Remove finger";
  st.state = fp_engine::EnrollState::RemoveFinger1;
  st.changed = true;
  ui::showEnroll(st);
  delay(2000);

  // Bước 3: đặt ngón lần 2
  st.message = "Place finger 2";
  st.state = fp_engine::EnrollState::WaitFinger2;
  st.changed = true;
  ui::showEnroll(st);
  delay(2000);

  // Hoàn tất OK
  st.state = fp_engine::EnrollState::DoneOK;
  st.message = "Done OK";
  st.changed = true;
  ui::showEnroll(st);
  delay(2000);

  // Thử trường hợp Fail
  st.state = fp_engine::EnrollState::DoneFail;
  st.message = "Failed";
  st.changed = true;
  ui::showEnroll(st);
  delay(2000);

  // 5. Delete
  ui::showRemovePrompt();
  delay(2000);
  ui::showRemoveSuccess("UID1234", 7);
  delay(2000);
  ui::showRemoveFail("uid_not_found");
  delay(2000);

  // 6. Misc
  ui::showMessage("Custom Msg", "Hello!");
  delay(2000);
  ui::clear();
  delay(2000);

  Serial.println("=== UI TEST DONE ===");
}

void loop() {}
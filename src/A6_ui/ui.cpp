#include "ui.hpp"

namespace ui
{

  static String lastLine1 = "";
  static String lastLine2 = "";

  // Hàm wrapper có cache
  // static void lcdShowCached(const String& line1, const String& line2) {
  //   if (line1 == lastLine1 && line2 == lastLine2) {
  //     return; // nội dung không đổi → bỏ qua
  //   }

  //   lastLine1 = line1;
  //   lastLine2 = line2;

  //   hal::lcdShow(line1, line2);  // gọi thật
  // }

  static void lcdShowCached(String line1, String line2)
  {
    if (line1 == lastLine1 && line2 == lastLine2)
      return;

    lastLine1 = line1;
    lastLine2 = line2;

    // đảm bảo không dài quá 16 ký tự
    if (line1.length() > 16)
      line1 = line1.substring(0, 16);
    if (line2.length() > 16)
      line2 = line2.substring(0, 16);

    hal::lcdShow(line1, line2);
  }

  // void showBoot() {
  //   hal::lcdShow("System Booting", hal::halVersion());
  //   hal::beepStart(hal::BeepPat::Double);
  // }

  void showBoot()
  {
    lcdShowCached("System Booting", hal::halVersion());
    hal::beepStart(hal::BeepPat::Double);
  }

  // void showReady(keypad_mode::Mode m) {
  //   hal::lcdShow("Ready Mode", keypad_mode::modeName(m));
  //   hal::beepStart(hal::BeepPat::Short);
  // }

  void showReady(keypad_mode::Mode m)
  {
    lcdShowCached("Ready Mode", keypad_mode::modeName(m));
    hal::beepStart(hal::BeepPat::Short);
  }

  // ===== Access Control =====
  void showGranted(const identity_store::User &u, keypad_mode::Mode m)
  {
    String line1 = String("Granted ") + keypad_mode::modeName(m);
    String line2;

    if (u.rfid_uid.length() > 0)
    {
      line2 = "UID:" + u.rfid_uid;
    }
    else if (u.fp_id >= 0)
    {
      line2 = "FP:" + String(u.fp_id);
    }
    else
    {
      line2 = "Unknown ID";
    }

    lcdShowCached(line1, line2);
    hal::beepStart(hal::BeepPat::Double);
  }

  
  void showDenied(const String &reason)
  {
    lcdShowCached("Access Denied", reason);
    hal::beepStart(hal::BeepPat::Triple);
  }

  

  void showStatus(const String &wifi, bool cloudOK, size_t userCount)
  {
    String l1 = "WiFi:" + wifi;
    String l2 = "Users:" + String(userCount) + (cloudOK ? " CloudOK" : " CloudNG");
    lcdShowCached(l1, l2); // dùng bản có cache, tránh nhấp nháy
  }

  // ===== Enroll =====
  void showEnroll(const fp_engine::EnrollStatus &st)
  {
    String l1 = "Enroll Slot:" + String(st.slot);
    String l2 = st.message;
    lcdShowCached(l1, l2);

    if (st.state == fp_engine::EnrollState::DoneOK)
    {
      hal::beepStart(hal::BeepPat::Double);
    }
    else if (st.state == fp_engine::EnrollState::DoneFail ||
             st.state == fp_engine::EnrollState::Timeout ||
             st.state == fp_engine::EnrollState::Cancelled)
    {
      hal::beepStart(hal::BeepPat::Triple);
    }
  }

  // ===== Employee Management =====
  void showAddPrompt()
  {
    lcdShowCached("Add Employee", "Scan RFID+FP");
  }

  // void showAddSuccess(const String& uid, int fp_id) {
  //   String line2 = "UID:" + uid + " FP:" + String(fp_id);
  //   lcdShowCached("Add Success", line2);
  //   hal::beepStart(hal::BeepPat::Double);
  // }

  void showAddSuccess(const String &uid, int fp_id)
  {
    String shortUid = uid;
    if (shortUid.length() > 4)
    {
      shortUid = shortUid.substring(shortUid.length() - 4); // lấy 4 ký tự cuối
    }

    String line2 = "U:" + shortUid + " F:" + String(fp_id);
    lcdShowCached("Add Success", line2);
    hal::beepStart(hal::BeepPat::Double);
  }

  void showAddFail(const String &reason)
  {
    lcdShowCached("Add Failed", reason);
    hal::beepStart(hal::BeepPat::Triple);
  }

  void showRemovePrompt()
  {
    lcdShowCached("Remove Employee", "Scan RFID/FP");
  }

  // void showRemoveSuccess(const String& uid) {
  //   hal::lcdShow("Removed", "UID:" + uid);
  //   hal::beepStart(hal::BeepPat::Double);
  // }
  // Sua lan 1 :

  void showRemoveSuccess(const String &uid, int fpId)
  {
    String line2 = "UID:" + uid;
    if (fpId >= 0)
    {
      line2 += " FP:" + String(fpId);
    }
    lcdShowCached("Removed", line2);
    hal::beepStart(hal::BeepPat::Double);
  }

  void showRemoveFail(const String &reason)
  {
    lcdShowCached("Remove Failed", reason);
    hal::beepStart(hal::BeepPat::Triple);
  }

  // ===== Misc =====
  void showMessage(const String &l1, const String &l2)
  {
    lcdShowCached(l1, l2);
  }

  void resetLCDCache()
  {
    lastLine1 = "";
    lastLine2 = "";
  }

  void clear()
  {
    hal::lcd().clear();
    resetLCDCache(); // reset cache để lần sau chắc chắn update lại
  }

} // namespace ui

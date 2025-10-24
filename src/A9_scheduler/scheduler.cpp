#include <WiFi.h>
#include "scheduler.hpp"
#include "../A0_config/config.hpp"
#include "../B2_sheets_client/mqtt_client.hpp"

namespace scheduler
{

  static State curState = State::BOOT;
  static keypad_mode::Mode curMode = keypad_mode::Mode::IN;
  static uint32_t lastResultAt = 0;
  static bool showingResult = false;
  static String pendingUid = "";
  static int pendingFpId = -1;
  static bool mqttStarted = false;

  void begin()
  {
    Serial.println("[SYS] Initializing hardware...");
    hal::initAll();
    keypad_mode::begin(50);
    fp_engine::begin();
    rfid_engine::begin();
    identity_store::begin();
    door::begin();
    log_queue::begin("/logs.jsonl");

    power::Config pcfg;
    pcfg.idleTimeoutMs = 30 * 60 * 1000;
    pcfg.mutePeripheralsBeforeSleep = true;
    pcfg.ext1Mask = (1ULL << GPIO_NUM_26) | (1ULL << GPIO_NUM_25) |
                    (1ULL << GPIO_NUM_33) | (1ULL << GPIO_NUM_32);
    pcfg.ext1Mode = ESP_EXT1_WAKEUP_ANY_HIGH;
    power::begin(pcfg);

    ui::showBoot();
    delay(1000);

    // --- 🌐 Bắt đầu kết nối Wi-Fi (non-blocking, không chặn loop) ---
    Serial.println("[SYS] Starting network setup...");
    config::begin();
    config::connectWifi(); // khởi động quá trình kết nối, trả về ngay

    curState = State::IDLE;
    ui::showReady(curMode);
  }

  // =====================================================
  // ATTENDANCE FLOW
  // =====================================================
  static void handleAttendance()
  {
    // --- 1. RFID ---
    auto r = rfid_engine::poll();
    if (r.hasResult && r.ok && r.uid.length() > 0)
    {
      power::feedActivity();
      String mode = keypad_mode::modeName(curMode);
      if (identity_store::existsUid(r.uid))
      {
        int fpId = identity_store::getFingerByUid(r.uid);
        identity_store::User u{r.uid, fpId};
        ui::showGranted(u, curMode);
        door::open(5000);
        log_queue::logAttendance(r.uid, fpId, mode, true, "rfid_access");
      }
      else
      {
        ui::showDenied("UID unknown");
        log_queue::logAttendance(r.uid, -1, mode, false, "unknown_uid");
      }
      lastResultAt = millis();
      showingResult = true;
      return;
    }

    // --- 2. Fingerprint ---
    auto fr = fp_engine::poll();
    if (fr.hasResult)
    {
      power::feedActivity();
      String mode = keypad_mode::modeName(curMode);
      if (fr.ok && fr.fingerId >= 0)
      {
        if (identity_store::existsFinger(fr.fingerId))
        {
          String uid = identity_store::getUidByFinger(fr.fingerId);
          identity_store::User u{uid, fr.fingerId};
          ui::showGranted(u, curMode);
          door::open(5000);
          log_queue::logAttendance(uid, fr.fingerId, mode, true, "fp_access");
        }
        else
        {
          ui::showDenied("FP not mapped");
          log_queue::logAttendance("", fr.fingerId, mode, false, "fp_not_mapped");
        }
      }
      else
      {
        ui::showDenied("FP unknown");
        log_queue::logAttendance("", -1, mode, false, "fp_not_found");
      }
      lastResultAt = millis();
      showingResult = true;
      return;
    }
  }

  // =====================================================
  // ENROLL FLOW
  // =====================================================
  static void handleEnroll()
  {
    if (pendingUid == "")
    {
      auto r = rfid_engine::poll();
      if (r.hasResult && r.ok && r.uid.length() > 0)
      {
        if (identity_store::existsUid(r.uid))
        {
          ui::showAddFail("UID exists");
          log_queue::logManagementAdd(r.uid, -1, false, "uid_exists");
          lastResultAt = millis();
          showingResult = true;
          return;
        }
        pendingUid = r.uid;
        ui::showMessage("Scan Finger", "");
        power::setBusy(true);
      }
      return;
    }

    if (!fp_engine::isEnrolling())
    {
      delay(200);
      fp_engine::startEnroll(0);
    }

    auto st = fp_engine::updateEnroll();
    if (st.changed)
      ui::showEnroll(st);

    if (st.state == fp_engine::EnrollState::DoneOK && st.changed)
    {
      delay(300);
      identity_store::addEmployee(pendingUid, st.slot);
      identity_store::saveToFlash();
      ui::showAddSuccess(pendingUid, st.slot);
      log_queue::logManagementAdd(pendingUid, st.slot, true, "added user");

      auto &finger = hal::finger();
      finger.getImage();
      finger.image2Tz(1);
      finger.image2Tz(2);

      lastResultAt = millis();
      showingResult = true;
      pendingUid = "";
      pendingFpId = -1;
      power::setBusy(false);
    }
    else if (st.state == fp_engine::EnrollState::DoneFail ||
             st.state == fp_engine::EnrollState::Timeout)
    {
      ui::showAddFail(st.message);
      log_queue::logManagementAdd(pendingUid, -1, false, st.message);
      fp_engine::cancelEnroll();
      lastResultAt = millis();
      showingResult = true;
      pendingUid = "";
      pendingFpId = -1;
      power::setBusy(false);
    }
  }

  // =====================================================
  // DELETE FLOW
  // =====================================================
  static void handleDelete()
  {
    auto r = rfid_engine::poll();
    if (r.hasResult && r.ok && r.uid.length() > 0)
    {
      int fpId = identity_store::getFingerByUid(r.uid);
      if (identity_store::removeByUid(r.uid))
      {
        if (fpId >= 0)
          fp_engine::deleteFingerprint(fpId);
        identity_store::saveToFlash();
        ui::showRemoveSuccess(r.uid, fpId);
        log_queue::logManagementDelete(r.uid, fpId, true, "deleted uid+finger");
      }
      else
      {
        ui::showRemoveFail("UID not found");
        log_queue::logManagementDelete(r.uid, -1, false, "uid_not_found");
      }
      lastResultAt = millis();
      showingResult = true;
      return;
    }

    auto fr = fp_engine::poll();
    if (fr.hasResult && fr.ok && fr.fingerId >= 0)
    {
      String uid = identity_store::getUidByFinger(fr.fingerId);
      if (uid != "" && identity_store::removeByUid(uid))
      {
        fp_engine::deleteFingerprint(fr.fingerId);
        identity_store::saveToFlash();
        ui::showRemoveSuccess(uid, fr.fingerId);
        log_queue::logManagementDelete(uid, fr.fingerId, true, "deleted finger+uid");
      }
      else
      {
        ui::showRemoveFail("FP not found");
        log_queue::logManagementDelete("", fr.fingerId, false, "fp_not_found");
      }
      lastResultAt = millis();
      showingResult = true;
    }
  }

  // =====================================================
  // MAIN UPDATE LOOP
  // =====================================================
  void update()
  {
    keypad_mode::update();
    config::ensureWifi();
    mqtt_client::update();

    // 🔄 Nếu Wi-Fi có IP mà MQTT chưa khởi động, bật lại MQTT
    if (config::isWifiConnected() && !mqtt_client::isConnected() && !mqttStarted)
    {
      auto mqttCfg = config::getMqtt();
      mqtt_client::Config mcfg;
      mcfg.server = mqttCfg.server;
      mcfg.port = mqttCfg.port;
      mcfg.user = mqttCfg.user;
      mcfg.pass = mqttCfg.pass;
      mcfg.topic = mqttCfg.topic;
      mcfg.retryMs = mqttCfg.retryMs;
      mqtt_client::begin(mcfg);
      mqttStarted = true;
      Serial.println("[MQTT] 🔌 MQTT client started after Wi-Fi connected.");
    }

    if (showingResult)
    {
      if (millis() - lastResultAt > 3000)
      {
        showingResult = false;
        if (curState == State::ENROLL_WAIT_RFID || curState == State::ENROLL_FINGERPRINT)
        {
          curState = State::ENROLL_WAIT_RFID;
          ui::showAddPrompt();
        }
        else if (curState == State::DELETE_EMPLOYEE)
        {
          curState = State::DELETE_EMPLOYEE;
          ui::showRemovePrompt();
        }
        else
        {
          curState = State::IDLE;
          ui::showReady(curMode);
        }
      }
      return;
    }

    door::update();
    hal::beepUpdate();
    power::update();

    keypad_mode::Event e;
    while (keypad_mode::consumeEvent(e))
    {
      power::feedActivity();
      switch (e.type)
      {
      case keypad_mode::EventType::ModeChanged:
        curMode = e.newMode;
        curState = State::IDLE;
        ui::showReady(curMode);
        break;
      case keypad_mode::EventType::EnrollEmployee:
        curState = State::ENROLL_WAIT_RFID;
        pendingUid = "";
        pendingFpId = -1;
        ui::showAddPrompt();
        break;
      case keypad_mode::EventType::DeleteEmployee:
        curState = State::DELETE_EMPLOYEE;
        ui::showRemovePrompt();
        break;
      case keypad_mode::EventType::SleepRequested:
        curState = State::SLEEP;
        ui::showMessage("Sleeping...", "after 3s");
        power::armSleepAfter(3000);
        break;
      default:
        break;
      }
    }

    switch (curState)
    {
    case State::IDLE:
      handleAttendance();
      break;
    case State::ENROLL_WAIT_RFID:
    case State::ENROLL_FINGERPRINT:
      handleEnroll();
      break;
    case State::DELETE_EMPLOYEE:
      handleDelete();
      break;
    default:
      break;
    }
  }
} // namespace scheduler

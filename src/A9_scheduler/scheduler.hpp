// ===========================================================
// File: scheduler.hpp
// ===========================================================
#pragma once
#include <Arduino.h>
#include "../A1_HAL/HAL.hpp"
#include "../A2_KeypadMode/A2_keypad_mode.hpp"
#include "../A3_FingerPrint_Engine/fp_engine.hpp"
#include "../A4_rfid_engine/rfid_engine.hpp"
#include "../A5_identity_store/identity_store.hpp"
#include "../A6_ui/ui.hpp"
#include "../A7_door/door.hpp"
#include "../A8_power/power.hpp"
#include "../B1_ghiLog/ghiLog.hpp"
#include "../B2_sheets_client/mqtt_client.hpp"
#include "../A0_config/config.hpp"
namespace scheduler
{

  enum class State
  {
    BOOT,
    IDLE,
    ATTENDANCE,
    ENROLL_WAIT_RFID,
    ENROLL_FINGERPRINT,
    DELETE_EMPLOYEE,
    SLEEP
  };

  void begin();
  void update();

} // namespace scheduler
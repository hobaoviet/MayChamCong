#pragma once
/**
 * ===========================================================
 *  A6 — UI Manager (LCD + Buzzer)
 *  -----------------------------------------------------------
 *  - Hiển thị thông tin hệ thống, chấm công, thêm/xoá nhân viên
 *  - Dùng hal::lcdShow() và hal::beepStart()
 * ===========================================================
 */

#include <Arduino.h>
#include "../A1_HAL/HAL.hpp"
#include "../A2_KeypadMode/A2_keypad_mode.hpp"
#include "../A5_identity_store/identity_store.hpp"
#include "../A3_FingerPrint_Engine/fp_engine.hpp"

namespace ui {

// ========== Boot / Ready ==========
void showBoot();
void showReady(keypad_mode::Mode m);

// ========== Access Control ==========
void showGranted(const identity_store::User& u, keypad_mode::Mode m);
void showDenied(const String& reason);

// ========== Status ==========
void showStatus(const String& wifi, bool cloudOK, size_t userCount);

// ========== Enroll ==========
void showEnroll(const fp_engine::EnrollStatus& st);

// ========== Employee Management ==========
void showAddPrompt();
void showAddSuccess(const String& uid, int fp_id);
void showAddFail(const String& reason);

void showRemovePrompt();
void showRemoveSuccess(const String& uid, int fpId);
void showRemoveFail(const String& reason);

// ========== Misc ==========
void showMessage(const String& l1, const String& l2);
void resetLCDCache() ;
void clear();

} // namespace ui
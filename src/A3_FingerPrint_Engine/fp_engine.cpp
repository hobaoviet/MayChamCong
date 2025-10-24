#include "fp_engine.hpp"
#include <SPIFFS.h> // để lưu map slot vào flash
static uint8_t storeModelFixed(Adafruit_Fingerprint &finger, uint16_t id);

namespace fp_engine
{

  // =====================
  // AUTH nội bộ
  // =====================
  static uint32_t lastPoll = 0;
  static uint32_t pollInterval = 100;

  // =====================
  // ENROLL nội bộ
  // =====================
  static EnrollState st = EnrollState::Idle;
  static uint16_t stSlot = 0;
  static uint32_t stT0 = 0;
  static uint32_t stepTimeoutMs = 20000; // 20s/bước
  static bool stChanged = false;

  // =====================
  // SLOT MANAGEMENT nội bộ
  // =====================
  static bool usedSlots[256]; // 1 = đã dùng, 0 = trống
  static const char *SLOT_FILE = "/fp_slots.bin";

  // ---------------------
  // Helpers
  // ---------------------
  static void _enter(EnrollState s)
  {
    st = s;
    stT0 = millis();
    stChanged = true;
  }

  static bool _timeout()
  {
    return (millis() - stT0) > stepTimeoutMs;
  }

  static void _clearFpBuffer()
  {
    Adafruit_Fingerprint &finger = hal::finger();
    finger.getImage();  // clear image buffer
    finger.image2Tz(1); // clear charBuffer1
    finger.image2Tz(2); // clear charBuffer2
  }

  static void _flushEnrollBuffers()
  {
    Adafruit_Fingerprint &finger = hal::finger();
    // Ép chuyển rỗng vào buffer để clear
    finger.getImage();
    finger.image2Tz(1);
    finger.image2Tz(2);
    delay(200);
    Serial.println("[FP] Flush enroll buffers done");
  }

  // ---------------------
  // Quản lý map slot
  // ---------------------
  static void _saveSlotMap()
  {
    if (!SPIFFS.begin(true))
      return;
    File f = SPIFFS.open(SLOT_FILE, FILE_WRITE);
    if (!f)
      return;
    f.write((uint8_t *)usedSlots, sizeof(usedSlots));
    f.close();
  }

  static void _loadSlotMap()
  {
    if (!SPIFFS.begin(true))
    {
      Serial.println("[FP] SPIFFS mount failed");
      return;
    }
    File f = SPIFFS.open(SLOT_FILE, FILE_READ);
    if (!f)
    {
      memset(usedSlots, 0, sizeof(usedSlots));
      return;
    }
    f.read((uint8_t *)usedSlots, sizeof(usedSlots));
    f.close();
  }

  /**
   * @brief SỬA LỖI: Hàm tìm slot trống được làm lại để an toàn hơn.
   *
   * 1. Duyệt nhanh qua mảng `usedSlots` trong RAM. Nếu tìm thấy slot trống, trả về ngay.
   * Đây là trường hợp phổ biến và nhanh nhất.
   * 2. Nếu sau khi duyệt mảng RAM mà không thấy slot trống (hoặc mảng bị sai),
   * thực hiện quét lại toàn bộ cảm biến để đồng bộ hóa trạng thái thực tế.
   * 3. Trong quá trình quét, nó sẽ cập nhật lại mảng `usedSlots` và tìm ra slot trống đầu tiên.
   * 4. Lưu lại bản đồ slot đã được đồng bộ để các lần gọi sau nhanh hơn.
   */
  static int _findFreeSlot()
  {
    Adafruit_Fingerprint &finger = hal::finger();
    int cap = finger.capacity > 0 ? finger.capacity : 255;

    // --- Bước 1: Duyệt nhanh qua bản đồ slot trong RAM ---
    for (int i = 1; i <= cap; i++)
    {
      if (!usedSlots[i])
      {
        Serial.printf("[FP][_findFreeSlot]  Found potential free slot in RAM map: %d\n", i);
        // Xác thực lại với cảm biến để chắc chắn 100%
        if (finger.loadModel(i) != FINGERPRINT_OK)
        {
          return i;
        }
        else
        {
          // Nếu RAM nói trống nhưng cảm biến lại báo đã dùng -> đồng bộ lại
          Serial.printf("[FP][_findFreeSlot]  RAM map mismatch. Slot %d is actually used. Syncing.\n", i);
          usedSlots[i] = true;
        }
      }
    }

    // --- Bước 2: Nếu không tìm thấy hoặc map bị sai, quét lại toàn bộ cảm biến ---
    Serial.println("[FP][_findFreeSlot] RAM map is full or out of sync. Rescanning sensor to find free slot...");
    delay(500); // Cho cảm biến thời gian nghỉ trước khi quét hàng loạt

    bool mapChanged = false;
    int foundSlot = -1;

    finger.getTemplateCount(); // Cập nhật lại tổng số vân tay
    Serial.printf("[FP][_findFreeSlot] Sensor reports %d templates / capacity=%d\n", finger.templateCount, cap);

    for (int i = 1; i <= cap; i++)
    {
      bool isUsedOnSensor = (finger.loadModel(i) == FINGERPRINT_OK);

      // Nếu trạng thái trên sensor khác với trong RAM, cập nhật lại RAM
      if (usedSlots[i] != isUsedOnSensor)
      {
        usedSlots[i] = isUsedOnSensor;
        mapChanged = true;
      }

      // Nếu tìm thấy slot trống trong lúc quét, lưu lại và thoát sớm
      if (!isUsedOnSensor && foundSlot == -1)
      {
        foundSlot = i;
      }
    }

    if (mapChanged)
    {
      Serial.println("[FP][_findFreeSlot] Slot map has been resynced with sensor.");
      _saveSlotMap(); // Lưu lại bản đồ slot vừa được đồng bộ
    }

    if (foundSlot != -1)
    {
      Serial.printf("[FP][_findFreeSlot]  Found free slot via full scan: %d\n", foundSlot);
    }
    else
    {
      Serial.println("[FP][_findFreeSlot]  No free slot found after full scan!");
    }

    return foundSlot;
  }

  // =====================
  // AUTH PUBLIC
  // =====================
  bool begin()
  {
    Adafruit_Fingerprint &finger = hal::finger();

    if (finger.verifyPassword())
    {
      Serial.println(F("[FP] Sensor AS608 found & verified."));

  
      finger.getTemplateCount();
      if (finger.templateCount > finger.capacity)
      {
        Serial.printf("[FP]  Invalid template count %d, forcing resync...\n", finger.templateCount);
        finger.emptyDatabase();
        finger.getTemplateCount();
      }

      // --- Load slot map hiện tại (nếu có) ---
      _loadSlotMap();

      //  Quét lại thực tế trên sensor để đồng bộ usedSlots[]
      if (finger.getTemplateCount() == FINGERPRINT_OK)
      {
        int total = finger.templateCount;
        Serial.printf("[FP] Sensor reports %d templates\n", total);

        memset(usedSlots, 0, sizeof(usedSlots));
        int cap = finger.capacity > 0 ? finger.capacity : 255;
        for (int i = 1; i <= cap; i++)
        {
          int p = finger.loadModel(i);
          usedSlots[i] = (p == FINGERPRINT_OK);
        }
        _saveSlotMap();
        Serial.println("[FP] Slot map resynced with sensor");
      }

      return true;
    }
    else
    {
      Serial.println(F("[FP] Fingerprint sensor not found / wrong password."));
      return false;
    }
  }

  fp_engine::AuthResult poll()
  {
    static bool hadFinger = false; // nhớ xem lần trước có tay không
    AuthResult r;

    if (isEnrolling())
      return r;

    uint32_t now = millis();
    if (now - lastPoll < pollInterval)
      return r;
    lastPoll = now;

    Adafruit_Fingerprint &finger = hal::finger();
    uint8_t p = finger.getImage();

    // ========== KHÔNG CÓ NGÓN NÀO ==========
    if (p == FINGERPRINT_NOFINGER)
    {
      hadFinger = false;
      return r;
    }

    // ========== VỪA CÓ NGÓN TAY MỚI ==========
    if (!hadFinger)
    {
      // chỉ xử lý khi tay vừa mới đặt
      hadFinger = true;

      // lấy hình ảnh
      if (p != FINGERPRINT_OK)
      {
        r.hasResult = true;
        r.ok = false;
        r.user_id = "UNKNOWN";
        r.reason = "image_error";
        r.t_ms = now;
        return r;
      }

      // chuyển ảnh sang template
      p = finger.image2Tz();
      if (p != FINGERPRINT_OK)
      {
        r.hasResult = true;
        r.ok = false;
        r.user_id = "UNKNOWN";
        r.reason = "image2Tz_fail";
        r.t_ms = now;
        return r;
      }

      // tìm trong DB
      p = finger.fingerFastSearch();
      r.hasResult = true;
      r.t_ms = now;
      if (p == FINGERPRINT_OK)
      {
        r.ok = true;
        r.fingerId = finger.fingerID;
        r.user_id = "FP_" + String(finger.fingerID);
        r.confidence = finger.confidence;
      }
      else
      {
        r.ok = false;
        r.fingerId = -1;
        r.user_id = "UNKNOWN";
        r.confidence = -1;
        r.reason = "not_found";
      }

      return r;
    }

    // ========== ĐANG GIỮ TAY TRÊN CẢM BIẾN ==========
    // nếu người dùng chưa nhấc tay lên, bỏ qua để không spam liên tục
    return r;
  }
  ////////////////
  void setPollInterval(uint32_t ms) { pollInterval = ms; }

  int verifyFingerprint()
  {
    Adafruit_Fingerprint &finger = hal::finger();
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK)
      return -1;

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK)
      return -1;

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK)
      return finger.fingerID;
    return -1;
  }

  // =====================
  // ENROLL PUBLIC
  // =====================
  bool startEnroll(uint16_t slotId)
  {
    if (st != EnrollState::Idle)
      return false;

    Adafruit_Fingerprint &finger = hal::finger();

    // 🔹 Dọn sạch bộ đệm trước khi bắt đầu enroll mới
    _clearFpBuffer();
    delay(300);
    Serial.println("[FP] Cleared sensor buffers before starting new enroll");

    // Nếu không chỉ định ID, tự tìm slot trống
    if (slotId == 0)
    {
      int freeSlot = _findFreeSlot();
      if (freeSlot < 0)
      {
        Serial.println("[FP][ENROLL] No free slot available!");
        return false;
      }
      slotId = static_cast<uint16_t>(freeSlot);
    }

    stSlot = slotId;
    _enter(EnrollState::WaitFinger1);
    Serial.printf("[FP][ENROLL] Start for slot %u\n", stSlot);
    return true;
  }

  fp_engine::EnrollStatus updateEnroll()
  {
    EnrollStatus out;
    out.changed = false;
    out.state = st;
    out.slot = stSlot;

    if (st == EnrollState::Idle)
    {
      out.message = "Idle";
      return out;
    }

    Adafruit_Fingerprint &finger = hal::finger();

    auto setMsg = [&](const char *m)
    {
      out.changed = stChanged;
      out.state = st;
      out.slot = stSlot;
      out.message = m;
      stChanged = false;
    };

    switch (st)
    {
    case EnrollState::WaitFinger1:
    {
      if (_timeout())
      {
        _enter(EnrollState::Timeout);
        setMsg("Timeout finger #1");
        break;
      }
      uint8_t p = finger.getImage();
      if (p == FINGERPRINT_OK)
      {
        _enter(EnrollState::Capture1);
        setMsg("Capturing #1...");
      }
      else
      {
        setMsg("Place finger #1");
      }
    }
    break;

    case EnrollState::Capture1:
    {
      uint8_t p = finger.image2Tz(1);
      if (p == FINGERPRINT_OK)
      {
        _enter(EnrollState::RemoveFinger1);
        setMsg("Remove finger");
      }
      else
      {
        _enter(EnrollState::DoneFail);
        out.reason = "image2Tz1_fail";
        setMsg("Fail convert #1");
      }
    }
    break;

    case EnrollState::RemoveFinger1:
    {
      if (_timeout())
      {
        _enter(EnrollState::Timeout);
        setMsg("Timeout remove");
        break;
      }
      uint8_t p = finger.getImage();
      if (p == FINGERPRINT_NOFINGER)
      {
        _enter(EnrollState::WaitFinger2);
        setMsg("Place finger #2");
      }
      else
      {
        setMsg("Lift finger off");
      }
    }
    break;

    case EnrollState::WaitFinger2:
    {
      if (_timeout())
      {
        _enter(EnrollState::Timeout);
        setMsg("Timeout finger #2");
        break;
      }
      uint8_t p = finger.getImage();
      if (p == FINGERPRINT_OK)
      {
        _enter(EnrollState::Capture2);
        setMsg("Capturing #2...");
      }
      else
      {
        setMsg("Place finger #2");
      }
    }
    break;

    case EnrollState::Capture2:
    {
      uint8_t p = finger.image2Tz(2);
      if (p == FINGERPRINT_OK)
      {
        _enter(EnrollState::CreateModel);
        setMsg("Creating model...");
      }
      else
      {
        _enter(EnrollState::DoneFail);
        out.reason = "image2Tz2_fail";
        setMsg("Fail convert #2");
      }
    }
    break;

    case EnrollState::CreateModel:
    {
      uint8_t p = finger.createModel();
      if (p == FINGERPRINT_OK)
      {
        _enter(EnrollState::StoreModel);
        setMsg("Storing model...");
      }
      else
      {
        _enter(EnrollState::DoneFail);
        out.reason = "createModel_fail";
        setMsg("Fail create model");
      }
    }
    break;

    case EnrollState::StoreModel:
    {
      uint8_t p = storeModelFixed(finger, stSlot);
      if (p == FINGERPRINT_OK)
      {
        usedSlots[stSlot] = true;
        _saveSlotMap();

        //  Force flush toàn bộ buffer (2 char buffers + image)
        _clearFpBuffer();
        delay(300);
        Serial.println("[FP] Flushed buffers after storeModel");

        //  Re-sync template count với sensor (đồng bộ slot)
        finger.getTemplateCount();
        delay(100);

        _enter(EnrollState::DoneOK);
        setMsg("Enroll OK");
      }
      else
      {
        _enter(EnrollState::DoneFail);
        out.reason = "storeModel_fail";
        setMsg("Fail store model");
      }
    }
    break;

    case EnrollState::DoneOK:
    {
      setMsg("Done OK");
      usedSlots[stSlot] = true; // Đảm bảo slot được đánh dấu đã dùng

      //  FLUSH BUFFER trong RAM của cảm biến để xoá ảnh cũ
      _clearFpBuffer();
      delay(300);
      Serial.println("[FP] Flushed buffers after DoneOK");

      //  Refresh lại danh sách template (đồng bộ slot thực tế)
      finger.getTemplateCount();
      delay(100);
      Serial.printf("[FP] Template count resynced = %d\n", finger.templateCount);

      // Giữ nguyên stSlot để scheduler lấy ID thật
      out.slot = stSlot;
      // Cập nhật lại bản đồ slot một lần nữa cho chắc
      _saveSlotMap();

      st = EnrollState::Idle;
      stChanged = true;

      Serial.printf("[FP][ENROLL] Finished OK at slot %u\n", stSlot);
      Serial.printf("[FP] Total templates now = %d\n", finger.templateCount);
      break;
    }
    case EnrollState::DoneFail:
      setMsg("Enroll FAIL");
      _clearFpBuffer();
      // phòng hờ build cũ từng đánh dấu sớm:
      if (stSlot > 0)
      {
        usedSlots[stSlot] = false;
        _saveSlotMap();
      }
      stSlot = 0;
      _enter(EnrollState::Idle);
      break;

    case EnrollState::Cancelled:
      setMsg("Cancelled");
      _clearFpBuffer();
      // đã giải phóng trong cancelEnroll(), nhưng reset slot cho chắc:
      stSlot = 0;
      _enter(EnrollState::Idle);
      break;

    case EnrollState::Timeout:
      setMsg("Timeout");
      _clearFpBuffer();
      // phòng hờ build cũ:
      if (stSlot > 0)
      {
        usedSlots[stSlot] = false;
        _saveSlotMap();
      }
      stSlot = 0;
      _enter(EnrollState::Idle);
      break;
    default:
      break;
    }

    return out;
  }

  void cancelEnroll()
  {
    if (st != EnrollState::Idle)
    {
      _enter(EnrollState::Cancelled);
      if (stSlot > 0)
      {
        usedSlots[stSlot] = false; // giải phóng lại slot
        _saveSlotMap();
      }
      stSlot = 0;
      Serial.println("[FP][ENROLL] Cancelled by user");
    }
  }

  bool isEnrolling()
  {
    return (st != EnrollState::Idle &&
            st != EnrollState::DoneOK &&
            st != EnrollState::DoneFail &&
            st != EnrollState::Cancelled &&
            st != EnrollState::Timeout);
  }

  void setEnrollStepTimeout(uint32_t ms) { stepTimeoutMs = ms; }

  bool deleteFingerprint(uint16_t id)
  {
    Adafruit_Fingerprint &finger = hal::finger();
    uint8_t p = finger.deleteModel(id);
    if (p == FINGERPRINT_OK)
    {
      usedSlots[id] = false; // giải phóng slot
      _saveSlotMap();
      Serial.printf("[FP] Deleted fingerprint ID %u\n", id);
      return true;
    }
    else
    {
      Serial.printf("[FP] Failed to delete fingerprint ID %u (code=%d)\n", id, p);
      return false;
    }
  }

  bool verifySensor()
  {
    return hal::finger().verifyPassword();
  }

  void clearAllFingerprints()
  {
    Adafruit_Fingerprint &finger = hal::finger();
    delay(1500); // chờ cảm biến sẵn sàng

    if (!finger.verifyPassword())
    {
      Serial.println("[FP]  Sensor not found or wrong password.");
      return;
    }

    uint8_t p = finger.emptyDatabase();
    if (p == FINGERPRINT_OK)
    {
      Serial.println("[FP]  All fingerprints cleared successfully!");
    }
    else
    {
      Serial.printf("[FP]  Failed to clear database (code=%d)\n", p);
    }

    // đồng bộ lại map slot
    memset(usedSlots, 0, sizeof(usedSlots));
    _saveSlotMap();

    if (finger.getTemplateCount() == FINGERPRINT_OK)
    {
      Serial.printf("[FP] Sensor reports %d templates after clear.\n", finger.templateCount);
    }
  }

} // namespace fp_engine

static uint8_t storeModelFixed(Adafruit_Fingerprint &finger, uint16_t id)
{
  // Packet format cho lệnh 0x06 (STORETEMPLATE)
  uint8_t packetData[4];
  packetData[0] = 0x06;      // FINGERPRINT_STORETEMPLATE
  packetData[1] = 0x01;      // Buffer ID 1 (thường là buffer 1)
  packetData[2] = id >> 8;   // High byte
  packetData[3] = id & 0xFF; // Low byte

  Adafruit_Fingerprint_Packet packet(FINGERPRINT_COMMANDPACKET, sizeof(packetData), packetData);
  finger.writeStructuredPacket(packet);

  if (finger.getStructuredPacket(&packet) != FINGERPRINT_OK)
    return FINGERPRINT_PACKETRECIEVEERR;

  if (packet.type != FINGERPRINT_ACKPACKET)
    return FINGERPRINT_PACKETRECIEVEERR;

  return packet.data[0]; // 0 = OK
}

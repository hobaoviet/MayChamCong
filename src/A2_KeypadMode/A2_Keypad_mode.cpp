#include "../A2_KeypadMode/A2_keypad_mode.hpp"

namespace keypad_mode
{

  // ===================== CẤU HÌNH =====================
  static const byte ROWS = 4;
  static const byte COLS = 4;

  static char keys[ROWS][COLS] = {
      {'1', '2', '3', 'A'},
      {'4', '5', '6', 'B'},
      {'7', '8', '9', 'C'},
      {'*', '0', '#', 'D'}};

  static byte rowPins[ROWS] = {
      hal::HAL_KEYPAD_ROWS[0], hal::HAL_KEYPAD_ROWS[1],
      hal::HAL_KEYPAD_ROWS[2], hal::HAL_KEYPAD_ROWS[3]};

  static byte colPins[COLS] = {
      hal::HAL_KEYPAD_COLS[0], hal::HAL_KEYPAD_COLS[1],
      hal::HAL_KEYPAD_COLS[2], hal::HAL_KEYPAD_COLS[3]};

  static Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

  static Mode currentMode = Mode::IN;
  static bool allowRawKey = false;

  // Queue FIFO
  constexpr size_t QSIZE = 32;
  static Event q[QSIZE];
  static volatile uint8_t q_head = 0;
  static volatile uint8_t q_tail = 0;

  static uint32_t lastKeyAtMs = 0;
  static uint16_t debounceMsLocal = 35;

  // ===================== HÀM NỘI BỘ =====================
  static void enqueue(const Event &e)
  {
    uint8_t nhead = (q_head + 1) % QSIZE;
    if (nhead == q_tail)
      q_tail = (q_tail + 1) % QSIZE;
    q[q_head] = e;
    q_head = nhead;
  }

  static bool isDigitKey(char k) { return (k >= '0' && k <= '9'); }
  static bool isSymbolKey(char k) { return (k == '*' || k == '#'); }

  // ===================== API PUBLIC =====================
  void begin(uint16_t debounceMs)
  {
    debounceMsLocal = debounceMs;
    keypad.setDebounceTime(debounceMsLocal);

    //  Wiring chuẩn ESP32: ROW = OUTPUT, COL = INPUT_PULLUP
    for (byte i = 0; i < ROWS; i++)
      pinMode(rowPins[i], OUTPUT);
    for (byte j = 0; j < COLS; j++)
      pinMode(colPins[j], INPUT_PULLUP);

    currentMode = Mode::IN;
    q_head = q_tail = 0;
    lastKeyAtMs = millis();

    Serial.println("[KEYPAD] Initialized (ROW=OUTPUT, COL=INPUT_PULLUP)");
  }

  void update()
  {
    char k = keypad.getKey();
    if (!k)
      return;

    uint32_t now = millis();
    if (now - lastKeyAtMs < debounceMsLocal)
      return;
    lastKeyAtMs = now;

    Event e;
    e.key = k;
    e.t_ms = now;

    switch (k)
    {
  
    case 'A': // Mode IN
      currentMode = Mode::IN;
      e.type = EventType::ModeChanged;
      e.newMode = Mode::IN;
      enqueue(e);
      break;

    case 'B': // Mode OUT
      currentMode = Mode::OUT;
      e.type = EventType::ModeChanged;
      e.newMode = Mode::OUT;
      enqueue(e);
      break;

    case '*': // Enroll
      e.type = EventType::EnrollEmployee;
      enqueue(e);
      break;

    case '#': // Delete
      e.type = EventType::DeleteEmployee;
      enqueue(e);
      break;

    case 'C': // Status
      e.type = EventType::StatusRequested;
      enqueue(e);
      break;

    case 'D': // Sleep
      e.type = EventType::SleepRequested;
      enqueue(e);
      break;

    default:
      if (allowRawKey && (isDigitKey(k) || isSymbolKey(k)))
      {
        e.type = EventType::RawKey;
        enqueue(e);
      }
      break;
    }
  }

  Mode getMode() { return currentMode; }

  void setMode(Mode m, bool emitEvent)
  {
    if (currentMode == m)
      return;
    currentMode = m;
    if (emitEvent)
    {
      Event e;
      e.type = EventType::ModeChanged;
      e.key = (m == Mode::IN ? 'A' : 'B');
      e.newMode = m;
      e.t_ms = millis();
      enqueue(e);
    }
  }

  bool consumeEvent(Event &out)
  {
    if (q_head == q_tail)
      return false;
    out = q[q_tail];
    q_tail = (q_tail + 1) % QSIZE;
    return true;
  }

  bool available() { return q_head != q_tail; }
  void setRawKeyAllowed(bool enable) { allowRawKey = enable; }
  const char *modeName(Mode m) { return (m == Mode::IN) ? "IN" : "OUT"; }

} // namespace keypad_mode
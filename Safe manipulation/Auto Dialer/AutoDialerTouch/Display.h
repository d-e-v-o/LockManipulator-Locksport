#pragma once

/*
  WT32-SC01 Plus onboard display driver and UI.

  Display: ST7796 480×320, parallel 8-bit (Intel 8080)
  Touch:   FT5x06 capacitive, I2C

  Layout (landscape, 480×320):
    Y=0   Header "AUTO DIALER" + status badge          H=44
    Y=44  Wheel combination numbers                    H=120
    Y=164 Progress bar + count / percentage            H=80
    Y=244 [EMERGENCY STOP] | [PAUSE/RESUME] buttons    H=76
*/

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ─── Board hardware configuration ───────────────────────────────────────────

class LGFX_WT32SC01Plus : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796  _panel_instance;
  lgfx::Bus_Parallel8 _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_FT5x06  _touch_instance;

public:
  LGFX_WT32SC01Plus(void) {
    {
      auto cfg       = _bus_instance.config();
      cfg.port       = 0;
      cfg.freq_write = 40000000;
      cfg.pin_wr     = 47;
      cfg.pin_rd     = -1;
      cfg.pin_rs     = 0;   // DC / data-command select
      cfg.pin_d0     = 9;
      cfg.pin_d1     = 46;
      cfg.pin_d2     = 3;
      cfg.pin_d3     = 8;
      cfg.pin_d4     = 18;
      cfg.pin_d5     = 17;
      cfg.pin_d6     = 16;
      cfg.pin_d7     = 15;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg              = _panel_instance.config();
      cfg.pin_cs            = -1;   // CS tied low on board
      cfg.pin_rst           = 4;
      cfg.pin_busy          = -1;
      cfg.panel_width       = 320;
      cfg.panel_height      = 480;
      cfg.offset_x          = 0;
      cfg.offset_y          = 0;
      cfg.offset_rotation   = 0;
      cfg.dummy_read_pixel  = 8;
      cfg.dummy_read_bits   = 1;
      cfg.readable          = false;
      cfg.invert            = true;
      cfg.rgb_order         = false;
      cfg.dlen_16bit        = false;
      cfg.bus_shared        = false;
      _panel_instance.config(cfg);
    }
    {
      auto cfg        = _light_instance.config();
      cfg.pin_bl      = 45;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    {
      auto cfg              = _touch_instance.config();
      cfg.x_min             = 0;
      cfg.x_max             = 319;
      cfg.y_min             = 0;
      cfg.y_max             = 479;
      cfg.pin_int           = 7;
      cfg.bus_shared        = false;
      cfg.offset_rotation   = 0;  // adjust if touch X/Y are swapped after testing
      cfg.i2c_port          = 0;
      cfg.i2c_addr          = 0x38;
      cfg.pin_sda           = 6;
      cfg.pin_scl           = 5;
      cfg.freq              = 400000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};

static LGFX_WT32SC01Plus lcd;

// ─── Layout constants ────────────────────────────────────────────────────────

static const int UI_W      = 480;
static const int UI_H      = 320;
static const int HDR_H     = 44;
static const int COMBO_Y   = HDR_H;
static const int COMBO_H   = 120;
static const int PROG_Y    = COMBO_Y + COMBO_H;
static const int PROG_H    = 80;
static const int BTN_Y     = PROG_Y + PROG_H;
static const int BTN_H     = UI_H - BTN_Y;
static const int BTN_MID   = UI_W / 2;

// ─── Colour palette (24-bit RGB) ─────────────────────────────────────────────

static const uint32_t C_BG        = 0x10131A;
static const uint32_t C_HEADER    = 0x1A1F2E;
static const uint32_t C_AMBER     = 0xFBBF24;
static const uint32_t C_GREEN     = 0x22C55E;
static const uint32_t C_RED       = 0xEF4444;
static const uint32_t C_GRAY      = 0x64748B;
static const uint32_t C_WHITE     = 0xF8FAFC;
static const uint32_t C_BAR_TRACK = 0x334155;
static const uint32_t C_BTN_STOP  = 0x7F1D1D;
static const uint32_t C_BTN_PAUSE = 0x1E3A5F;
static const uint32_t C_DIVIDER   = 0x334155;

// ─── State for change-detection ──────────────────────────────────────────────

static String  _dStatus       = "";
static float   _dWheels[4]    = { -999, -999, -999, -999 };
static int     _dWheelCount   = -1;
static int     _dCurrent      = -1;
static int     _dTotal        = -1;
static bool    _dPaused       = false;
static bool    _dReady        = false;

// ─── Private drawing helpers ─────────────────────────────────────────────────

static void _drawStatusBadge(const String& status) {
  const int BADGE_X = UI_W - 178;
  const int BADGE_Y = 6;
  const int BADGE_W = 170;
  const int BADGE_H = HDR_H - 12;

  lcd.fillRect(BADGE_X, 0, BADGE_W, HDR_H, C_HEADER);

  uint32_t col;
  if      (status == "Waiting")        col = C_GRAY;
  else if (status == "EMERGENCY STOP") col = C_RED;
  else if (status == "PAUSED")         col = C_AMBER;
  else if (status == "Calibrating")    col = C_AMBER;
  else                                 col = C_GREEN;

  String label = status.startsWith("Trying") ? "Trying..." : status;

  lcd.fillRoundRect(BADGE_X, BADGE_Y, BADGE_W, BADGE_H, 7, col);
  lcd.setFont(&lgfx::fonts::Font2);
  lcd.setTextColor(C_BG);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString(label, BADGE_X + BADGE_W / 2, HDR_H / 2);
}

static void _drawCombo(float* nums, int count) {
  lcd.fillRect(0, COMBO_Y, UI_W, COMBO_H, C_BG);
  if (count <= 0) return;

  int colW = UI_W / count;

  for (int i = 0; i < count; i++) {
    int cx = colW * i + colW / 2;
    int bx = colW * i + 10;
    int bw = colW - 20;
    int by = COMBO_Y + 34;
    int bh = 64;

    // "WHEEL N" label above the box
    lcd.setFont(&lgfx::fonts::Font2);
    lcd.setTextColor(C_GRAY);
    lcd.setTextDatum(lgfx::middle_center);
    lcd.drawString("WHEEL " + String(i + 1), cx, COMBO_Y + 18);

    // Number box
    lcd.fillRoundRect(bx, by, bw, bh, 8, C_HEADER);
    lcd.drawRoundRect(bx, by, bw, bh, 8, C_AMBER);

    // Number value — font chosen by wheel count so text fits the box
    lcd.setTextColor(C_WHITE);
    lcd.setTextDatum(lgfx::middle_center);
    if (count <= 2) {
      lcd.setFont(&lgfx::fonts::Font7);   // large 7-segment style
    } else if (count == 3) {
      lcd.setFont(&lgfx::fonts::Font6);   // medium 7-segment style
    } else {
      lcd.setFont(&lgfx::fonts::Font4);
    }
    lcd.drawString(String(nums[i], 1), cx, by + bh / 2);
  }
}

static void _drawProgress(int current, int total) {
  lcd.fillRect(0, PROG_Y + 1, UI_W, PROG_H - 1, C_BG);
  if (total <= 0) return;

  const int bx = 16;
  const int bw = UI_W - 32;
  const int bh = 22;
  const int by = PROG_Y + 12;

  // Track and fill
  lcd.fillRoundRect(bx, by, bw, bh, bh / 2, C_BAR_TRACK);
  float pct   = constrain((float)current / (float)total, 0.0f, 1.0f);
  int   fillW = (int)(bw * pct);
  if (fillW > 0)
    lcd.fillRoundRect(bx, by, fillW, bh, bh / 2, C_AMBER);

  // Count and percentage labels
  lcd.setFont(&lgfx::fonts::Font2);
  lcd.setTextColor(C_WHITE);
  lcd.setTextDatum(lgfx::middle_left);
  lcd.drawString(String(current) + " / " + String(total), bx, PROG_Y + 54);
  lcd.setTextDatum(lgfx::middle_right);
  lcd.drawString(String(pct * 100.0f, 1) + "%", bx + bw, PROG_Y + 54);
}

static void _drawPauseButton(bool paused) {
  lcd.fillRect(BTN_MID + 1, BTN_Y + 1, UI_W - BTN_MID - 1, BTN_H - 1, C_BTN_PAUSE);
  lcd.setFont(&lgfx::fonts::Font4);
  lcd.setTextColor(C_WHITE);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString(paused ? "RESUME" : "PAUSE",
                 BTN_MID + (UI_W - BTN_MID) / 2,
                 BTN_Y + BTN_H / 2);
}

static void _drawStaticChrome() {
  lcd.fillScreen(C_BG);

  // Header bar
  lcd.fillRect(0, 0, UI_W, HDR_H, C_HEADER);
  lcd.setFont(&lgfx::fonts::Font4);
  lcd.setTextColor(C_AMBER);
  lcd.setTextDatum(lgfx::middle_left);
  lcd.drawString("AUTO DIALER", 12, HDR_H / 2);

  // Horizontal dividers
  lcd.drawFastHLine(0, HDR_H,   UI_W, C_DIVIDER);
  lcd.drawFastHLine(0, PROG_Y,  UI_W, C_DIVIDER);
  lcd.drawFastHLine(0, BTN_Y,   UI_W, C_DIVIDER);

  // Button backgrounds and centre divider
  lcd.fillRect(0,          BTN_Y, BTN_MID,        BTN_H, C_BTN_STOP);
  lcd.fillRect(BTN_MID + 1, BTN_Y, UI_W - BTN_MID - 1, BTN_H, C_BTN_PAUSE);
  lcd.drawFastVLine(BTN_MID, BTN_Y, BTN_H, C_DIVIDER);

  // Button labels
  lcd.setFont(&lgfx::fonts::Font4);
  lcd.setTextColor(C_WHITE);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString("EMERGENCY STOP",   BTN_MID / 2,              BTN_Y + BTN_H / 2);
  lcd.drawString("PAUSE",            BTN_MID + (UI_W - BTN_MID) / 2, BTN_Y + BTN_H / 2);
}

// ─── Public API ──────────────────────────────────────────────────────────────

void DisplayInit() {
  lcd.init();
  lcd.setRotation(1);   // landscape: 480 wide × 320 tall
  lcd.setBrightness(200);
  _drawStaticChrome();
  _drawStatusBadge("Waiting");
  _dStatus = "Waiting";
  _dReady  = true;
}

// Called from SendStatus() after each combination attempt.
void DisplayUpdateStatus(const String& status, float* wheelNums,
                         int wheelCount, int current, int total) {
  if (!_dReady) return;

  if (status != _dStatus) {
    _drawStatusBadge(status);
    _dStatus = status;
  }

  bool wheelsChanged = (wheelCount != _dWheelCount);
  if (!wheelsChanged) {
    for (int i = 0; i < wheelCount; i++) {
      if (fabsf(wheelNums[i] - _dWheels[i]) > 0.001f) { wheelsChanged = true; break; }
    }
  }
  if (wheelsChanged) {
    _drawCombo(wheelNums, wheelCount);
    _dWheelCount = wheelCount;
    for (int i = 0; i < wheelCount; i++) _dWheels[i] = wheelNums[i];
  }

  if (current != _dCurrent || total != _dTotal) {
    _drawProgress(current, total);
    _dCurrent = current;
    _dTotal   = total;
  }
}

// Called from SendStatusSimple() for Waiting / EMERGENCY STOP / etc.
void DisplayUpdateSimple(const String& status) {
  if (!_dReady) return;

  if (status != _dStatus) {
    _drawStatusBadge(status);
    _dStatus = status;
  }

  // Clear combo and progress when returning to an idle state
  if (status == "Waiting" || status == "EMERGENCY STOP" || status == "Calibrating") {
    if (_dWheelCount != 0) {
      lcd.fillRect(0, COMBO_Y, UI_W, COMBO_H, C_BG);
      _dWheelCount = 0;
    }
    if (_dCurrent != 0 || _dTotal != 0) {
      lcd.fillRect(0, PROG_Y + 1, UI_W, PROG_H - 1, C_BG);
      _dCurrent = 0;
      _dTotal   = 0;
    }
    if (status == "EMERGENCY STOP" && _dPaused) {
      _dPaused = false;
      _drawPauseButton(false);
    }
  }
}

// Called from loop() — non-blocking touch poll.
// Sets *emergStop or toggles *pauseFlag when a button zone is tapped.
void DisplayCheckTouch(volatile bool* emergStop, bool* pauseFlag) {
  if (!_dReady) return;

  uint16_t tx, ty;
  if (!lcd.getTouch(&tx, &ty)) return;
  if (ty < (uint16_t)BTN_Y)   return;

  // 300 ms debounce
  static unsigned long lastMs = 0;
  if (millis() - lastMs < 300) return;
  lastMs = millis();

  if (tx < (uint16_t)BTN_MID) {
    // Emergency Stop
    *emergStop = true;
    _drawStatusBadge("EMERGENCY STOP");
    _dStatus = "EMERGENCY STOP";
  } else {
    // Pause / Resume toggle
    *pauseFlag = !(*pauseFlag);
    _dPaused   = *pauseFlag;
    _drawPauseButton(_dPaused);
    if (_dPaused) {
      _drawStatusBadge("PAUSED");
      _dStatus = "PAUSED";
    }
    // On resume the next SendStatus call restores the status badge
  }
}

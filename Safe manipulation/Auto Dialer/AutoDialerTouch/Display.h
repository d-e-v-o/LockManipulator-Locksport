#pragma once

/*
  WT32-SC01 Plus onboard display, touch, and audio driver.

  Display: ST7796 480×320, parallel 8-bit (Intel 8080)
  Touch:   FT5x06 capacitive, I2C
  Audio:   onboard IIS amplifier (2.5W @ 4R)

  Layout (landscape, 480×320):
    Y=0   Header "AUTO DIALER" | [VOL] | status badge     H=44
    Y=44  Wheel combination numbers                        H=120
    Y=164 Progress bar + count / percentage                H=80
    Y=244 [EMERGENCY STOP] | [PAUSE/RESUME] buttons        H=76
*/

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "driver/i2s_std.h"

// ─── Audio hardware ──────────────────────────────────────────────────────────
// Onboard IIS amplifier pins, per ZX3D50CE02S-USRC-4832 datasheet Tab.7.
// These GPIOs are free only on -R2 (quad PSRAM) modules, which this board uses.
#define AUDIO_WS_PIN     35   // LRCK (word select)
#define AUDIO_BCK_PIN    36   // BCLK (bit clock)
#define AUDIO_DATA_PIN   37   // DOUT (serial data to amplifier)
#define AUDIO_SAMPLE_RATE 16000

// ─── Board display/touch configuration ──────────────────────────────────────

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
      cfg.pin_rs     = 0;
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
      cfg.pin_cs            = -1;
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
      cfg.offset_rotation   = 0;
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

// Volume button sits in the header bar between the title and status badge
static const int VOL_BTN_X = 175;
static const int VOL_BTN_W = 105;
static const int VOL_BTN_H = 30;
static const int VOL_BTN_Y = (HDR_H - VOL_BTN_H) / 2;

// Status badge is pushed right of the volume button
static const int BADGE_X   = UI_W - 168;
static const int BADGE_W   = 160;

// ─── Colour palette ──────────────────────────────────────────────────────────

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
static const uint32_t C_VOL_BTN   = 0x1E293B;

// ─── State ───────────────────────────────────────────────────────────────────

static String  _dStatus      = "";
static float   _dWheels[4]   = { -999, -999, -999, -999 };
static int     _dWheelCount  = -1;
static int     _dCurrent     = -1;
static int     _dTotal       = -1;
static bool    _dPaused      = false;
static bool    _dReady       = false;
static bool    _dStopped     = false;   // big "STOPPED!" banner currently shown

// Volume: index into {0, 25, 50, 75, 100}
static int     _volIdx       = 3;   // default (75% label)
static float   _volume       = 0.45f;   // must match levels[3] in _cycleVolume

static i2s_chan_handle_t _i2sTx = NULL;

// Concurrency guards. The touch handler runs on a separate FreeRTOS task (core 0)
// while status updates and the chime are issued from the motor context (core 1).
// _lgfxMux serializes all LovyanGFX access (drawing + getTouch); _audioMux
// serializes I2S writes so a button click can't collide with the open chime.
static SemaphoreHandle_t _lgfxMux  = NULL;
static SemaphoreHandle_t _audioMux = NULL;

// Web-mirror announce codes. The touch task cannot call events.send() safely
// (AsyncTCP must be driven from the Arduino task/core 1), so it records what
// happened here and the sketch pushes it to the web SSE from core 1.
#define WEB_ANN_NONE   0
#define WEB_ANN_ESTOP  1
#define WEB_ANN_PAUSED 2

// Flag pointers into the sketch, wired up by DisplayStartInput().
static volatile bool*    _pEmerg    = NULL;
static bool*             _pPause    = NULL;
static volatile uint8_t* _pAnnounce = NULL;

// ─── Audio ───────────────────────────────────────────────────────────────────

void AudioInit() {
  if (!_audioMux) _audioMux = xSemaphoreCreateMutex();
  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  chanCfg.auto_clear = true;
  if (i2s_new_channel(&chanCfg, &_i2sTx, NULL) != ESP_OK) {
    _i2sTx = NULL;
    return;
  }

  i2s_std_config_t stdCfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)AUDIO_BCK_PIN,
      .ws   = (gpio_num_t)AUDIO_WS_PIN,
      .dout = (gpio_num_t)AUDIO_DATA_PIN,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
    },
  };
  i2s_channel_init_std_mode(_i2sTx, &stdCfg);
  i2s_channel_enable(_i2sTx);
}

// Plays a sine tone at freqHz for durationMs with a cosine fade-out envelope.
static void _playTone(int freqHz, int durationMs) {
  if (_volume < 0.001f || _i2sTx == NULL) return;
  if (_audioMux) xSemaphoreTake(_audioMux, portMAX_DELAY);

  const int sr           = AUDIO_SAMPLE_RATE;
  const int totalSamples = sr * durationMs / 1000;
  const int BUF          = 128;
  int16_t   buf[BUF * 2];
  int       idx = 0;

  while (idx < totalSamples) {
    int chunk = min(BUF, totalSamples - idx);
    for (int i = 0; i < chunk; i++) {
      float progress = (float)(idx + i) / totalSamples;
      float env = (progress > 0.65f)
                    ? 0.5f * (1.0f + cosf(M_PI * (progress - 0.65f) / 0.35f))
                    : 1.0f;
      float val = sinf(2.0f * M_PI * freqHz * (idx + i) / (float)sr);
      int16_t s = (int16_t)(val * env * _volume * 26000.0f);
      buf[i * 2]     = s;
      buf[i * 2 + 1] = s;
    }
    size_t written;
    i2s_channel_write(_i2sTx, buf, chunk * 4, &written, portMAX_DELAY);
    idx += chunk;
  }
  if (_audioMux) xSemaphoreGive(_audioMux);
}

// Short click for button feedback.
void AudioClick() {
  _playTone(1100, 22);
}

// 3-note ascending chime played when a lock open is detected.
void AudioChime() {
  _playTone(523, 280);   // C5
  delay(40);
  _playTone(659, 280);   // E5
  delay(40);
  _playTone(784, 420);   // G5
}

// ─── Drawing helpers ─────────────────────────────────────────────────────────

static void _drawStatusBadge(const String& status) {
  lcd.fillRect(BADGE_X, 0, BADGE_W + 8, HDR_H, C_HEADER);

  uint32_t col;
  if      (status == "Waiting")        col = C_GRAY;
  else if (status == "EMERGENCY STOP") col = C_RED;
  else if (status == "PAUSED")         col = C_AMBER;
  else if (status == "Calibrating")    col = C_AMBER;
  else                                 col = C_GREEN;

  String label = status.startsWith("Trying") ? "Trying..." : status;

  lcd.fillRoundRect(BADGE_X, 7, BADGE_W, HDR_H - 14, 7, col);
  lcd.setFont(&lgfx::fonts::Font2);
  lcd.setTextColor(C_BG);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString(label, BADGE_X + BADGE_W / 2, HDR_H / 2);
}

static void _drawVolumeButton() {
  static const int pcts[] = { 0, 25, 50, 75, 100 };
  int pct = pcts[_volIdx];

  lcd.fillRoundRect(VOL_BTN_X, VOL_BTN_Y, VOL_BTN_W, VOL_BTN_H, 6, C_VOL_BTN);
  lcd.drawRoundRect(VOL_BTN_X, VOL_BTN_Y, VOL_BTN_W, VOL_BTN_H, 6, C_GRAY);

  String label = (pct == 0) ? "VOL: MUTE" : ("VOL: " + String(pct) + "%");
  lcd.setFont(&lgfx::fonts::Font2);
  lcd.setTextColor(pct == 0 ? C_GRAY : C_AMBER);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString(label, VOL_BTN_X + VOL_BTN_W / 2, VOL_BTN_Y + VOL_BTN_H / 2);
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

    lcd.setFont(&lgfx::fonts::Font2);
    lcd.setTextColor(C_GRAY);
    lcd.setTextDatum(lgfx::middle_center);
    lcd.drawString("WHEEL " + String(i + 1), cx, COMBO_Y + 18);

    lcd.fillRoundRect(bx, by, bw, bh, 8, C_HEADER);
    lcd.drawRoundRect(bx, by, bw, bh, 8, C_AMBER);

    lcd.setTextColor(C_WHITE);
    lcd.setTextDatum(lgfx::middle_center);
    if      (count <= 2) lcd.setFont(&lgfx::fonts::Font7);
    else if (count == 3) lcd.setFont(&lgfx::fonts::Font6);
    else                 lcd.setFont(&lgfx::fonts::Font4);
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

  lcd.fillRoundRect(bx, by, bw, bh, bh / 2, C_BAR_TRACK);
  float pct   = constrain((float)current / (float)total, 0.0f, 1.0f);
  int   fillW = (int)(bw * pct);
  if (fillW > 0)
    lcd.fillRoundRect(bx, by, fillW, bh, bh / 2, C_AMBER);

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

  lcd.fillRect(0, 0, UI_W, HDR_H, C_HEADER);
  lcd.setFont(&lgfx::fonts::Font4);
  lcd.setTextColor(C_AMBER);
  lcd.setTextDatum(lgfx::middle_left);
  lcd.drawString("AUTO DIALER", 12, HDR_H / 2);

  _drawVolumeButton();

  lcd.drawFastHLine(0, HDR_H,  UI_W, C_DIVIDER);
  lcd.drawFastHLine(0, PROG_Y, UI_W, C_DIVIDER);
  lcd.drawFastHLine(0, BTN_Y,  UI_W, C_DIVIDER);

  lcd.fillRect(0,           BTN_Y, BTN_MID,             BTN_H, C_BTN_STOP);
  lcd.fillRect(BTN_MID + 1, BTN_Y, UI_W - BTN_MID - 1, BTN_H, C_BTN_PAUSE);
  lcd.drawFastVLine(BTN_MID, BTN_Y, BTN_H, C_DIVIDER);

  lcd.setFont(&lgfx::fonts::Font4);
  lcd.setTextColor(C_WHITE);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString("EMERGENCY STOP", BTN_MID / 2,              BTN_Y + BTN_H / 2);
  lcd.drawString("PAUSE",          BTN_MID + (UI_W - BTN_MID) / 2, BTN_Y + BTN_H / 2);
}

// ─── Public API ──────────────────────────────────────────────────────────────

void DisplayInit() {
  if (!_lgfxMux) _lgfxMux = xSemaphoreCreateMutex();
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(200);
  _drawStaticChrome();
  _drawStatusBadge("Waiting");
  _dStatus = "Waiting";
  _dReady  = true;
}

// Boot-time one-line diagnostic drawn in the combo area (overwritten on first run).
// Used to surface the TMC5160 SPI self-test result on the screen at startup.
void DisplayBootLine(const String& msg, bool warn) {
  if (!_dReady) return;
  if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);
  lcd.fillRect(0, COMBO_Y, UI_W, COMBO_H, C_BG);
  lcd.setFont(&lgfx::fonts::Font4);
  lcd.setTextColor(warn ? C_RED : C_GREEN);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString(msg, UI_W / 2, COMBO_Y + COMBO_H / 2);
  if (_lgfxMux) xSemaphoreGive(_lgfxMux);
}

// ─── Boot diagnostic panel ───────────────────────────────────────────────────
// Multi-line register dump drawn in the combo+progress area so the TMC5160
// health can be read at the bench with no serial cable. Persists until the first
// run redraws those areas.

static int _diagY = 0;

// Clears the panel area and draws the title; call before DisplayDiagLine().
void DisplayDiagBegin(const String& title) {
  if (!_dReady) return;
  if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);
  lcd.fillRect(0, COMBO_Y, UI_W, PROG_Y + PROG_H - COMBO_Y, C_BG);
  lcd.setFont(&lgfx::fonts::Font2);
  lcd.setTextColor(C_AMBER);
  lcd.setTextDatum(lgfx::top_left);
  lcd.drawString(title, 16, COMBO_Y + 8);
  if (_lgfxMux) xSemaphoreGive(_lgfxMux);
  _diagY = COMBO_Y + 22;
}

// Draws one "label            value" row; value shows red when warn is true.
void DisplayDiagLine(const String& label, const String& value, bool warn) {
  if (!_dReady) return;
  if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);
  lcd.setFont(&lgfx::fonts::Font2);
  lcd.setTextDatum(lgfx::top_left);
  lcd.setTextColor(C_GRAY);
  lcd.drawString(label, 24, _diagY);
  lcd.setTextDatum(lgfx::top_right);
  lcd.setTextColor(warn ? C_RED : C_GREEN);
  lcd.drawString(value, UI_W - 24, _diagY);
  if (_lgfxMux) xSemaphoreGive(_lgfxMux);
  _diagY += 16;
}

void DisplayUpdateStatus(const String& status, float* wheelNums,
                         int wheelCount, int current, int total) {
  if (!_dReady) return;
  if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);

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
    _drawCombo(wheelNums, wheelCount);   // overwrites any STOPPED! banner
    _dWheelCount = wheelCount;
    _dStopped    = false;
    for (int i = 0; i < wheelCount; i++) _dWheels[i] = wheelNums[i];
  }

  if (current != _dCurrent || total != _dTotal) {
    _drawProgress(current, total);
    _dCurrent = current;
    _dTotal   = total;
  }
  if (_lgfxMux) xSemaphoreGive(_lgfxMux);
}

// Big red "STOPPED!" banner filling the combo area on emergency stop, so the
// halt is obvious from across the shop, not just the corner status badge.
static void _drawStoppedBanner() {
  lcd.fillRect(0, COMBO_Y, UI_W, COMBO_H, C_BTN_STOP);   // dark-red band
  lcd.setFont(&lgfx::fonts::Font4);
  lcd.setTextSize(3);
  lcd.setTextColor(C_WHITE);
  lcd.setTextDatum(lgfx::middle_center);
  lcd.drawString("STOPPED!", UI_W / 2, COMBO_Y + COMBO_H / 2);
  lcd.setTextSize(1);   // reset so later text isn't scaled
}

void DisplayUpdateSimple(const String& status) {
  if (!_dReady) return;
  if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);

  if (status != _dStatus) {
    _drawStatusBadge(status);
    _dStatus = status;
  }

  if (status == "EMERGENCY STOP") {
    if (!_dStopped) {
      _drawStoppedBanner();
      _dStopped = true;
    }
    _dWheelCount = 0;   // banner replaced the combo readout
    if (_dCurrent != 0 || _dTotal != 0) {
      lcd.fillRect(0, PROG_Y + 1, UI_W, PROG_H - 1, C_BG);
      _dCurrent = 0;
      _dTotal   = 0;
    }
    if (_dPaused) {
      _dPaused = false;
      _drawPauseButton(false);
    }
  }
  else if (status == "Waiting" || status == "Calibrating") {
    // Clear the combo area, including a lingering STOPPED! banner.
    if (_dWheelCount != 0 || _dStopped) {
      lcd.fillRect(0, COMBO_Y, UI_W, COMBO_H, C_BG);
      _dWheelCount = 0;
      _dStopped    = false;
    }
    if (_dCurrent != 0 || _dTotal != 0) {
      lcd.fillRect(0, PROG_Y + 1, UI_W, PROG_H - 1, C_BG);
      _dCurrent = 0;
      _dTotal   = 0;
    }
  }
  if (_lgfxMux) xSemaphoreGive(_lgfxMux);
}

// Handle one touch sample. Runs on the input task (core 0). Reads the panel and
// sets the sketch's emergency-stop / pause flags — the same flags the async web
// handlers set — then draws immediate button feedback. All LovyanGFX access is
// serialized through _lgfxMux; audio is played outside that lock to avoid
// nesting with the motor-context chime.
static void _handleTouch() {
  if (!_dReady) return;

  uint16_t tx, ty;
  bool got;
  if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);
  got = lcd.getTouch(&tx, &ty);
  if (_lgfxMux) xSemaphoreGive(_lgfxMux);
  if (!got) return;

  static unsigned long lastMs = 0;
  if (millis() - lastMs < 300) return;
  lastMs = millis();

  // Header zone: volume button
  if (ty < (uint16_t)HDR_H) {
    if (tx >= (uint16_t)VOL_BTN_X && tx <= (uint16_t)(VOL_BTN_X + VOL_BTN_W)) {
      // Amplitude scalars behind the 0/25/50/75/100 labels. The top two are held
      // well below 1.0: the small speaker + NS4168 amp saturates above ~0.6, so
      // old 75% and 100% both clipped and sounded identical. 25%/50% were fine.
      static const float levels[] = { 0.0f, 0.15f, 0.30f, 0.45f, 0.60f };
      _volIdx = (_volIdx + 1) % 5;
      _volume = levels[_volIdx];
      if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);
      _drawVolumeButton();
      if (_lgfxMux) xSemaphoreGive(_lgfxMux);
      if (_volume > 0.001f) AudioClick();
    }
    return;
  }

  // Bottom button zone
  if (ty >= (uint16_t)BTN_Y) {
    AudioClick();
    if (tx < (uint16_t)BTN_MID) {
      if (_pEmerg)    *_pEmerg = true;
      if (_pAnnounce) *_pAnnounce = WEB_ANN_ESTOP;
      if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);
      _drawStatusBadge("EMERGENCY STOP");
      _dStatus = "EMERGENCY STOP";
      if (_lgfxMux) xSemaphoreGive(_lgfxMux);
    } else {
      if (_pPause) *_pPause = !(*_pPause);
      _dPaused = _pPause ? *_pPause : false;
      // Only announce PAUSED; on resume the run's next status update mirrors itself.
      if (_pAnnounce && _dPaused) *_pAnnounce = WEB_ANN_PAUSED;
      if (_lgfxMux) xSemaphoreTake(_lgfxMux, portMAX_DELAY);
      _drawPauseButton(_dPaused);
      if (_dPaused) {
        _drawStatusBadge("PAUSED");
        _dStatus = "PAUSED";
      }
      if (_lgfxMux) xSemaphoreGive(_lgfxMux);
    }
  }
}

// Input task — polls touch every 40 ms on core 0, independent of the motor loop
// on core 1, so buttons stay responsive during a run with zero effect on step
// timing. This mirrors how the AsyncWebServer task sets the same flags.
static void _touchTaskFn(void* arg) {
  for (;;) {
    _handleTouch();
    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

// Start touch input handling. Call once from setup() after DisplayInit/AudioInit,
// passing the sketch's emergency-stop and pause flags plus a web-announce byte
// the sketch drains from core 1 (see PumpWebAnnounce).
void DisplayStartInput(volatile bool* emergStop, bool* pauseFlag,
                       volatile uint8_t* announce) {
  _pEmerg    = emergStop;
  _pPause    = pauseFlag;
  _pAnnounce = announce;
  xTaskCreatePinnedToCore(_touchTaskFn, "touch", 4096, NULL, 1, NULL, 0);
}

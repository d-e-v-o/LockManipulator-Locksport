# Plan: WT32-SC01 Plus Touchscreen UI

## Context
The Auto Dialer project targets an ESP32-S3 and controls a TMC5160 stepper driver over SPI. The WT32-SC01 Plus board uses the same MCU but its onboard 480×320 display (parallel 8-bit) and FT5x06 touch controller occupy GPIOs that conflict with the current motor driver wiring. The goal is to:
1. Remap motor driver pins to conflict-free GPIOs
2. Add a new `Display.h` that drives the onboard screen with live stats (status, current combination, progress) and two touch buttons (Emergency Stop, Pause/Resume)

---

## Pin Remapping (WT32-SC01 Plus reserved → new motor pins)

**Reserved by board hardware:**
| Function | GPIOs |
|---|---|
| Display 8-bit parallel + control | 0, 3, 4, 8, 9, 15–18, 45, 46, 47 |
| Touch FT5x06 (I2C) | 5, 6, 7 |
| Backlight | 45 |

**Available GPIOs** (SD card slot unused, debug UART0 free because USB CDC is enabled):
- SD card slot: 10, 11, 12, 13
- Exposed header: 14, 21
- Debug port UART0 (TXD0/RXD0): 43, 44  ← free because Serial uses USB CDC, not UART0

**New motor driver pin assignments:**
```cpp
#define STEP_PIN   10
#define DIR_PIN    21
#define MOSI_PIN   11
#define MISO_PIN   12
#define SCK_PIN    13
#define CS_PIN     14
#define DIAG0_PIN  43   // debug port pin 3 (TXD0), free when USB CDC is active
// DIAG1_PIN removed — was defined but never referenced in code
```

---

## Library
**LovyanGFX** — required for WT32-SC01 Plus because its ST7796 display uses a parallel 8-bit (Intel 8080) interface that TFT_eSPI does not support. Install via Arduino Library Manager: `LovyanGFX`.

---

## New File: `Display.h`

### LovyanGFX panel configuration
```cpp
class LGFX_WT32SC01Plus : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796    _panel;
  lgfx::Bus_Parallel8   _bus;
  lgfx::Light_PWM       _light;
  lgfx::Touch_FT5x06    _touch;
  // Bus: WR=47, DC=0, RST=4, D0–D7 = 9,46,3,8,18,17,16,15
  // Touch: SDA=6, SCL=5, INT=7, addr=0x38, I2C port 0
  // Backlight: pin=45, PWM ch=7
  // Panel: 320×480 physical, rotation=1 (landscape 480×320)
};
```

### Screen layout (480×320 landscape)
```
┌────────────────────────────────────────────────┐  Y=0
│  AUTO DIALER                       [ STATUS ]  │  H=44 header
├────────────────────────────────────────────────┤  Y=44
│                                                │
│   Wheel 1         Wheel 2         Wheel 3      │  H=120 combo area
│  ┌────────┐      ┌────────┐      ┌────────┐   │  (up to 4 wheels,
│  │  25.0  │      │  50.0  │      │  75.0  │   │   font scaled to fit)
│  └────────┘      └────────┘      └────────┘   │
├────────────────────────────────────────────────┤  Y=164
│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░  234 / 1250      │  H=80 progress
│                                      62.5%     │
├──────────────────────┬─────────────────────────┤  Y=244
│   EMERGENCY STOP     │    PAUSE / RESUME       │  H=76 touch buttons
└──────────────────────┴─────────────────────────┘  Y=320
```

### Functions exposed by Display.h
```cpp
void DisplayInit();
// Called from setup(). Initialises LovyanGFX, draws static chrome.

void DisplayUpdateStatus(const String& status, float* wheelNums,
                         int wheelCount, int current, int total);
// Called from SendStatus(). Redraws status badge, combo numbers,
// and progress bar. Skips unchanged regions to avoid flicker.

void DisplayUpdateSimple(const String& status);
// Called from SendStatusSimple(). Redraws status badge only
// (clears combo area when status is Waiting/Emergency Stop).

void DisplayCheckTouch(volatile bool* emergStop, bool* pauseFlag);
// Called at top of loop(). Non-blocking getTouch(). If touch falls
// in bottom-left zone → set *emergStop = true.
// If touch falls in bottom-right zone → toggle *pauseFlag.
```

### State tracked inside Display.h (file-static)
- `prevStatus`, `prevWheelNums[]`, `prevWheelCount`, `prevCurrent`, `prevTotal`  
  Used to skip no-op redraws.
- `pausedState` bool — tracks current Pause/Resume label on button.

---

## Changes to `AutoDialer.ino`

1. **Pin definitions** — replace the 8 `#define` lines with the new values listed above.
2. **Include** — add `#include "Display.h"` after existing includes.
3. **`setup()`** — add `DisplayInit();` as the first call (before `WifiSetup`).
4. **`SendStatus()`** — append `DisplayUpdateStatus(status, wheelNums, wheelCount, current, total);`
5. **`SendStatusSimple()`** — append `DisplayUpdateSimple(status);`
6. **`loop()`** — add `DisplayCheckTouch(&emergencyStop, &pauseDialing);` as the first line in the function body.

No changes to motor logic, web server, or dialing algorithms.

---

## Colour scheme (matches web UI amber-on-dark)
| Element | Hex |
|---|---|
| Background | `0x10131A` |
| Header bar | `0x1A1F2E` |
| Status — Waiting | `0x64748B` (gray) |
| Status — Trying | `0x22C55E` (green) |
| Status — Emergency | `0xEF4444` (red) |
| Status — Calibrating | `0xFBBF24` (amber) |
| Wheel number text | `0xF8FAFC` (white) |
| Progress bar fill | `0xFBBF24` (amber) |
| Progress bar track | `0x334155` (slate) |
| Stop button | `0x7F1D1D` (dark red) |
| Pause button | `0x1E3A5F` (dark blue) |

---

## Verification
1. Compile in Arduino IDE with board set to **ESP32S3 Dev Module** (same settings as current header comment).
2. Confirm no linker errors after adding LovyanGFX.
3. Flash to WT32-SC01 Plus and verify display initialises with the layout.
4. Use web UI to start a dial sequence; confirm screen updates status and combo in real time.
5. Tap Emergency Stop button on screen; confirm `emergencyStop` fires (motor stops, web UI shows EMERGENCY STOP).
6. Tap Pause/Resume; confirm dialing pauses and label toggles.
# AutoDialer — Bench Wiring Checklist (WT32-SC01 Plus build)

Board: **ZX3D50CE02S-USRC-4832** (WT32-S3-WROVER **N8R2**, quad PSRAM).
Header names + physical pin numbers are from the module datasheet
(Debug = Tab.1, Extended IO = Tab.2, Speaker = Tab.3, Audio IIS = Tab.7).

> GPIO 10–13 live on the **Extended IO Interface**, NOT the SD-card header
> (the SD interface is GPIO 38–41). All motor logic lines are on this one header.

---

## 1. Power distribution

| From | To | Rail | ✓ |
|------|----|------|---|
| AC mains L/N | Rocker switch (P1) → PSU L/N | mains | ☐ |
| PSU **+24V** | TMC **VMOT (16)** | +24V | ☐ |
| PSU **+5V** | WT32 **Extended IO pin 1 (+5V)** — powers the board | +5V | ☐ |
| PSU **+5V** | AMS1117 **VIN** | +5V | ☐ |
| AMS1117 **3.3V OUT** | TMC **VIO (10)** | +3.3V | ☐ |
| PSU **+12V** | *(unused — leave unconnected)* | — | ☐ |

Caps: **100nF** across TMC VIO→GND at the driver; **10µF** in + **10µF** out at the AMS1117.

> Power the WT32 from **either** Extended IO pin 1 **or** USB-C — not both at once.

---

## 2. Logic signals — WT32 → TMC5160

| Signal | GPIO | WT32 header (pin) | TMC pin | ✓ |
|--------|------|-------------------|---------|---|
| STEP  | IO10 | Extended IO (pin 3) | 7  | ☐ |
| MOSI  | IO11 | Extended IO (pin 4) | 2  | ☐ |
| MISO  | IO12 | Extended IO (pin 5) | 5  | ☐ |
| SCK   | IO13 | Extended IO (pin 6) | 3  | ☐ |
| CS    | IO14 | Extended IO (pin 7) | 4  | ☐ |
| DIR   | IO21 | Extended IO (pin 8) | 8  | ☐ |
| DIAG0 | IO43 | Debug Interface (pin 3, ESP_TXD/TXD0) | DIAG0 | ☐ |
| DIAG1 | —    | — | *(n/c, leave floating)* | ☐ |

> DIAG0 comes off the **Debug** header, so tie **Debug pin 7 (GND)** solidly into the
> star ground — that is the reference StallGuard compares against.

---

## 3. Motor phases — TMC5160 → 23HE30-2804S

| TMC pin | Motor lead | Typical wire colour* | ✓ |
|---------|-----------|----------------------|---|
| A1 (13) | A+ | Black | ☐ |
| A2 (14) | A− | Green | ☐ |
| B1 (11) | B+ | Red   | ☐ |
| B2 (12) | B− | Blue  | ☐ |

*\*Verify the coil pairs against the 23HE30-2804S datasheet or by continuity.
A1/A2 must be **one** coil and B1/B2 the **other**. Crossing pairs = buzz, no rotation;
reversed within a pair = just runs the wrong way (fix in firmware R/L).*

---

## 4. Config / ground pins on the TMC

| TMC pin | Tie to | Meaning | ✓ |
|---------|--------|---------|---|
| EN (1)  | GND | Driver **enabled** (active low) | ☐ |
| CLK (6) | GND | Use **internal** oscillator | ☐ |
| GND (9) | Star ground | Power ground | ☐ |
| GND (15)| Star ground | Power ground | ☐ |

---

## 5. Star ground (single joining point at the PSU)

Join all of these to **one** node at the PSU 24V-ground. Run the **motor return
(TMC 9/15)** as its own heavier wire straight back — don't daisy-chain 2.2A through
the breadboard rail your DIAG0 line references.

> **Tie BOTH sides of the TMC to the star — pins 9 AND 15.** They are separate
> ground pins on opposite corners of the carrier and are NOT bonded together on
> the board. If one side floats, the EN→GND tie loses its reference and DRV_ENN
> drifts high (internal pull-up) → the output stage stays **disabled**: SPI still
> answers and `cs_actual` still reads full current, but the motor gets no current,
> no torque, and no heat. (This exact failure cost a full debug session once.)

- ☐ PSU GND (star origin)
- ☐ TMC GND **9 AND 15** — both corners, heavy wire, direct
- ☐ TMC EN 1, CLK 6
- ☐ WT32 Extended IO pin 2 (GND)
- ☐ WT32 Debug pin 7 (GND)  ← DIAG0 reference
- ☐ AMS1117 GND

---

## 6. Audio (onboard — no external signal wiring)

| Item | Detail | ✓ |
|------|--------|---|
| Speaker | Plug into **Speaker Connector**: pin 1 SPK+, pin 2 SPK− | ☐ |
| I2S amp | Internal; driven by GPIO 35 (LRCK) / 36 (BCLK) / 37 (DOUT) — free on N8R2 | ☐ |

---

## Do-not-use

- ☐ Debug **pin 2 (+3.3V)** — "for reference, not for power input." Left unused; VIO comes from the AMS1117.
- ☐ TMC **DIAG1** — not tied to any rail.

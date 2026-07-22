# PSU Enclosure — Auto Dialer

Vented enclosure for the Auto Dialer's mains power supply. It holds an Arcity-style
**160 × 98.5 × 39 mm** arcade switching PSU and a **screw-mount fused/switched IEC power inlet**,
with a removable honeycomb-vented lid. The low-voltage harness exits upward to the
[touchscreen case](../) through a lid-height notch.

Generated parametrically with [build123d](https://github.com/gumyr/build123d) — see
`PSU-Case-generator.py` (deps: `build123d`, `numpy`; writes STL/STEP to the `OUT` path).

## Print these
- `PSU-Case-Body.stl` / `.step`
- `PSU-Case-Lid.stl` / `.step`

## Design
- External **~241 × 131.5 × 61.5 mm**; internal 236 × 126.5 × 59; walls/floor 2.5 mm.
- **PSU mount:** sits on four 4 mm corner standoffs (airflow underneath), screwed down at its two
  diagonal holes (M3 self-tap into the standoffs). Post spacing **151 mm** long-way × **86.5 mm**
  short-way. Two plain support pads + low corner guides locate it.
- **IEC inlet:** screw-mount, on the −X end wall. Cutout **47.7 × 28.2 mm**, two M3 holes **40 mm**
  apart (vertical), centre 26 mm up.
- **Terminal access:** the PSU terminal block faces the **+X end**, which has a **38 mm (1.5")**
  access bay for spade connectors and wire routing — reachable without removing the inlet.
- **Lid:** recessed into a flanged rabbet, four full-height M3 corner posts (self-tapping). The
  cable notch (front wall, +X bay) is open to the top, so the harness drops in/out with the lid off.
- **Venting:** honeycomb on both long walls, the +X end, the floor, and the lid.

## Hardware
- PSU: Arcity 5V/12V/24V arcade PSU (160 × 98.5 × 39)
- Inlet: 15 A fused + switched IEC C14 inlet module (screw-mount, ~47 × 28 cutout)
- Screws: 4× M3 (lid), 2× M3 (PSU hold-down), 2× M3 (inlet)
- Spade/fork terminals for the PSU barrier strip — **stud size TBD, see below**

## Print notes
- **Body:** open-top up. No supports needed except the **top of the inlet cutout** (one small
  bridge). The lid ledge is undercut with a ~58° self-supporting ramp and the corner posts are
  full-height, so neither needs support.
- **Lid:** flat, either side down.

## To confirm before final assembly
- The PSU barrier-strip **terminal stud size is not published.** Measure the ring-terminal inner
  diameter on the inlet's pre-wired leads (they fit the PSU): ~3.7 → #6/M3.5, ~4.3 → #8/M4,
  ~5.3 → #10/M5. Order fork/spade terminals in that stud size, rated for your gauge
  (14 AWG = blue, 22 AWG = red). Note the inlet's own leads are 18 AWG.
- Test-fit the spade bodies + wire bend in the +X bay after printing.
- Re-slice the STLs (any previously bundled `.3mf` is stale).

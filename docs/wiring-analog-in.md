
# Analog Input Wiring (Line/Mic) — NeoTrellis M4

Use Adafruit’s **Audio Input Circuit**: AC-couple with a capacitor, add a resistor divider to bias around mid-supply, and feed the analog pin (see `Config.h`). This centers the waveform for the ADC and protects against DC offsets. The TRRS jack’s mic path can also be routed to an ADC channel if you prefer headset input.

**Recommended defaults**
- **ADC pin:** `A5` (edit in `Config.h`).
- **Sample rate:** 22,050 Hz (shared with playback).
- **Input level:** Keep around line level; avoid clipping. The bias network sets mid-rail; the Trellis visualizer guide shows exact values.

**Testing**
- Record short takes (≤ 2.5 s). On stop, the firmware saves `source.raw` then writes `A1.raw..A8.raw` as equal segments.
- If noise is high, add a simple RC low-pass (< 10 kHz) in front of the bias network.

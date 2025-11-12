
# Analog Input Wiring (Line/Mic) — NeoTrellis M4

Use Adafruit’s **Audio Input Circuit**: AC-couple with a capacitor, add a resistor divider to bias around mid-supply, and feed the analog pin (see `Config.h`). This centers the waveform for the ADC and protects against DC offsets. The TRRS jack’s mic path can also be routed to an ADC channel if you prefer headset input.

## Firmware ADC setup

- **Resolution:** `analogReadResolution(12)` is called during recorder setup so the raw samples land in the 0–4095 range our 12-bit→16-bit scaling math expects.
- **Reference:** We explicitly request `analogReference(AR_DEFAULT)` (3.3 V on the NeoTrellis M4). The bias network should therefore sit around 1.65 V so the captured waveform swings ±2048 counts before we upshift to 16-bit.
- **Averaging:** A small `analogReadAveraging(4)` smooths quantization noise without turning transients to mush. Feel free to back it off if you want the rawest grit.

When you scope the recorded data (e.g. dump a `source.raw` capture into Audacity), it should now hover around zero with plenty of headroom before clipping. If you see asymmetry, revisit the bias resistors or confirm the reference really is 3.3 V.

**Recommended defaults**
- **ADC pin:** `A5` (edit in `Config.h`).
- **Sample rate:** 22,050 Hz (shared with playback).
- **Input level:** Keep around line level; avoid clipping. The bias network sets mid-rail; the Trellis visualizer guide shows exact values.

**Testing**
- Record short takes (≤ 2.6 s by default). On stop, the firmware saves `source.raw` then writes `A1.raw..A8.raw` as equal segments.
- If noise is high, add a simple RC low-pass (< 10 kHz) in front of the bias network.

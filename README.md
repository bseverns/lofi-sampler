
# NeoTrellis M4 — Lo‑Fi Sampler (Arduino) — Live Resampling + USB MIDI Clock

**Core idea:** 4 rows = 4 voices. Each row holds 1 sample, auto‑sliced into **8 equal regions**. A global **USB MIDI clock** quantizes playback; each step all rows advance in lockstep. You get that sliding **silence→phase→chaos** when source lengths differ.

This build targets **Arduino (TinyUSB)** + **analog line‑in** (as in Adafruit’s Audio Input Circuit) on the **NeoTrellis M4**. It also supports recording via analog input into RAM, writing to QSPI **LittleFS**, and auto‑slicing to 8 RAW files per row.

---

## Features
- **Quantized gates:** 8 steps per bar, one step per NeoTrellis column.
- **4 voices (rows A–D):** one sample per row, sliced into A1..A8, etc.
- **USB MIDI Clock** (24 PPQN) + Start/Stop/Continue → transport.
- **Multi-button controls:**
  - **Shift (col 8) + Row pad** → **Record/Stop** row (analog line-in).
  - **Alt (col 7) + Row pad** → **Erase** row’s slices.
  - **Shift + Alt + Row pad** → **Reslice** row from `source.raw` (equal 8ths).
  - **Normal taps** → toggle gate at that column for that row.
- **Audio out:** DAC A0 mirrored to A1; timer‑driven at 22,050 Hz, 16‑bit signed.
- **Storage:** QSPI flash via **LittleFS** (raw 16‑bit mono), fast prefetch on step.
- **Live resampling:** 2.5 s default (≈110 KB). On stop, auto‑slice → 8 raw files.

> This repo purposely stores **RAW** 16‑bit little‑endian PCM (`.raw`) to avoid WAV parsing on-device. Use the `tools/wav_to_raw_slices.py` helper or record directly on the Trellis.

---

## Hardware & Libraries

- **Board:** Adafruit NeoTrellis M4 Express (SAMD51).
- **Analog input (line/mic):** Follow Adafruit’s **Audio Input Circuit** to AC‑couple and bias the signal, then feed the configured **ADC pin** (see `Config.h`). TRRS mic input can also be used; set the matching ADC pin.
- **Arduino Libraries (install via Library Manager):**
  - `Adafruit NeoTrellis M4`
  - `Adafruit TinyUSB Library` (for USB MIDI device)
  - `Adafruit SPIFlash`
  - `Adafruit LittleFS`
  - `Adafruit ZeroTimer` (timer ISR at 22.05 kHz)
  - `MIDI Library` (FortySevenEffects), optional (we use TinyUSB directly by default)

---

## Folder layout
```
firmware/arduino/lofi_sampler/
  lofi_sampler.ino
  AudioEngine.h / .cpp       # DAC timer ISR, 4‑voice mix, slice preload
  RecorderADC.h / .cpp       # analog line‑in capture to RAM
  Storage.h / .cpp           # LittleFS (QSPI) mount, read/write raw slices
  Slicer.h / .cpp            # equal‑eighth slicing (RAM → files)
  Config.h                   # pins, sample rates, timings, colors
  TrellisUI.h / .cpp         # key scanning, LED states, combos
tools/
  wav_to_raw_slices.py       # convert WAV→8 RAW files for a row
docs/
  wiring-analog-in.md        # analog input circuit + pin notes
  workflow.md                # clock math, file scheme, testing checklist
```
---

## Build quickstart
1. **Boards Manager:** Install *Adafruit SAMD* core. Select **Adafruit NeoTrellis M4**.
2. **Libraries:** Install the list above.
3. **Flash FS:** First upload the sketch; it will format LittleFS on first boot (QSPI).
4. **Load samples:** Either
   - Record a row: hold **Shift (col 8)** + tap a row pad. Tap again to stop.
   - Or pre‑slice: run `tools/wav_to_raw_slices.py` on a WAV and copy `A1.raw..A8.raw` to `/A/` (same for B/C/D).
5. **Clock:** Start your DAW so it sends USB MIDI **Clock** + Start. Toggle gates and listen.

---

## Notes
- **RAW format:** 16‑bit signed little‑endian, mono, 22,050 Hz.
- **Max record secs:** Adjust in `Config.h` (RAM‑bound).
- **Playback:** On each step, active rows preload that step’s raw slice from QSPI into a small RAM buffer; ISR mixes 4 voices and writes DAC.
- **CPU budget:** The ISR only mixes 4 int16 samples → saturation → DAC write. All file I/O happens in the main loop between steps.
- **AudioEngine etiquette:** `service()` runs in the foreground, drains a job queue, and tops off circular buffers in flash-sized chunks. The 22.05 kHz ISR only ever reads already-primed samples + gain ramps. If you add new work, make it a job and let the loop babysit it; the interrupt stays allergic to anything slower than a multiply.

See `docs/workflow.md` for timing math and performance tips.

### AudioEngine job queue cheat sheet

Think of the engine as a stubborn bandmate who only plays what’s been laid out the night before:

- **Jobs are the todo list.** Preload requests, fades, and diagnostic dumps all go through the tiny queue so the loop can serialize slow work without blocking the ISR.
- **`pumpStreams()` feeds the beast.** It reads flash in 64–256 sample chunks (depending on buffer size), wrapping in-place so voices always have something queued.
- **`pumpGains()` is just housekeeping.** Gain ramps are precomputed steps; no envelopes inside the interrupt.
- **`isr()` is boring by design.** It mixes signed 16-bit samples already waiting in RAM, clamps them, and hits the DAC. No filesystem, no Serial prints, no drama.

When in doubt, keep heavy lifting in `service()` and treat the ISR like a sacred cave where only deterministic math is allowed.

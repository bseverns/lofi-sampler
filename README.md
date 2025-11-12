
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
- **Live resampling:** 2.6 s default (≈115 KB capture). On stop, auto‑slice → 8 raw files.

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

### RAM budget vs. record slider (SAMD51)
The NeoTrellis M4 gives us **192 KiB** of SRAM. Recording burns RAM three ways: one capture buffer and four voice buffers (one slice per voice). Rule of thumb:

```
audio_RAM_bytes ≈ SAMPLE_RATE_HZ * seconds * 3
```

| Max record seconds | Capture buffer | Voice buffers | Audio SRAM | Headroom vs. 192 KiB |
| --- | --- | --- | --- | --- |
| 2.0 s | ~86 KiB | ~43 KiB | ~129 KiB | ~63 KiB free |
| 2.6 s *(default)* | ~112 KiB | ~56 KiB | ~168 KiB | ~24 KiB for Trellis/USB/stack |
| 2.7 s *(upper comfy limit)* | ~116 KiB | ~58 KiB | ~174 KiB | ~18 KiB left — risky above this |

That 24 KiB margin at 2.6 s keeps the Trellis driver, USB MIDI buffers, and the stack happy. Each extra **0.1 s** costs ~6.6 KiB, so if you crank `MAX_RECORD_SECONDS` past ~2.7 s you’ll start starving the rest of the firmware.

LittleFS still keeps up: a step only has to slurp one slice (`BUF_SAMPLES` ≈ 7k samples → ~14 KiB) per active voice, which the QSPI flash handles comfortably before the next MIDI tick. On stop, writing eight slices + `source.raw` is ~4× the captured sample count; even the conservative ~400 KiB/s page-program rate finishes a 2.6 s take in <0.6 s, so USB MIDI can backlog clocks without overflowing.

See `docs/workflow.md` for timing math and performance tips.

---

## Control Atlas (pad combos vs. firmware branches)

If you’re spelunking the UI logic, every pad mash ends up in the `loop()` state machine inside `firmware/arduino/lofi_sampler/lofi_sampler.ino`. Here’s the cheat-sheet so you can keep one eye on the Trellis and one eye on the code:

| Pad combo | `loop()` branch | Expected side effects |
| --- | --- | --- |
| **Tap any step (cols 0–5) with no modifiers** | `else { gates[r][c] = !gates[r][c]; ui.setGate(...); }` | Toggles the gate latch for that row/column and repaints the LED immediately. |
| **Hold Alt column (col 7)** | `if (c == COL_ALT) { gates[r][COL_ALT] = true; }` | Latches the per-row Alt modifier flag so the very next pad press runs the erase logic. Releases clear the flag. |
| **Hold Shift column (col 8)** | `else if (c == COL_SHIFT) { gates[r][COL_SHIFT] = true; }` | Latches the per-row Shift modifier flag so the next pad press arms record/reslice behaviors. Releases clear the flag. |
| **Shift + Row pad** | `else if (shift) { ... rec.start()/rec.stop(); Slicer::writeEight(...); }` | Starts live recording on first hit; on the second hit stops capture, writes `/[Row]/source.raw`, then slices + commits eight RAW files. |
| **Alt + Row pad** | `else if (alt) { ... storage.remove(...); }` | Nukes every slice file (`R1.raw…R8.raw`) and the row’s `source.raw`. Think of it as “panic/blank this row.” |
| **Shift + Alt + Row pad** | `if (shift && alt) { /* TODO: reslice in-place */ }` | Currently a deliberate no-op (placeholder for an in-place re-slice). Enjoy the blinking lights, but don’t expect audio changes yet. |
| **Release Alt/Shift** | `if (c == COL_ALT) gates[r][COL_ALT] = false;` / `if (c == COL_SHIFT) gates[r][COL_SHIFT] = false;` | Resets the modifier flags so normal tapping resumes. |

Need to see how those branches sync with USB clocking, storage writes, and the DAC ISR? Jump to the [Timing Swim-Lane](docs/workflow.md#timing-swim-lane-midi-vs-ui-vs-storage-vs-dac) notes.

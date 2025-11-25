
# NeoTrellis M4 — Lo‑Fi Sampler (PlatformIO) — Live Resampling + USB MIDI Clock

**Core idea:** 4 rows = 4 voices. Each row holds 1 sample, auto‑sliced into **8 equal regions**. A global **USB MIDI clock** quantizes playback; each step all rows advance in lockstep. You get that sliding **silence→phase→chaos** when source lengths differ.

This build now targets **PlatformIO + Adafruit’s TinyUSB Arduino core** on the **NeoTrellis M4**. Same hardware, same neon chaos, but the toolchain is scripted so you can `pio run -t upload` instead of sweet‑talking the Arduino IDE. It still supports recording via analog input into RAM, writing to QSPI **LittleFS**, and auto‑slicing to 8 RAW files per row.

## Getting Started (hardware-first, in order)
1. **Plug things in:** USB-C from your computer → NeoTrellis M4. Patch the **line input** (phone, synth, tape deck) through the Audio Input Circuit into **A5** (or your configured ADC pin) and plug headphones/speakers into the Trellis’ DAC jack. The wiring cheatsheet lives in [`docs/wiring-analog-in.md`](docs/wiring-analog-in.md).
2. **Install PlatformIO:** `pip install platformio` or grab the VS Code extension. This repo already carries a [`platformio.ini`](firmware/platformio/platformio.ini) that pins every required library.
3. **Build + upload:**
   ```bash
   cd firmware/platformio
   pio run                   # compile + fetch Adafruit libs
   pio run -t upload        # flash firmware over USB
   pio device monitor       # peek at serial output (115200 baud)
   ```
   First boot formats the QSPI flash as LittleFS; keep it powered during the progress blip.
4. **Load samples:** Option A — hold **Shift (col 8)** and tap any row pad to record the analog input, then tap again to stop + auto-slice. Option B — pre-slice a WAV (see the exact command below) and copy the `A1.raw…A8.raw` files plus `source.raw` to `/A`, `/B`, etc. on the mounted LittleFS drive (e.g. drag the files onto the NeoTrellis volume).
5. **Clock + jam:** Start the provided `examples/midi_clock_sender.py` or your DAW so it emits MIDI Start + Clock. Toggle gates, hold **Alt** (col 7) to erase, hold **Shift** for live record/stutter. Connect headphones and run `python examples/gen_demo_row_A.py` once if you want a baked-in sample pack to copy to `/A/`.

---

## Features
- **Quantized gates:** 8 steps per bar, one step per NeoTrellis column.
- **4 voices (rows A–D):** one sample per row, sliced into A1..A8, etc.
- **USB MIDI Clock** (24 PPQN) + Start/Stop/Continue → transport.
- **Multi-button controls:**
  - **Shift (col 8) + Row pad** → **Record/Stop** row (analog line-in).
  - **Shift + active gate pad** → **Stutter** that slice momentarily at a boosted velocity (no gate toggle).
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
- **PlatformIO auto-installs the libraries:** see [`firmware/platformio/platformio.ini`](firmware/platformio/platformio.ini). It pulls `Adafruit NeoTrellis M4`, `Adafruit TinyUSB`, `Adafruit SPIFlash`, `Adafruit LittleFS`, and `Adafruit ZeroTimer` so you don’t have to babysit the Library Manager.

---

## Folder layout
```
firmware/platformio/
  platformio.ini             # board + library roster (PlatformIO)
  src/
    main.cpp                 # former lofi_sampler.ino entry point
    AudioEngine.h / .cpp     # DAC timer ISR, 4‑voice mix, slice preload
    RecorderADC.h / .cpp     # analog line‑in capture to RAM
    Storage.h / .cpp         # LittleFS (QSPI) mount, read/write raw slices
    Slicer.h / .cpp          # equal‑eighth slicing (RAM → files)
    Config.h                 # pins, sample rates, timings, colors
    TrellisUI.h / .cpp       # key scanning, LED states, combos
tools/
  wav_to_raw_slices.py       # convert WAV→8 RAW files for a row
docs/
  wiring-analog-in.md        # analog input circuit + pin notes
  workflow.md                # clock math, file scheme, testing checklist
```
---

## Build quickstart
1. **PlatformIO toolchain:** `pip install platformio` (or use the VS Code extension). The repo’s `platformio.ini` already names every dependency.
2. **Compile + upload:**
   ```bash
   cd firmware/platformio
   pio run
   pio run -t upload
   ```
3. **Flash FS on first boot:** The firmware formats LittleFS on the first startup—let it finish before yanking USB.
4. **Load samples:** Either
   - Record a row: hold **Shift (col 8)** + tap a row pad. Tap again to stop.
   - Or pre-slice: run `tools/wav_to_raw_slices.py` and copy the slices straight to the Trellis drive. Example:
     ```bash
     python tools/wav_to_raw_slices.py ~/loops/vocal.wav --outdir /tmp/rowA --prefix A
     cp /tmp/rowA/A*.raw /tmp/rowA/source.raw /media/NEOTRELLIS/A/
     ```
     Drop the matching files into `/B`, `/C`, `/D` as needed (`/A/A1.raw…A8.raw`, `/B/B1.raw…`).
   - Need a built-in loop but can’t ship WAVs? Run `python examples/gen_demo_row_A.py` to synthesize a factory row, then copy the emitted `.raw` files under `/A/` on the Trellis.
5. **Clock:** Start your DAW *or* run `python examples/midi_clock_sender.py --out "NTM4 Sampler"` so the board sees MIDI **Clock** + Start. Toggle gates and listen.

---

## Examples & DIY helpers
- **`examples/gen_demo_row_A.py`:** Synthesizes a 2.56 s drone/chord mash, writes `source.raw` plus `A1.raw…A8.raw`, and never stores WAVs in the repo. Run it once and copy the emitted folder onto the Trellis to sanity-check hardware.
- **`examples/midi_clock_sender.py`:** Python + `mido` script that spits MIDI Start + Clock so you can rehearse quantized playback without launching a DAW. Pass `--bpm` to change tempo.
- **Need your own slices?** Run `tools/wav_to_raw_slices.py` (see the command above) and drag the files into the root-level `/A`, `/B`, `/C`, `/D` directories that LittleFS exposes.

### First jam checklist
1. Run `python examples/gen_demo_row_A.py` (once) and copy the resulting files onto the mounted Trellis drive under `/A/`.
2. Plug headphones into the DAC jack (or patch the line out into your mixer).
3. Run `python examples/midi_clock_sender.py --out "NTM4 Sampler" --bpm 90` to clock it.
4. Tap gates on row A (columns 1–6) to lay down the 8-step groove.
5. Hold **Shift** on column 8 + tap the row A pad to overdub your own recording whenever inspiration hits.

---

## Notes
- **RAW format:** 16‑bit signed little‑endian, mono, 22,050 Hz.
- **Max record secs:** Adjust in `Config.h` (RAM‑bound).
- **Playback:** On each step, active rows preload that step’s raw slice from QSPI into a small RAM buffer; ISR mixes 4 voices and writes DAC.
- **CPU budget:** The ISR only mixes 4 int16 samples → saturation → DAC write. All file I/O happens in the main loop between steps.
- **AudioEngine etiquette:** `service()` runs in the foreground, drains a job queue, and tops off circular buffers in flash-sized chunks. The 22.05 kHz ISR only ever reads already-primed samples + gain ramps. If you add new work, make it a job and let the loop babysit it; the interrupt stays allergic to anything slower than a multiply.

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

**TL;DR:** run the stock **2.6 s** capture window unless you know the rest of your code’s RAM hunger; it leaves ~24 KiB for UI + USB. You can sneak up to ~2.7 s, but crashes lurk beyond that.

That 24 KiB margin at 2.6 s keeps the Trellis driver, USB MIDI buffers, and the stack happy. Each extra **0.1 s** costs ~6.6 KiB, so if you crank `MAX_RECORD_SECONDS` past ~2.7 s you’ll start starving the rest of the firmware.

LittleFS still keeps up: a step only has to slurp one slice (`BUF_SAMPLES` ≈ 7k samples → ~14 KiB) per active voice, which the QSPI flash handles comfortably before the next MIDI tick. On stop, writing eight slices + `source.raw` is ~4× the captured sample count; even the conservative ~400 KiB/s page-program rate finishes a 2.6 s take in <0.6 s, so USB MIDI can backlog clocks without overflowing.

See `docs/workflow.md` for timing math and performance tips.

### AudioEngine job queue cheat sheet

Think of the engine as a stubborn bandmate who only plays what’s been laid out the night before:

- **Jobs are the todo list.** Preload requests, fades, and diagnostic dumps all go through the tiny queue so the loop can serialize slow work without blocking the ISR.
- **`pumpStreams()` feeds the beast.** It reads flash in 64–256 sample chunks (depending on buffer size), wrapping in-place so voices always have something queued.
- **`pumpGains()` is just housekeeping.** Gain ramps are precomputed steps; no envelopes inside the interrupt.
- **`isr()` is boring by design.** It mixes signed 16-bit samples already waiting in RAM, clamps them, and hits the DAC. No filesystem, no Serial prints, no drama.

When in doubt, keep heavy lifting in `service()` and treat the ISR like a sacred cave where only deterministic math is allowed.
---

## Control Atlas (pad combos vs. firmware branches)

If you’re spelunking the UI logic, every pad mash ends up in the `loop()` state machine inside `firmware/platformio/src/main.cpp`. Here’s the cheat-sheet so you can keep one eye on the Trellis and one eye on the code:

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

Ready to riff on new modes or effects? Peep [CONTRIBUTING.md](CONTRIBUTING.md) for the house style + testing checklist.

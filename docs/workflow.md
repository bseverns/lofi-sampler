
# Workflow, timing, and files

Need a refresher on which pad combo does what? Bounce back to the [Control Atlas](../README.md#control-atlas-pad-combos-vs-firmware-branches) for the UI-side cheatsheet.

**Clock math**
- USB MIDI Clock = 24 PPQN
- 4/4, 8 steps/bar → **12 clocks/step**
- Step duration at tempo T BPM: `(60/T) * (beats_per_bar / steps_per_bar)` seconds

**Files**
- Per row: `/<Row>/source.raw` (optional), and `/<Row>/<Row>1.raw … <Row>8.raw`
- RAW format: signed 16-bit little‑endian, mono, 22,050 Hz

**Playback**
- On step boundary:
  - If gate ON for row R at column C, preload `R{C+1}.raw` into row buffer.
  - ISR mixes 4 voices: `sum = clamp(sum of int16)`; write to DAC (12‑bit).

**Recording**
- While recording, the player continues; the row being recorded is muted.
- On stop: slice buffer into 8 equal parts → write as raw files.

## Pad combo cheat-sheet (per row)

- **Shift (hold col 8) + row pad** → arm/cut tape style **record**.
- **Shift + lit step** → fire a **stutter blast** of that slice (no gate change, auto velocity curve).
- **Alt (hold col 7) + row pad** → **erase** that row’s slices + `source.raw`.
- **Shift + Alt + row pad** → **reslice** from the saved `source.raw` (reloads from flash first).
---

## Timing Swim-Lane (MIDI vs. UI vs. Storage vs. DAC)

Low-tech diagram, high-truth content. Each lane is a core subsystem; rows scroll left→right in real time. Use it to reason about back-pressure and why the ISR is sacred.

```
Time ───────────────────────────────────────────────────────────────────────────▶
MIDI Clock : |Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick| ...
             |<--12 clocks-->|
UI loop    : [scan pads]─┐                         ┌─[scan pads]─┐
             │             │                         │            │
             └─(modifier flags set)───┐   ┌─(gate flip / record cmd)┘
Storage I/O:                (idle)    │   │   (erase, slice writes)
                                       ▼   ▼
             ────────────┬─────────────┬────────────────────────────
                          preload step │
DAC ISR    : [mix voices @22.05 kHz | mix | mix | mix | mix | mix | …]
             └── runs every 45.35 µs no matter what the main loop is doing ────
```

Key moments:
- **Clock boundary:** every 12 MIDI clocks we bump `stepIndex`, preload slices, and the DAC keeps hammering samples without missing a beat.
- **UI bursts:** modifier pads set flags instantly; the expensive work (record stop → slice writes) happens just after the UI event, while the ISR keeps breathing.
- **Storage spikes:** erase/slice writes block the main loop for a beat, but they’re intentionally outside the ISR so audio playback stays stable.

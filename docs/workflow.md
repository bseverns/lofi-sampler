
# Workflow, timing, and files

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

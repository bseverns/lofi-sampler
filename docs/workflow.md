# Workflow, Timing, And Files

For the canonical pad behaviors, see [`controls.md`](controls.md). For the narrated teaching flow, see [`demo-exercises.md`](demo-exercises.md).

## Clock Math
- USB MIDI clock = 24 PPQN
- 4/4, 8 steps per bar = 12 clocks per step
- Step duration at tempo `T` BPM:
  - `(60 / T) * (beats_per_bar / steps_per_bar)` seconds

## Files
- Per row:
  - `/<Row>/<Row>1.raw` through `/<Row>/<Row>8.raw`
  - `/<Row>/source.raw` when the row has recorded or staged source material
  - `/<Row>/source_prev.raw` when a previous take has been preserved
- Audio format:
  - signed 16-bit little-endian PCM
  - mono
  - 22050 Hz

## Playback Model
- On each step boundary, active rows preload that row's current slice.
- The foreground `service()` path handles storage work and gain/fx jobs.
- The ISR only mixes ready samples and writes the DAC.

## Recording Model
- Recording captures a new source for one row.
- On stop, the source becomes `source.raw` and the row is re-sliced into eight files.
- Restore swaps back `source_prev.raw` when it exists.
- Reslice rebuilds the eight row slices from the current row source.

## Timing Swim-Lane

```text
Time --------------------------------------------------------------->
MIDI Clock : |Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick|Tick|...
             |<----------- 12 clocks per step ----------->|

UI loop    : [scan pads] [route combo] [toggle gate] [queue jobs] ...
Storage    :            [read slice]          [write source/slices]
Audio loop :            [service jobs] [pump streams] [pump gains]
DAC ISR    : [mix] [mix] [mix] [mix] [mix] [mix] [mix] [mix] ...
```

Key point: storage and control work can be lumpy, but the ISR must stay boring and deterministic.

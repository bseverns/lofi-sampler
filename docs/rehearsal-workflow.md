# Rehearsal Candidate Workflow

## Prepare a set

1. Copy `examples/performance-set.template.json` outside the repository and replace its four source paths, or provide a manifest with 32 `pads` entries (`A1` through `D8`). Sources must already be mono, 16-bit PCM, 22.05 kHz WAV.
2. From the repository root:

   ```sh
   python3 tools/build_performance_set.py /path/to/my-set.json
   cd firmware/platformio
   pio run -e adafruit_trellis_m4_performance
   python3 scripts/bin_to_uf2.py \
     .pio/build/adafruit_trellis_m4_performance/firmware.bin \
     0x4000 artifacts/my-set.uf2
   ```

3. Copy the UF2 to the NeoTrellis M4 boot volume. This workflow does not flash a connected board automatically.

## Performance controls

| Gesture | Result |
| --- | --- |
| Tap any pad | Toggle its gate. Columns 7/8 still work as step taps. |
| Hold the row's offset Shift, tap any step | Direct retrigger; does not edit the pattern. |
| Hold offset Alt, tap a lit step | Cycle probability. |
| Hold both offsets, steps 1–4 | Filter, bitcrush, drive, clear FX. |
| Hold both offsets, step 8 | Momentary panic: stops audio, not transport. |
| MIDI Stop | Latched emergency silence and transport stop. Start/Continue is required to resume. |

A modifier is the same physical column on the next row down (D wraps to A). When a modifier is held first, columns 7/8 on the target row remain chord targets; when tapped alone, they remain normal steps.

## Before a rehearsal

Run every row of the hardware matrix in `docs/performance-profile.md`. A successful build does not prove USB behavior, flash behavior, timing margin, or an hour-long rehearsal. Keep the previous known-good UF2 available until the new set completes that matrix.

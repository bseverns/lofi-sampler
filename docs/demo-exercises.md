# Demo Exercises - Teaching The NeoTrellis M4 Lo-Fi Sampler

These exercises are short, repeatable demos that explain both what the sampler does and why the design is shaped this way. If you want the narrated version, use [`demo-script.md`](demo-script.md).

## Preflight
Make sure:
- the current firmware is flashed
- MIDI clock is running
- the bundled demo pack is audible or you have recorded material on at least one row

Helpful tools:
- `python examples/midi_clock_sender.py --out "NTM4 Sampler" --bpm 90`
- `python examples/gen_demo_row_A.py`

## Exercise 1 - Clocked Grid
Goal: show that playback advances only on USB MIDI clock.

Steps:
1. Start MIDI clock at 90 BPM.
2. Toggle a few gates on row A.
3. Stop and Continue the transport.

Observe: playback advances only on clock and resumes cleanly.

Why: the clock is the transport spine for the whole instrument.

## Exercise 2 - Phase Drift
Goal: show how equal slicing plus different source material creates motion.

Steps:
1. Enable the same gate pattern on rows A and B.
2. Let it run for several bars.

Observe: the rows drift in and out of phase.

Why: complexity comes from the audio material, not from adding sequencer pages.

## Exercise 3 - Chorded Controls
Goal: show that steps 7 and 8 are still playable while also serving as modifiers.

Steps:
1. Tap step 7 or 8 on a row by itself.
2. Hold the corresponding control pad one row down and tap a pad on the target row.
3. Release and tap the control pad by itself again.

Observe: the same physical pad can be a musical step or a held modifier depending on context.

Why: this preserves the full 8-step lane.

## Exercise 4 - Velocity Lanes
Goal: show per-step dynamics.

Steps:
1. Light a few steps on a row.
2. Hold Shift for that row and tap a lit step repeatedly.

Observe: the velocity lane changes and the hit is more or less accented.

Why: discrete expressive states are fast to teach and fast to play.

## Exercise 5 - Probability Lanes
Goal: show controlled randomness.

Steps:
1. Light a few steps on a row.
2. Hold Alt for that row and tap a lit step repeatedly.

Observe: the probability lane cycles.

Why: variation without UI sprawl.

## Exercise 6 - Stutter Without Reprogramming
Goal: show a live gesture that does not rewrite the pattern.

Steps:
1. Light a step on a row.
2. Hold Shift for that row and tap the lit step while it plays.

Observe: you get a stutter hit while the underlying gate pattern stays intact.

Why: expressive overlay beats menu diving.

## Exercise 7 - Record And Auto-Slice
Goal: show the live capture path.

Steps:
1. Patch a line-level source into the analog input.
2. Hold Shift for a row and tap an unlit pad on that row to start recording.
3. Repeat to stop.

Observe: the row writes a new source and eight fresh slices.

Why: every take is normalized back into the same 8-step playback model.

## Exercise 8 - Restore And Reslice
Goal: show the safety net.

Steps:
1. Record a new take on a row.
2. Hold Alt for that row and tap an unlit pad on the row.
3. Hold Shift and Alt and tap step 6 on the row.

Observe: Alt restores the previous take when present or blanks the row when not. Shift+Alt+step 6 reslices the current row source without touching gates.

Why: you can experiment without rebuilding the pattern from scratch.

## Exercise 9 - FX And ISR Discipline
Goal: show the performance FX and the engineering rule behind them.

Steps:
1. Hold Shift+Alt and tap step 1.
2. Repeat with steps 2, 3, and 4.

Observe: the timbre changes immediately while playback remains stable.

Why: the foreground builds the work and the ISR stays deterministic.

## Exercise 10 - Known-Good Demo Surface
Goal: show that the repo has a fast trust surface.

Steps:
1. Flash the current firmware.
2. Start clock.
3. Use the bundled demo pack.
4. Compare what you hear with [`ListeningGuide.md`](ListeningGuide.md).

Observe: a freshly flashed board has an audible proof surface without a separate filesystem upload ritual.

Why: public-facing hardware repos need a musical trust surface, not just source code.

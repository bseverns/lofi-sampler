# Demo Script - Lo-Fi Sampler Walkthrough

This script follows the same order as [`demo-exercises.md`](demo-exercises.md). It is written for an 8-10 minute walkthrough.

## Intro
"This is the NeoTrellis M4 Lo-Fi Sampler: four rows, one sample per row, eight clocked steps, and a very small control surface that tries hard not to get in the way of playing."

## Clocked Grid
"First, the grid only moves on MIDI clock. Start and Continue are transport events, not decoration. When the host stops, the pattern freezes exactly where it is."

## Phase Drift
"Rows can share the same gate pattern and still feel alive because their source material differs. The drift is musical content drift, not sequencer complexity."

## Chorded Controls
"The old dedicated control pads are now dual-purpose. Tap steps 7 or 8 and they are still steps. Hold them and press another pad, and they become Alt or Shift for the row above. That keeps the full eight-step lane intact."

## Velocity And Probability
"Shift on a lit step changes the accent lane. Alt on a lit step changes the probability lane. These are discrete, repeatable musical states, not hidden menu values."

## Stutter
"Shift on a lit step also throws a stutter hit. You get a performative burst without rewriting the pattern."

## Record And Slice
"Record a new take on a row, stop it, and the row gets a new `source.raw` plus eight slices. The grid stays conceptually simple even though the source material just changed underneath it."

## Restore And Reslice
"Alt on an unlit pad in a row restores the previous take when one exists, or blanks the row when it does not. Shift plus Alt on step 6 reslices the row without touching the gate pattern."

## FX
"Shift plus Alt on steps 1 through 4 triggers filter, bitcrush, drive, and clear. The important engineering detail is that the ISR stays boring while the foreground prepares the heavy work."

## Demo Confidence
"The current firmware ships with a known-good bundled demo pack, so a freshly flashed board has an audible proof surface even before you start recording new material."

## Outro
"The machine is small on purpose: one clock, one grid, one source per row, and just enough control depth to keep it expressive without turning it into a menu system."

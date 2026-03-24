# Controls

This is the canonical control reference for the current firmware.

## Grid Layout
- 4 rows = 4 voices.
- 8 columns = 8 steps per bar.
- Physical columns 7 and 8 are dual-purpose:
  - tap them alone: they behave like normal steps
  - hold them and press another pad: they act as row modifiers

## Modifier Ownership
Modifiers still live one row down from the row they affect.

- Row A modifiers live on **row B, col 7/8**
- Row B modifiers live on **row C, col 7/8**
- Row C modifiers live on **row D, col 7/8**
- Row D modifiers live on **row A, col 7/8**

Think of them as hold-for-control pads, not permanently reserved control buttons.

## Canonical Gestures

### Normal step editing
- Tap any pad with no modifier: toggle that step's gate.
- Tap col 7 or col 8 by itself: still toggles step 7 or step 8 normally.

### Shift-held gestures
- Hold the target row's Shift pad and tap a **lit** step: cycle the velocity lane and fire a stutter hit.
- Hold the target row's Shift pad and tap an **unlit** step on that row: begin or stop recording for that row.

### Alt-held gestures
- Hold the target row's Alt pad and tap a **lit** step: cycle the probability lane.
- Hold the target row's Alt pad and tap an **unlit** step on that row: perform a restore-or-blank action. It restores the previous take if `source_prev.raw` exists; otherwise it blanks the row.

### Shift + Alt gestures
- Hold both modifier pads for a row and tap step 1: filter sweep.
- Hold both modifier pads for a row and tap step 2: bitcrush.
- Hold both modifier pads for a row and tap step 3: drive.
- Hold both modifier pads for a row and tap step 4: clear FX.
- Hold both modifier pads for a row and tap step 6: reslice the row from its current source material.

## Notes On Semantics
- The docs use **target row** instead of **row pad** because the hardware has only the 4x8 grid; there is no separate row button bank.
- Record/restore gestures are row-level behaviors routed from the pad you hit on that row after modifiers are applied.
- A lit-step Alt gesture changes probability instead of the row-level restore-or-blank action.
- A lit-step Shift gesture changes velocity and fires a stutter hit in the same press path.

## Files Touched By Row-Level Actions
- Recording writes `/<Row>/source.raw` and fresh slices `/<Row>/<Row>1.raw` through `/<Row>/<Row>8.raw`.
- Restore uses `/<Row>/source_prev.raw` when it exists.
- Reslice rebuilds the eight slices from the current row source.

For transport timing and subsystem flow, see [`workflow.md`](workflow.md). For the live teaching version of this same surface, see [`demo-exercises.md`](demo-exercises.md).

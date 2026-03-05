# Demo Script — Lo‑Fi Sampler Walkthrough (Narrated)

This script follows the punching order from `docs/demo-exercises.md`. It is
written for an ~8–10 minute walkthrough with short transitions between sections.
For related setup and reference docs, see the [README documentation map](../README.md#documentation-map).

---

**Intro**
“This is the NeoTrellis M4 Lo‑Fi Sampler: four rows, one sample per row, and an
8‑step grid that stays locked to a global MIDI clock. I’ll walk through how the
system works and why the design choices look the way they do. If you want to
reproduce the demo, the steps are in `docs/demo-exercises.md`.”

---

**Clocked Grid**
“First, the grid is clocked. I’m sending USB MIDI clock at 90 BPM, and you’ll
notice the pads advance only on clock ticks. When I stop the clock, playback
freezes, and when I hit Continue it picks up right where it left off. That’s
the design goal: the clock is the spine, so timing stays deterministic even
when the UI or storage is busy.”

Transition: “With the clock stable, we can lean into how slicing creates motion.”

---

**Phase Drift**
“Rows A and B are playing the same pattern, but the source lengths are
different. Even though each row is sliced into eight equal steps, the material
itself doesn’t line up for long, so you hear it drift in and out of phase over
several bars. That’s intentional: you get evolving rhythm from a fixed grid
without adding complexity to the sequencer.”

Transition: “Now let’s look at why the modifier pads are offset.”

---

**Offset Latches**
“Alt and Shift are latched one row down. I hold Alt on the row below, and now I
can tap any of the eight steps without giving up a step column. That’s the key:
each track keeps a full 8‑step lane while still getting modifier access. It’s a
small layout change that preserves the music‑making surface.”

Transition: “Next are the two per‑step expression lanes.”

---

**Velocity Lanes**
“Each lit step cycles through three velocity lanes. You’ll see the LED change
brightness as I tap with Shift. This gives dynamics without a menu or hidden
page, and because the lanes are discrete, it stays quick and reliable in a
live context.”

Transition: “And the same idea applies to probability.”

---

**Probability Lanes**
“Holding Alt and tapping a lit step cycles probability lanes. It’s controlled
randomness: same pattern, different outcomes. The point is to add variation
without asking the player to program a bunch of additional UI states.”

Transition: “Now we’ll hit a performance gesture that doesn’t alter the grid.”

---

**Stutter Gesture**
“Shift‑tapping an active step fires a stutter blast of that slice, but the gate
pattern stays untouched. You can perform on top of the pattern without
reprogramming it. This is the live‑play sweet spot: expression without side
effects.”

Transition: “Let’s capture new material and watch how it gets folded into the grid.”

---

**Record + Auto‑Slice**
“I’ll record a new take by holding Shift and tapping the row pad. The row mutes
while recording, then on stop it writes `source.raw` and immediately slices it
into eight files. That automatic slicing is why the grid stays stable—every
recording turns into a predictable 8‑step layout.”

Transition: “And because recording is live, we also need a safety net.”

---

**Undo + Reslice**
“Alt + Row swaps back the previous take if it exists, so a bad capture isn’t the
end of the world. And Shift+Alt+Step 6 reslices the current source without
touching your gates. The idea is to let you experiment without losing the
pattern you already built.”

Transition: “Next, the performance FX and why the audio interrupt stays boring.”

---

**Performance FX**
“Shift+Alt on steps 1 through 4 triggers filter, bitcrush, drive, and clear.
These effects feel immediate, but under the hood they’re precomputed in the
foreground, and the audio interrupt only reads deterministic tables. That’s why
the playback stays stable even when you hammer the controls.”

Transition: “Finally, we want demo rigs to be consistent every time.”

---

**Factory Restore**
“With a serial monitor open, pressing `f` restores the factory demo from the
manifest. It copies the known‑good slices into place and logs the status, so a
teaching board can be reset without reflashing firmware. That keeps workshops
and demos consistent.”

---

**Outro**
“That’s the system: clocked grid, expressive lanes, live capture, and safe
recovery, all built around a stable audio engine. If you want the full step‑by‑step
actions, check `docs/demo-exercises.md`, and I’ll add time‑stamped cues in the
README for quick navigation.”

# Examples that exercise the whole board

Think of this folder as the sampler's crashpad: quick scripts that light up the
Trellis without committing binary blobs to the repo. Everything here is meant to
cover the *entire* machine so you can shake down hardware, MIDI plumbing, and
LittleFS without prep work.

## Demo row generator

```
python examples/gen_demo_row_A.py
```

*Default:* renders all four rows (A–D) into `examples/demo_rows/`, with each row
getting its own `source.raw` and eight slices. Copy the contents of each row
folder straight onto the mounted Trellis drive (`/A`, `/B`, `/C`, `/D`) to sanity
check every pad.

*Selective:* want to mimic the original single-row flow? Aim it at one letter
and an output folder you care about:

```
python examples/gen_demo_row_A.py --rows A --outdir examples/demo_row_A
```

That keeps old docs happy and still uses the refreshed palettes. The goal is to
have a ready-made loop for every bank so you can practice recording, stutter,
and wipe behaviour without hunting for WAVs.

## MIDI clock sender

```
python examples/midi_clock_sender.py --out "NTM4 Sampler" --bpm 90
```

Shoots a barebones USB MIDI Start + Clock at the Trellis so you can test
quantized playback without a DAW. It is intentionally boring, because the point
is to prove sync works before the rest of your rig joins the party.

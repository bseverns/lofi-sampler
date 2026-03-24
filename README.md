# NeoTrellis M4 Lo-Fi Sampler

The NeoTrellis M4 Lo-Fi Sampler is a 4-row, 8-step hardware instrument built around USB MIDI clock, live capture, and equal-slice playback. Each row is one voice. Each voice advances in lockstep. The musical motion comes from how the source material differs, not from a deep sequencer UI.

This repo's canonical firmware path is **PlatformIO** under [`firmware/platformio/`](firmware/platformio). The current machine is working on real hardware: pads register, MIDI clock drives transport, bundled demo slices play, and the chorded modifier workflow keeps all eight step pads usable.

## Start Here
- Current status: [`CURRENT_STATE.md`](CURRENT_STATE.md)
- Controls: [`docs/controls.md`](docs/controls.md)
- Audio internals: [`docs/audio-engine.md`](docs/audio-engine.md)
- Demo/sample workflow: [`docs/demo-samples.md`](docs/demo-samples.md)
- Testing and CI: [`docs/testing.md`](docs/testing.md)
- Proof/listening surface: [`docs/ListeningGuide.md`](docs/ListeningGuide.md)

## Quick Start
1. Build the firmware:
   ```sh
   cd firmware/platformio
   pio run
   ```
2. Flash it:
   - First try `pio run -t upload`
   - If the SAM-BA port is finicky, double-tap reset, then build a UF2 and copy it to `TRELM4BOOT`:
     ```sh
     python scripts/bin_to_uf2.py \
       .pio/build/adafruit_trellis_m4/firmware.bin \
       0x4000 \
       .pio/build/adafruit_trellis_m4/firmware.uf2
     ```
3. Open the monitor:
   ```sh
   pio device monitor
   ```
4. Send MIDI Start + Clock from your DAW or from [`examples/midi_clock_sender.py`](examples/midi_clock_sender.py).
5. Use the bundled demo pack or record a new take on a row.

## What The Instrument Does
- 4 rows = 4 voices.
- 8 columns = 8 steps per bar.
- USB MIDI clock is the transport spine.
- Live recording writes a new `source.raw` and re-slices the row into eight equal regions.
- Step 7 and step 8 are usable musical steps; they only behave as modifiers when you hold them in a chord.

## Canonical Docs
- [`CURRENT_STATE.md`](CURRENT_STATE.md): honest project status, stable surfaces, and active edges.
- [`docs/controls.md`](docs/controls.md): canonical combo and pad behavior reference.
- [`docs/workflow.md`](docs/workflow.md): timing model, file layout, and subsystem interaction.
- [`docs/audio-engine.md`](docs/audio-engine.md): ISR/foreground split and audio job queue.
- [`docs/demo-samples.md`](docs/demo-samples.md): bundled demo slices, staging assets, and how to refresh them.
- [`docs/testing.md`](docs/testing.md): PlatformIO build, host tests, CI, and hardware smoke checks.
- [`docs/ListeningGuide.md`](docs/ListeningGuide.md): compact proof surface for what to listen for.
- [`docs/demo-exercises.md`](docs/demo-exercises.md): workshop-style teaching flow.
- [`docs/demo-script.md`](docs/demo-script.md): narrated walkthrough built from the same exercise order.

## Repo Map
```text
firmware/platformio/     Canonical firmware project
  src/                   Firmware modules
  lib/                   TinyUSB MIDI shim + bundled demo storage shim
  data/                  Canonical demo slice source assets
  scripts/               Firmware-side helpers (including UF2 generation)

docs/                    User, maintainer, and architecture docs
examples/                Host-side demo helpers
scripts/                 Host-side build/test wrappers
tests/                   Host tests for transport and routing
tools/                   Demo/sample preparation tools
```

## Commenting Policy
This repo does **not** aim for exhaustive line-by-line code comments. The canonical behavior belongs in the docs above. Code comments should explain invariants, hardware traps, ISR rules, and non-obvious design choices. See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the maintainer-facing version of that policy.

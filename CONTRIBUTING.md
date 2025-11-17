# Contributing to the NeoTrellis Lo-Fi Sampler

This repo is half lab notebook, half hands-on tutorial. Keep commits conversational but reproducible.

## Workflow
1. Fork + branch (`git checkout -b feature/whatever`).
2. Run the firmware on real hardware (or log your bench limitations in the PR).
3. Describe *why* you changed things in the commit message and PR body. The README doubles as a teaching aid, so document knobs you add.
4. For code, follow the existing formatting (2 spaces in `.ino` files, snake_case for helpers).
5. Please include reproduction steps + before/after audio notes if you tweak timing, slicing, or effects.

## Where to hack
- **Modes / pad combos:** See `firmware/arduino/lofi_sampler/lofi_sampler.ino` — the `handlePadCombo()` registry is where new behaviors land.
- **Effects / audio math:** `AudioEngine.*` holds the DAC ISR and buffer logic. Keep the ISR deterministic; push heavy work into `service()` jobs.
- **Storage layout:** `Storage.*` + `Slicer.*`. If you add formats beyond RAW, document the transfer flow in the README.
- **UI colors / pins / timing:** `Config.h` contains the pin map and global constants. Leave comments explaining any new pin choices.

## Adding tutorials or tools
- Drop hands-on walkthroughs into `docs/` and reference them from the README.
- Scripts (Python, Processing, etc.) belong in `tools/` or `examples/` depending on whether they are production helpers or learning aids.

## Testing checklist
- `examples/midi_clock_sender.py` can clock the board without a DAW — use it in demos.
- Verify LittleFS still formats and mounts after your changes (first boot serial log).
- Mash Alt/Shift combos while clocking to ensure your feature does not stall the UI loop.

Thanks for keeping the punk-rock sampler energy alive.

# Contributing To The NeoTrellis Lo-Fi Sampler

Keep the repo teachable, reproducible, and honest.

## Workflow
1. Branch from current main.
2. Build the firmware from `firmware/platformio/`.
3. Bench-test on real hardware when your change touches USB, storage, DAC, Trellis scanning, or recording.
4. Say why the change exists in the commit message and PR body.
5. If behavior changes, update the canonical docs in `README.md`, `CURRENT_STATE.md`, or `docs/` in the same change.

## Canonical Places To Edit
- Modes and controls: `firmware/platformio/src/main.cpp`, `PadInput.*`, `PadActionRouter.*`
- Audio path: `AudioEngine.*`
- Storage and slicing: `Storage.*`, `Slicer.*`, `RecordingController.*`
- Timing and constants: `Config.h`
- User-facing behavior docs: `docs/filesystem-contract.md`, `docs/controls.md`, `docs/workflow.md`, `CURRENT_STATE.md`

## Commenting Policy
Do not aim for exhaustive line-by-line comments.

Prefer:
- targeted comments for hardware traps
- ISR-safety notes
- queueing or storage invariants
- comments that explain why a weird tradeoff exists

Avoid:
- narrating every obvious assignment
- duplicating doc content in many files
- turning the code into a prose transcript

If behavior needs a broad explanation, put it in `docs/` and link to the relevant file.

## Testing Checklist
- Firmware build: `cd firmware/platformio && pio run -e adafruit_trellis_m4`
- Host tests: `./scripts/run_host_tests.sh`
- Hardware smoke checks:
  - board enumerates on USB serial and MIDI
  - pads register
  - MIDI clock advances playback
  - bundled demo slices play
  - recording and reslicing still work on at least one row

## Docs Matter
This repo is part instrument, part teaching tool, part firmware project. If you change controls, transport, sample loading, or architecture, update the docs in the same stack.

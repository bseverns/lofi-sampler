# Testing And CI

This is the canonical testing surface for the repo.

## Firmware Build
From repo root:

```sh
cd firmware/platformio
pio run -e adafruit_trellis_m4
```

That is the same primary firmware build lane used by CI.

## Host Tests
Run the host-side transport, routing, and filesystem-contract checks from repo root:

```sh
./scripts/run_host_tests.sh
```

Current host coverage:
- `tests/clock_transport_test.cpp`
- `tests/pad_action_router_test.cpp`
- `scripts/validate_filesystem_contract.py`

The filesystem validator checks host-visible v0.1 assumptions:
- staged demo data contains `/A` through `/D`, each with `source.raw` and eight playable slices
- row slices are 16-bit aligned, non-empty, and add up to the row source
- `BundledDemoSlices.h` exposes exactly 32 playable slice paths
- the bundled header excludes `source.raw`, `manifest.json`, and `/factory/*`
- `docs/filesystem-contract.md` keeps manifest/factory language demoted from the primary path

These tests are useful for control routing, transport math, and sample path assumptions, but they do not replace a hardware bench pass.

## Filesystem Workflow Validation
Use [`docs/receipts/2026-06-filesystem-workflow-pass.md`](receipts/2026-06-filesystem-workflow-pass.md) as the repeatable receipt format for filesystem workflow passes.

Automated checks:
- `cd firmware/platformio && pio run -e adafruit_trellis_m4`
- `./scripts/run_host_tests.sh`

Manual hardware smoke checklist:
- Record the firmware commit and whether the working tree was clean.
- Build and flash the `adafruit_trellis_m4` target.
- Open serial monitor and confirm the boot filesystem line is one of the expected contract states.
- Confirm a freshly flashed board plays bundled demo slices after MIDI Start + Clock.
- Record a short take on one row; confirm the row plays the new material.
- Reslice that row; confirm eight step slices still play in order.
- Record a second take on the same row, then restore; confirm the previous take returns.
- Treat `m=legacy manifest check` as diagnostic only.
- Treat `f=experimental factory restore` as unsupported unless a `/factory/*` tree was deliberately provisioned for that test.

## Hardware Smoke Checks
After flashing a board, verify:
- the board enumerates on USB serial
- the board enumerates as a MIDI destination
- pad presses register
- MIDI Start + Clock advance the step position
- bundled demo slices play audibly
- recording still writes a new source and fresh slices on a row
- restore swaps back the previous source after a second row take
- reslice rebuilds the row slices from row source material

## CI
CI lives in [`.github/workflows/firmware-build.yml`](../.github/workflows/firmware-build.yml).

Current CI expectations:
- PlatformIO installs cleanly
- the Trellis M4 firmware still builds
- the canonical source tree remains `firmware/platformio/`

## When To Add Tests
Add or extend host tests when you change:
- transport math
- combo routing
- row-level control semantics
- probability or velocity lane behavior

Bench-test on real hardware when you change:
- USB enumeration
- storage behavior
- DAC/ISR flow
- Trellis scanning or redraw cadence
- recording or slicing lifecycle

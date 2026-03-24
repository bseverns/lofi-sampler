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
Run the host-side transport and routing tests from repo root:

```sh
./scripts/run_host_tests.sh
```

Current host coverage:
- `tests/clock_transport_test.cpp`
- `tests/pad_action_router_test.cpp`

These tests are useful for control routing and transport math, but they do not replace a hardware bench pass.

## Hardware Smoke Checks
After flashing a board, verify:
- the board enumerates on USB serial
- the board enumerates as a MIDI destination
- pad presses register
- MIDI Start + Clock advance the step position
- bundled demo slices play audibly
- recording still writes a new source and fresh slices on a row

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

# Filesystem Workflow Pass - 2026-06

Date: 2026-06-17

## Test Subject

- Firmware commit tested: `aafd5ab` plus current working-tree filesystem workflow validation changes
- Board target: `adafruit_trellis_m4`
- Filesystem contract: [`docs/filesystem-contract.md`](../filesystem-contract.md)
- Receipt status: automated host/build proof added; hardware smoke pass still required on a physical NeoTrellis M4

## Build And Flash

- Firmware build command: `cd firmware/platformio && pio run -e adafruit_trellis_m4`
- Build result: pass
- Flash method: not performed in this host-only pass
- Serial monitor command: `cd firmware/platformio && pio device monitor`

## Automated Validation

- Host validation command: `./scripts/run_host_tests.sh`
- Host validation result: pass
- Filesystem validator coverage:
  - staged row paths exist for `/A` through `/D`
  - each row has `source.raw`
  - each row has eight playable slices
  - slice byte totals match `source.raw`
  - bundled header exposes exactly 32 playable slice paths
  - bundled header excludes `source.raw`, `/manifest.json`, and `/factory/*`

## Manual Hardware Smoke Checklist

- [ ] Board enumerated on USB serial.
- [ ] Board enumerated as a MIDI destination.
- [ ] Boot serial line reported expected filesystem status:
  - Expected fresh-flash line: `[filesystem] using bundled demo slices`
  - Actual line:
- [ ] Bundled demo playback worked after MIDI Start + Clock.
- [ ] Live recording produced row-local source material.
  - Evidence: newly recorded row played back new material.
  - Evidence: reslice on that row still produced playable slices.
- [ ] Reslicing produced eight row slices.
  - Row tested:
  - Evidence:
- [ ] Restore found and swapped `source_prev.raw`.
  - Method: record take 1, record take 2 on same row, trigger restore.
  - Expected: take 1 returns.
  - Actual:
- [ ] Restore-or-blank behavior checked when no `source_prev.raw` exists.
- [ ] Audio ISR/timing behavior remained stable during playback and recording.
- [ ] MIDI transport behavior remained stable during storage operations.

## Workflow Status

- Bundled demo playback: not run in this host-only pass; requires hardware.
- Live recording produced `source.raw`: not run in this host-only pass; requires hardware.
- Reslice produced eight row slices: host staged data validated; hardware reslice still required.
- Restore found and swapped `source_prev.raw`: not run in this host-only pass; requires hardware.
- Manifest support: legacy/internal diagnostic only via `m=legacy manifest check`.
- Factory restore support: experimental maintainer path only via `f=experimental factory restore`; not a public v0.1 workflow.
- `buildfs` / `uploadfs`: not a primary board path for v0.1.

## Known Failures Or Remaining Edges

- Hardware smoke evidence is not captured in this receipt yet.
- Persistence of live-recorded material across reboot remains a hardware/filesystem edge until explicitly tested.
- `/factory/*` restore remains unproven without a deliberately provisioned factory tree.

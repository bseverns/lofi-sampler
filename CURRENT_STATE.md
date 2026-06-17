# Current State

This file is the plain-language status board for the NeoTrellis M4 Lo-Fi Sampler.

## Stable
- Hardware path: working on real NeoTrellis M4 hardware.
- Firmware path: [`firmware/platformio/`](firmware/platformio) is the canonical build lane.
- Clocked playback: USB MIDI Start/Continue/Stop/Clock are working in the current firmware.
- Pad input: Trellis scanning, event packing, and chorded modifier handling are working.
- Audio output: the timer ISR, direct DAC writes, and slice playback are stable enough for normal use.
- Filesystem contract: freshly flashed firmware uses bundled demo slices for immediate playback; live recording writes row-local source/slice files.

## Canonical Paths
- Firmware: [`firmware/platformio/`](firmware/platformio)
- Filesystem contract: [`docs/filesystem-contract.md`](docs/filesystem-contract.md)
- Controls reference: [`docs/controls.md`](docs/controls.md)
- Audio architecture: [`docs/audio-engine.md`](docs/audio-engine.md)
- Testing: [`docs/testing.md`](docs/testing.md)
- Demo/sample pipeline: [`docs/demo-samples.md`](docs/demo-samples.md)

## Honest Edges
- The primary user path is bundled demo playback plus row-local live recording.
- `/manifest.json` is a legacy/internal diagnostic, not the v0.1 runtime contract.
- `buildfs` / `uploadfs` is not the primary board path for v0.1.
- `/factory/*` restore is experimental and should be treated as a maintainer topic.

## Testing Surface
- PlatformIO firmware build is active.
- Host tests for clock transport and pad routing are active and useful.
- CI builds the firmware on every PR through [`.github/workflows/firmware-build.yml`](.github/workflows/firmware-build.yml).

## Who This Repo Serves
- Performer: flash firmware, use the bundled demo slices, record rows as needed, send clock, and follow [`docs/controls.md`](docs/controls.md).
- Teacher or workshop lead: use [`docs/demo-exercises.md`](docs/demo-exercises.md), [`docs/demo-script.md`](docs/demo-script.md), and [`docs/ListeningGuide.md`](docs/ListeningGuide.md).
- Hacker or contributor: start with [`CONTRIBUTING.md`](CONTRIBUTING.md), then read [`docs/filesystem-contract.md`](docs/filesystem-contract.md), [`docs/audio-engine.md`](docs/audio-engine.md), and [`docs/testing.md`](docs/testing.md).

## Commenting Policy
The repo favors targeted comments plus strong external docs over exhaustive commentary in every function body. Use code comments for:
- hardware constraints
- ISR safety rules
- storage or transport invariants
- non-obvious tradeoffs

Do not try to make the codebase read like a line-by-line transcript. The canonical explanations live in the docs.

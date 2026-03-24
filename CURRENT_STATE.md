# Current State

This file is the plain-language status board for the NeoTrellis M4 Lo-Fi Sampler.

## Stable
- Hardware path: working on real NeoTrellis M4 hardware.
- Firmware path: [`firmware/platformio/`](firmware/platformio) is the canonical build lane.
- Clocked playback: USB MIDI Start/Continue/Stop/Clock are working in the current firmware.
- Pad input: Trellis scanning, event packing, and chorded modifier handling are working.
- Audio output: the timer ISR, direct DAC writes, and slice playback are stable enough for normal use.
- Demo playback surface: the shipped firmware includes a known-good bundled slice set for immediate playback after flashing.

## Canonical Paths
- Firmware: [`firmware/platformio/`](firmware/platformio)
- Controls reference: [`docs/controls.md`](docs/controls.md)
- Audio architecture: [`docs/audio-engine.md`](docs/audio-engine.md)
- Testing: [`docs/testing.md`](docs/testing.md)
- Demo/sample pipeline: [`docs/demo-samples.md`](docs/demo-samples.md)

## Honest Edges
- The current storage layer is not a fully settled persistent filesystem workflow yet.
- The current known-good lane is bundled read-only demo slices compiled into the firmware plus runtime-generated content from live recording.
- Older docs that assumed `buildfs`/`uploadfs` as the primary board path have been retired from the canonical surface.
- Advanced restore/factory-image workflows are still evolving and should be treated as a maintainer topic, not the primary performer workflow.

## Testing Surface
- PlatformIO firmware build is active.
- Host tests for clock transport and pad routing are active and useful.
- CI builds the firmware on every PR through [`.github/workflows/firmware-build.yml`](.github/workflows/firmware-build.yml).

## Who This Repo Serves
- Performer: use the bundled demo pack, send clock, and follow [`docs/controls.md`](docs/controls.md).
- Teacher or workshop lead: use [`docs/demo-exercises.md`](docs/demo-exercises.md), [`docs/demo-script.md`](docs/demo-script.md), and [`docs/ListeningGuide.md`](docs/ListeningGuide.md).
- Hacker or contributor: start with [`CONTRIBUTING.md`](CONTRIBUTING.md), then read [`docs/audio-engine.md`](docs/audio-engine.md) and [`docs/testing.md`](docs/testing.md).

## Commenting Policy
The repo favors targeted comments plus strong external docs over exhaustive commentary in every function body. Use code comments for:
- hardware constraints
- ISR safety rules
- storage or transport invariants
- non-obvious tradeoffs

Do not try to make the codebase read like a line-by-line transcript. The canonical explanations live in the docs.

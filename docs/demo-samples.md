# Demo Samples And Sample Loading

This page describes the current, honest sample-loading story.

## Current Known-Good Path
Freshly flashed firmware includes a bundled read-only demo slice set for playback. That means a board can boot, receive MIDI clock, and play the demo material without a separate filesystem upload step.

That bundled set is built from the canonical slice source tree at:
- [`firmware/platformio/data/`](../firmware/platformio/data/)

## Important Current-State Notes
- The old `buildfs` / `uploadfs` story is **not** the canonical board path today.
- The bundled demo pack is the trusted playback surface.
- Bundled demo playback does **not** mean every archival `/factory` or `source.raw` workflow is active in the same way as older docs implied.
- If you need row-local source material for reslice/restore experiments, record a fresh take on that row.

## Regenerating The Demo Pack
If you want to refresh the staged demo raws from WAV files:

```sh
python tools/build_demo_fs.py \
  --input-dir examples \
  --stage-dir firmware/platformio/data
```

That repopulates `firmware/platformio/data/` with per-row slices and `source.raw` staging files.

Then regenerate the bundled firmware header:

```sh
python tools/build_bundled_demo_header.py
```

Then rebuild the firmware:

```sh
cd firmware/platformio
pio run
```

## What Lives Where
- `firmware/platformio/data/`: canonical demo source assets in row folders
- `firmware/platformio/lib/Adafruit_LittleFS/src/generated/BundledDemoSlices.h`: generated bundled playback header compiled into firmware
- `examples/`: convenient source WAVs and host-side demo helpers

## When To Use Which Path
- Want a known-good board quickly: flash the firmware and use the bundled demo pack.
- Want to audition new WAV material: rebuild the staged data and bundled header, then rebuild firmware.
- Want to record on hardware: use the live record workflow and let the board write row-local source/slices.

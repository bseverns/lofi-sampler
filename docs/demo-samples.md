# Demo Samples And Sample Loading

For the runtime contract, see [`filesystem-contract.md`](filesystem-contract.md). This page is the maintainer guide for refreshing the bundled demo material.

## Current Board Path

The public v0.1 board path is:

1. Flash firmware.
2. Play the bundled demo slices immediately.
3. Record rows on hardware when you want live material.

There is no required `buildfs` / `uploadfs` step for normal use. The bundled slice header is the trusted demo playback surface.

## Refreshing Bundled Demo Material

Rebuild staged row data from WAV files:

```sh
python tools/build_demo_fs.py \
  --input-dir examples \
  --stage-dir firmware/platformio/data
```

That repopulates `firmware/platformio/data/` with host-side row slices and `source.raw` staging files.

Regenerate the bundled firmware header:

```sh
python tools/build_bundled_demo_header.py
```

Then rebuild firmware:

```sh
cd firmware/platformio
pio run -e adafruit_trellis_m4
```

## What Lives Where

- `firmware/platformio/data/`: host-side staging tree for demo source assets.
- `firmware/platformio/lib/Adafruit_LittleFS/src/generated/BundledDemoSlices.h`: generated slice pack compiled into firmware.
- `examples/`: convenient source WAVs and host-side demo helpers.

## What Is Not Primary

- `buildfs` / `uploadfs` is not the normal board path for v0.1.
- `/manifest.json` is a legacy/internal diagnostic artifact.
- `/factory/*` restore is experimental and should not be used as the public reset workflow.

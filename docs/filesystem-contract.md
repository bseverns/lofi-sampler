# Filesystem Contract

This is the v0.1 filesystem contract for the NeoTrellis M4 Lo-Fi Sampler.

## Primary User Path

Freshly flashed firmware must make sound without a separate filesystem upload. The firmware ships 32 bundled demo slices compiled into the local LittleFS layer:

- `/A/A1.raw` through `/A/A8.raw`
- `/B/B1.raw` through `/B/B8.raw`
- `/C/C1.raw` through `/C/C8.raw`
- `/D/D1.raw` through `/D/D8.raw`

Step playback and stutter read those paths directly. On boot, serial output reports the active slice source:

- `[filesystem] using bundled demo slices`
- `[filesystem] using live filesystem slices with row source files`
- `[filesystem] using experimental factory-restored slice set`
- `[filesystem] incomplete slice set: ...`

## Live Recording Path

Recording is row-local. When a row recording is committed, firmware writes:

- `/<Row>/source.raw`
- `/<Row>/<Row>1.raw` through `/<Row>/<Row>8.raw`

Before replacing an existing `source.raw`, firmware makes a best-effort backup at:

- `/<Row>/source_prev.raw`

Restore swaps `source_prev.raw` back into `source.raw` when available, then regenerates the row's eight slices. If no previous source exists, restore-or-blank removes that row's slices and source files.

Bundled demo slices do not include `source.raw`. Reslice and restore are only blessed for rows that have row source material from live recording or a maintainer-provisioned filesystem.

## Maintainer Path

Maintainers can rebuild the bundled demo pack from source WAV files:

```sh
python tools/build_demo_fs.py \
  --input-dir examples \
  --stage-dir firmware/platformio/data

python tools/build_bundled_demo_header.py

cd firmware/platformio
pio run -e adafruit_trellis_m4
```

`firmware/platformio/data/` is the host-side staging tree. `tools/build_bundled_demo_header.py` compiles the staged slice files into `firmware/platformio/lib/Adafruit_LittleFS/src/generated/BundledDemoSlices.h`. The generated header is the board's blessed demo playback source.

## Legacy And Experimental Paths

`/manifest.json` is a legacy/internal diagnostic artifact. It is not the v0.1 runtime contract, and missing manifest data must not block bundled demo playback.

`buildfs` / `uploadfs` is not the primary board path for v0.1. Do not document it as the normal user setup unless it is restored and tested as a first-class workflow.

`/factory/*` restore is experimental. The firmware still has a serial maintainer command for an explicitly provisioned factory tree, but no public workflow should rely on it until the factory source, marker behavior, and persistence are tested end to end.

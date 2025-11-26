# Demo samples + LittleFS bake

Welcome to the sampler's lunchbox lab notebook. This page is the recipe for
turning human-friendly WAVs into the bytes we actually flash onto the Trellis
M4's QSPI flash. Every time we cut a demo image we want auditors (and our
future selves) to know exactly what went down.

## What the pipeline does

1. **Validate and slice**: each row (A/B/C/D) gets a mono, 16-bit PCM, 22050 Hz
   WAV. The script cuts it into eight equal-ish slices plus a `source.raw`
   reference, dropping them into `firmware/platformio/data/<Row>/`.
2. **Document**: `manifest.json` lives next to the raws and records hashes,
   command line, and firmware tag.
3. **Pack**: `pio run -t buildfs` wraps the staging directory into
   `.pio/build/adafruit_trellis_m4/littlefs.bin`.
4. **Flash automatically**: `pio run -t upload` now triggers an `uploadfs`
   after firmware upload, so fresh boards boot with the baked demo set.

## Prep your WAVs

* Mono, 16-bit PCM, **22050 Hz**. If you're unsure, force it with SoX:
  ```sh
  sox input.wav -c1 -r22050 -b16 output.wav
  ```
* Name them `A.wav`, `B.wav`, `C.wav`, `D.wav` inside a working directory
  (defaults to `./examples/`). You can override per-row paths with `--row`.

## Run the bake

From the repo root:

```sh
python tools/build_demo_fs.py \
  --input-dir examples \
  --stage-dir firmware/platformio/data
```

This will:

* Clean and repopulate `firmware/platformio/data/` with `A/`, `B/`, `C/`, `D/`
  folders containing `source.raw` plus `X1.raw`..`X8.raw` slices.
* Write `firmware/platformio/data/manifest.json` with SHA-256s and the exact
  command line used.
* Invoke `pio run -t buildfs` to generate
  `.pio/build/adafruit_trellis_m4/littlefs.bin`.

If you just need the RAWs/manifest without building the filesystem image (e.g.,
for a quick hash audit), pass `--skip-buildfs`.

## Flashing a board

With the image generated, a normal upload will also flash the filesystem:

```sh
pio run -t upload
```

Behind the scenes `firmware/platformio/scripts/flash_fs.py` runs both `buildfs`
and `uploadfs` around the firmware upload step. If `littlefs.bin` is missing,
it will skip the filesystem upload and tell you to run the pipeline above.

## Provenance + tagging

* `manifest.json` includes `firmware_version` (from `git describe` unless you
  override with `--firmware-version`) and `render_command` so firmware tags can
  cite the exact demo payload.
* Commit the manifest alongside firmware tags. Treat it like liner notes: it's
  the authoritative record of what bytes shipped.

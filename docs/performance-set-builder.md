# Prepared Performance-Set Builder — Minimal Design

**Status: proposed; not implemented.** The first version extends the existing compiled `BundledDemoSlices.h` path. It does not create banks, upload to the device, decode on-device, or require new hardware.

## Input

One JSON set manifest supplies an identity, semantic version, optional initial 4×8 pattern, and either:

- four row source WAV files sliced equally into eight pads; or
- 32 explicit pad WAV assignments.

Every source is converted offline to mono, signed 16-bit PCM at 22.05 kHz. The builder applies explicit trim, gain, and optional edge fade before writing `/<Row>/<Row><Step>.raw`.

## Required validation

Before generating firmware, the builder must reject:

- missing or duplicate pad assignments;
- non-PCM conversion failures;
- samples exceeding the configured per-voice buffer/streaming constraints;
- non-finite or out-of-range gain values;
- a staged set exceeding the generated-header flash budget.

It emits SHA-256 for each source and generated raw asset, plus the resolved manifest. A missing or invalid set is an error: it must not silently substitute the demo pack.

## Reproducible output

The builder stages assets in a fresh directory, deterministically serializes the resolved manifest, then invokes `tools/build_bundled_demo_header.py`. The resulting header and PlatformIO environment form the reproducible firmware input. The build receipt records set ID/version, asset hashes, tool command, target environment, and PlatformIO flash/RAM output.

## Migration and benchmark gate

Keep `Storage` and `AudioEngine` unchanged for the first builder. A future immutable `SampleBank` may replace path construction only after a benchmark compares its 32-pad lookup and worst-case four-voice onset against the current compiled-asset path. Measure foreground preload duration, queue rejection count, and ISR underruns on hardware; do not migrate based on source-level estimates.

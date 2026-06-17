# Filesystem Workflow Audit

Truth-mapping pass for the current filesystem and sample-storage workflow. This is descriptive only; no behavior changes are proposed here.

## 1. Runtime Paths

The firmware expects four row directories:

- `/A`
- `/B`
- `/C`
- `/D`

Per row, playback and recording use:

- `/<Row>/<Row>1.raw` through `/<Row>/<Row>8.raw`
- `/<Row>/source.raw`
- `/<Row>/source_prev.raw`

The storage layer also knows about:

- `/manifest.json`
- `/factory/<Row>/<Row>N.raw`
- `/factory/<Row>/source.raw`

Audio files are signed 16-bit little-endian mono PCM at 22050 Hz.

## 2. Read-Only Versus Writable Material

Read-only bundled demo material:

- `firmware/platformio/lib/Adafruit_LittleFS/src/generated/BundledDemoSlices.h` contains 32 compiled demo slice files only: `/A/A1.raw` through `/D/D8.raw`.
- The local LittleFS shim seeds those bundled slices into its file table at `begin()`.
- These are playback-ready fallback/demo slices. They do not include `source.raw`, `source_prev.raw`, `manifest.json`, or `/factory/*`.

Writable runtime material:

- Recording and slicing write `/<Row>/<Row>1.raw` through `/<Row>/<Row>8.raw`.
- Recording writes `/<Row>/source.raw`.
- Before replacing `source.raw`, the storage layer best-effort copies the current source to `/<Row>/source_prev.raw`.
- In the current shim, writing a path that started as bundled read-only material converts that file record into writable data for the running session.

Important caveat: the checked-in LittleFS implementation is an in-memory shim seeded from compiled demo slices. Persistent on-device filesystem behavior is not proven by this audit.

## 3. Current Path Relationships

`/<Row>/<Row>N.raw`:

- This is the playback surface.
- Step playback and stutter build these paths directly from row and step index.
- The audio engine streams these files in the foreground service path; the ISR only mixes already-loaded samples.
- Bundled demo playback provides these paths without requiring a separate filesystem upload.

`/<Row>/source.raw`:

- This is the row's whole-take source material.
- Live recording commits write it, then slice it into the eight row files.
- Reslice reads it and rewrites the eight row slices.
- The bundled demo header does not include it, even though `firmware/platformio/data/` currently contains host-side staged `source.raw` files.

`/<Row>/source_prev.raw`:

- This is a one-step previous-take backup created when replacing `source.raw`.
- Restore swaps it back into `source.raw`, then reslices.
- If no previous source exists, restore-or-erase removes the row's slices plus source files.

`/manifest.json`:

- `tools/build_demo_fs.py` can generate it when rebuilding the staged data tree.
- The currently checked-in `firmware/platformio/data/` tree does not contain a manifest.
- At runtime, `checkLegacyManifest()` is an internal diagnostic and falls back to checking the default 32 slice paths plus four `source.raw` paths.
- The v0.1 boot diagnostic uses `checkSliceSet()` instead, so bundled-only boot reports the playable bundled slice set without treating missing source files as a user-facing warning.

`/factory/*`:

- `restoreExperimentalFactorySet()` expects a mirror of manifest/default paths under `/factory`.
- No checked-in data tree or bundled header currently provides `/factory/*`.
- The serial `f` command calls this path as an explicitly experimental maintainer command; without an actual factory tree it is not a blessed performer workflow.

## 4. Known-Good Workflows

- Fresh firmware playback from bundled demo slices: known-good and documented as the current playback surface.
- Clocked step playback of `/<Row>/<Row>N.raw` paths: code-proven through `StepPlaybackController`, `PadActionRouter`, `AudioEngine`, and the bundled slice seed.
- Host-side regeneration of staged row data from WAV files: `tools/build_demo_fs.py` writes row slices and `source.raw` files under `firmware/platformio/data/`.
- Host-side regeneration of the compiled demo header: `tools/build_bundled_demo_header.py` reads staged slice files, excludes `source.raw`, and emits `BundledDemoSlices.h`.
- Live recording commit within a running firmware session: code writes source plus slices and preserves the previous source when present.

## 5. Implied But Not Proven

- Persistence of live-recorded content across reboot or power cycle.
- Using `buildfs` / `uploadfs` as the primary board-loading workflow.
- `/manifest.json` as a complete runtime contract for the public workflow.
- `/factory/*` restore as a working board feature without a separately provisioned factory tree.
- Reslicing bundled demo rows before a live `source.raw` exists. The bundled path has slices, not sources.
- Restore behavior after deleting or overwriting bundled slice paths, especially across sessions.

## 6. Assumptions To Retire Or Rename

- Retire "buildfs/uploadfs is the normal user path." The normal path is compiled bundled demo slices plus live recording.
- Rename "factory restore" as "experimental maintainer restore" until `/factory/*` is actually provisioned and tested.
- Do not describe bundled demo playback as a full filesystem image. It is a compiled slice pack.
- Do not imply bundled demo rows have `source.raw` available for reslice/restore.
- Treat `manifest.json` as a staging/diagnostic artifact for now, not the canonical runtime loader.

## 7. Smallest Safe v0.1 Public Workflow

Bless this:

1. Flash the firmware.
2. Use bundled demo slices for immediate playback.
3. Send USB MIDI clock/start/stop.
4. Toggle row steps; playback reads `/<Row>/<Row>N.raw`.
5. Record a row on hardware; the firmware writes `source.raw` and fresh row slices.
6. Use reslice/restore only for rows that have been recorded during the current workflow.
7. For new bundled demo material, regenerate staged data with `tools/build_demo_fs.py`, regenerate `BundledDemoSlices.h`, then rebuild firmware.

Do not bless this yet:

- Public `buildfs` / `uploadfs` sample installation.
- Public `/factory/*` reset.
- Public guarantees around persistent recorded samples across reboot.
- Public reslice/restore guarantees for untouched bundled demo rows.

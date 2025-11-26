#!/usr/bin/env python3
"""
Build a deterministic LittleFS image packed with demo slices.

This script is a fussy, repeatable pipeline that:
- Validates/ingests per-row WAV files.
- Emits RAW slices in the firmware's expected layout (A/A1.raw..A8.raw etc.).
- Drops a manifest.json documenting exactly what was baked.
- Invokes PlatformIO's `buildfs` target so `littlefs.bin` is ready to flash.
"""
import argparse
import datetime
import hashlib
import json
import shutil
import subprocess
import sys
import textwrap
from pathlib import Path
import wave

ROWS = ("A", "B", "C", "D")
SLICES = 8
EXPECTED_RATE = 22050
EXPECTED_WIDTH = 2
EXPECTED_CHANNELS = 1

REPO_ROOT = Path(__file__).resolve().parent.parent
PLATFORMIO_DIR = REPO_ROOT / "firmware" / "platformio"
DATA_DIR = PLATFORMIO_DIR / "data"


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def _read_wav(path: Path):
    with wave.open(str(path), "rb") as w:
        channels = w.getnchannels()
        sample_width = w.getsampwidth()
        framerate = w.getframerate()
        frames = w.getnframes()
        pcm = w.readframes(frames)
    if channels != EXPECTED_CHANNELS or sample_width != EXPECTED_WIDTH:
        raise SystemExit(f"{path} must be mono 16-bit PCM (got {channels}ch, {sample_width*8}-bit)")
    if framerate != EXPECTED_RATE:
        raise SystemExit(f"{path} must be {EXPECTED_RATE} Hz (got {framerate})")
    return frames, pcm


def _write_row(row: str, pcm_bytes: bytes, stage_root: Path):
    row_dir = stage_root / row
    row_dir.mkdir(parents=True, exist_ok=True)

    # write source.raw
    source_path = row_dir / "source.raw"
    source_path.write_bytes(pcm_bytes)

    # slice into eight even-ish chunks; the last slice soaks any remainder
    bytes_per_sample = EXPECTED_WIDTH
    total_samples = len(pcm_bytes) // bytes_per_sample
    base_seg = total_samples // SLICES
    remainder = total_samples - (base_seg * SLICES)

    for i in range(SLICES):
        seg = base_seg
        if i == SLICES - 1:
            seg += remainder
        start = i * base_seg * bytes_per_sample
        end = start + seg * bytes_per_sample
        slice_bytes = pcm_bytes[start:end]
        slice_path = row_dir / f"{row}{i+1}.raw"
        slice_path.write_bytes(slice_bytes)

    return {
        "source.raw": _sha256(source_path),
        **{
            f"{row}{i+1}.raw": _sha256(row_dir / f"{row}{i+1}.raw")
            for i in range(SLICES)
        },
    }, total_samples


def _discover_rows(input_dir: Path, explicit: dict[str, Path]) -> dict[str, Path]:
    mapping: dict[str, Path] = {}
    mapping.update(explicit)
    for row in ROWS:
        if row in mapping:
            continue
        candidate = input_dir / f"{row}.wav"
        if candidate.exists():
            mapping[row] = candidate
    return mapping


def build_manifest(rows, render_command, firmware_version, littlefs_bin: Path | None):
    return {
        "generated_at": datetime.datetime.utcnow().isoformat() + "Z",
        "firmware_version": firmware_version,
        "render_command": render_command,
        "rows": rows,
        "littlefs_image": None
        if littlefs_bin is None
        else {
            "path": str(littlefs_bin.relative_to(REPO_ROOT)),
            "size_bytes": littlefs_bin.stat().st_size,
            "sha256": _sha256(littlefs_bin),
        },
    }


def _git_describe():
    try:
        out = subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty"], cwd=REPO_ROOT)
        return out.decode().strip()
    except Exception:
        return "unknown"


def _buildfs():
    print("[buildfs] Running PlatformIO buildfs...")
    subprocess.run(["pio", "run", "-t", "buildfs"], cwd=PLATFORMIO_DIR, check=True)
    return PLATFORMIO_DIR / ".pio" / "build" / "adafruit_trellis_m4" / "littlefs.bin"


def main():
    parser = argparse.ArgumentParser(
        description=textwrap.dedent(
            """
            Turn per-row WAV files into RAW slices and a flashable LittleFS image.

            By default the script looks for A.wav/B.wav/C.wav/D.wav inside --input-dir
            and writes the resulting RAW tree into firmware/platformio/data.
            """
        )
    )
    parser.add_argument("--input-dir", type=Path, default=REPO_ROOT / "examples",
                        help="Where to search for A.wav/B.wav/etc. (default: ./examples)")
    parser.add_argument(
        "--row",
        action="append",
        default=[],
        help="Explicit row mapping like A:/path/to/file.wav. Overrides --input-dir discovery.",
    )
    parser.add_argument("--stage-dir", type=Path, default=DATA_DIR,
                        help="Where to write RAWs and manifest.json (default: firmware/platformio/data)")
    parser.add_argument("--skip-buildfs", action="store_true", help="Skip calling PlatformIO buildfs")
    parser.add_argument("--firmware-version", help="Override firmware tag recorded in the manifest")
    args = parser.parse_args()

    explicit_rows: dict[str, Path] = {}
    for entry in args.row:
        if ":" not in entry:
            parser.error("--row expects ROW:/abs/path/to/file.wav")
        row, path = entry.split(":", 1)
        row = row.upper()
        if row not in ROWS:
            parser.error(f"Row must be one of {ROWS} (got {row})")
        explicit_rows[row] = Path(path)

    discovered = _discover_rows(args.input_dir, explicit_rows)
    missing = [r for r in ROWS if r not in discovered]
    if missing:
        parser.error(f"Missing WAVs for rows: {', '.join(missing)}")

    if args.stage_dir.exists():
        print(f"[stage] Cleaning {args.stage_dir}")
        shutil.rmtree(args.stage_dir)
    args.stage_dir.mkdir(parents=True, exist_ok=True)

    rows_manifest = {}
    for row, wav_path in sorted(discovered.items()):
        if not wav_path.exists():
            parser.error(f"Row {row} file does not exist: {wav_path}")
        print(f"[row {row}] Loading {wav_path}")
        frames, pcm = _read_wav(wav_path)
        slice_hashes, total_samples = _write_row(row, pcm, args.stage_dir)
        rows_manifest[row] = {
            "source_path": str(wav_path),
            "source_sha256": _sha256(wav_path),
            "frames": frames,
            "samples": total_samples,
            "slice_hashes": slice_hashes,
        }

    firmware_version = args.firmware_version or _git_describe()
    render_command = "python " + " ".join(map(str, [Path(__file__).resolve()]))
    if len(sys.argv) > 1:
        render_command = "python " + " ".join(sys.argv)

    littlefs_bin = None
    if not args.skip_buildfs:
        littlefs_bin = _buildfs()

    manifest = build_manifest(rows_manifest, render_command, firmware_version, littlefs_bin)
    manifest_path = args.stage_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2))
    print(f"[manifest] Wrote {manifest_path}")
    if littlefs_bin:
        print(f"[buildfs] Image ready at {littlefs_bin}")


if __name__ == "__main__":
    main()

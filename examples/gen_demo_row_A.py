#!/usr/bin/env python3
"""Generate playable RAW slices for one *or all* Trellis rows.

The sampler has four banks (A–D) with eight slots each. Earlier drafts of this
script only minted a single "Row A" palette, which meant folks testing fresh
hardware couldn't push every pad without digging up their own audio. This
version fills the whole board: every row gets a distinct palette so you can
check cross-row playback, stutter, and recording without shipping any WAVs.

Why keep the old filename? Backwards compat. You can still target a single row
with `--rows A --outdir examples/demo_row_A` and copy those files to `/A/` on
the Trellis. Default behaviour now burns all four rows into `examples/demo_rows`.
"""

import argparse
import math
from pathlib import Path
from typing import Iterable

SAMPLE_RATE = 22050
SLICE_COUNT = 8
DEFAULT_SECONDS = 2.56  # divisible by 8 → 0.32 s slices

# Keep the palettes simple: one chord-ish sweep, one octave-up lead, one bass,
# one percussive click bed. They are intentionally lo-fi so we never hide
# aliasing or timing hiccups the hardware might reveal.
ROW_PROFILES = {
    "A": {
        "freqs": [110.0, 138.59, 165.0, 196.0, 220.0, 246.94, 277.18, 329.63],
        "wobble_hz": 0.5,
        "amplitude": 0.28,
        "label": "drifty chords",
    },
    "B": {
        "freqs": [220.0, 246.94, 261.63, 293.66, 329.63, 369.99, 415.3, 466.16],
        "wobble_hz": 0.75,
        "amplitude": 0.22,
        "label": "mid synth lead",
    },
    "C": {
        "freqs": [55.0, 69.3, 82.4, 98.0, 110.0, 123.5, 146.8, 164.8],
        "wobble_hz": 0.35,
        "amplitude": 0.32,
        "label": "bass bed",
    },
    "D": {
        "freqs": [440.0, 523.25, 659.25, 880.0, 587.33, 698.46, 783.99, 987.77],
        "wobble_hz": 1.0,
        "amplitude": 0.2,
        "label": "bright clicks",
    },
}


def synth_sample(row: str, duration: float) -> bytearray:
    profile = ROW_PROFILES[row]
    freqs = profile["freqs"]
    wobble_hz = profile["wobble_hz"]
    amplitude = profile["amplitude"]

    total_samples = int(SAMPLE_RATE * duration)
    buf = bytearray()
    for i in range(total_samples):
        slice_idx = (i * SLICE_COUNT) // total_samples
        freq = freqs[slice_idx % len(freqs)]
        t = i / SAMPLE_RATE
        wobble = 0.04 * math.sin(2 * math.pi * wobble_hz * t)
        amp = amplitude + wobble
        sample = int(amp * 32767 * math.sin(2 * math.pi * freq * t))
        buf += int(sample).to_bytes(2, "little", signed=True)
    return buf


def write_slices(row: str, raw_bytes: bytes, outdir: Path) -> None:
    samples = len(raw_bytes) // 2
    slice_samples = samples // SLICE_COUNT
    for idx in range(SLICE_COUNT):
        start = idx * slice_samples * 2
        end = start + slice_samples * 2
        name = f"{row}{idx + 1}.raw"
        (outdir / name).write_bytes(raw_bytes[start:end])


def generate_rows(rows: Iterable[str], outdir: Path, seconds: float) -> None:
    outdir.mkdir(parents=True, exist_ok=True)
    for row in rows:
        if row not in ROW_PROFILES:
            raise SystemExit(f"Row {row} is not valid. Choose from {sorted(ROW_PROFILES)}")

        row_dir = outdir / row
        row_dir.mkdir(parents=True, exist_ok=True)
        raw = synth_sample(row, seconds)
        source_path = row_dir / "source.raw"
        source_path.write_bytes(raw)
        write_slices(row, raw, row_dir)
        print(f"Wrote {source_path} and {SLICE_COUNT} slices ({ROW_PROFILES[row]['label']})")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--rows",
        nargs="+",
        default=sorted(ROW_PROFILES),
        choices=sorted(ROW_PROFILES),
        help="One or more rows to render (default: all rows)",
    )
    parser.add_argument(
        "--outdir",
        type=Path,
        default=Path("examples/demo_rows"),
        help="Directory to place per-row folders full of slices",
    )
    parser.add_argument(
        "--seconds",
        type=float,
        default=DEFAULT_SECONDS,
        help="Duration for each generated loop (default: %(default)s)",
    )
    args = parser.parse_args()

    generate_rows(args.rows, args.outdir, args.seconds)


if __name__ == "__main__":
    main()

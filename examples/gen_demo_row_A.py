#!/usr/bin/env python3
"""Generate a simple demo loop for row A without shipping binary RAW/WAV assets."""
import argparse
import math
from pathlib import Path

SAMPLE_RATE = 22050
ROW = "A"
SLICE_COUNT = 8
DEFAULT_SECONDS = 2.56  # divisible by 8 → 0.32 s slices
# palette of frequencies that loosely mimic a chord stab that drifts per slice
FREQS = [110.0, 138.59, 165.0, 196.0, 220.0, 246.94, 277.18, 329.63]


def synth_sample(duration: float) -> bytearray:
    total_samples = int(SAMPLE_RATE * duration)
    buf = bytearray()
    for i in range(total_samples):
        slice_idx = (i * SLICE_COUNT) // total_samples
        freq = FREQS[slice_idx % len(FREQS)]
        t = i / SAMPLE_RATE
        amp = 0.28 + 0.04 * math.sin(2 * math.pi * 0.5 * t)
        sample = int(amp * 32767 * math.sin(2 * math.pi * freq * t))
        buf += int(sample).to_bytes(2, "little", signed=True)
    return buf


def write_slices(raw_bytes: bytes, outdir: Path) -> None:
    samples = len(raw_bytes) // 2
    slice_samples = samples // SLICE_COUNT
    for idx in range(SLICE_COUNT):
        start = idx * slice_samples * 2
        end = start + slice_samples * 2
        name = f"{ROW}{idx + 1}.raw"
        (outdir / name).write_bytes(raw_bytes[start:end])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--outdir", type=Path, default=Path("examples/demo_row_A"),
                        help="Directory to place source.raw and the 8 slices")
    parser.add_argument("--seconds", type=float, default=DEFAULT_SECONDS,
                        help="Duration for the generated loop (default: %(default)s)")
    args = parser.parse_args()

    args.outdir.mkdir(parents=True, exist_ok=True)
    raw = synth_sample(args.seconds)
    source_path = args.outdir / "source.raw"
    source_path.write_bytes(raw)
    write_slices(raw, args.outdir)
    print(f"Wrote {source_path} and {SLICE_COUNT} slices (row {ROW})")


if __name__ == "__main__":
    main()

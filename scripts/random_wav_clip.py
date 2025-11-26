#!/usr/bin/env python3
"""
Randomly snip an in-bounds clip from a mono 16-bit 22,050 Hz WAV.

We keep things blunt and studio-notebook honest: no resampling, no format
conversions, just pick a legal window and spit it back out. The slicer in
`tools/wav_to_raw_slices.py` expects this exact format, so we refuse anything
else.
"""
import argparse
import pathlib
import random
import wave


EXPECTED_CHANNELS = 1
EXPECTED_SAMPLE_WIDTH = 2  # bytes, so 16-bit
EXPECTED_RATE = 22050


def validate_wav_params(params):
    channels, sample_width, frame_rate, total_frames, _, _ = params
    if channels != EXPECTED_CHANNELS or sample_width != EXPECTED_SAMPLE_WIDTH:
        raise SystemExit("WAV must be mono 16-bit PCM")
    if frame_rate != EXPECTED_RATE:
        raise SystemExit("Please resample to 22050 Hz first")
    return total_frames


def pick_start(total_frames, frames_needed, rng):
    max_start = total_frames - frames_needed
    if max_start < 0:
        raise SystemExit("Source is shorter than the requested clip length")
    return rng.randint(0, max_start)


def slice_random_window(src_wav, out_wav, target_seconds, seed=None):
    rng = random.Random(seed)
    with wave.open(src_wav, "rb") as wav:
        total_frames = validate_wav_params(wav.getparams())
        frames_needed = int(EXPECTED_RATE * target_seconds)
        start = pick_start(total_frames, frames_needed, rng)
        wav.setpos(start)
        clip = wav.readframes(frames_needed)

    out_path = pathlib.Path(out_wav)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(out_path, "wb") as out:
        out.setparams(
            (EXPECTED_CHANNELS, EXPECTED_SAMPLE_WIDTH, EXPECTED_RATE, frames_needed, "NONE", "not compressed")
        )
        out.writeframes(clip)

    print(
        f"Wrote {out_path} ({target_seconds:.2f}s) starting at frame {start} of {total_frames}"
    )


def main():
    parser = argparse.ArgumentParser(description="Clip a random in-bounds window from a mono 16-bit 22,050 Hz WAV")
    parser.add_argument("wav", help="mono 16-bit PCM @ 22,050 Hz")
    parser.add_argument("--out", required=True, help="output WAV path")
    parser.add_argument("--seconds", type=float, required=True, help="clip duration in seconds")
    parser.add_argument("--seed", type=int, help="optional random seed for reproducible slices")
    args = parser.parse_args()

    if args.seconds <= 0:
        raise SystemExit("Clip duration must be positive")

    slice_random_window(args.wav, args.out, args.seconds, seed=args.seed)


if __name__ == "__main__":
    main()

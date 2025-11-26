# Scripts: quick-and-dirty studio helpers

We stash standalone utilities here so you can riff without cracking open the
firmware. Think of this folder as the little black book you keep in the booth:
short notes, direct actions, minimal ceremony.

## `random_wav_clip.py`

Chops a random, in-bounds slice out of a mono 16-bit 22,050 Hz WAV—perfect for
feeding the rest of the toolchain or just auditioning micro-moments.

### Why it exists
- The firmware only wants slices that are already the right format.
- Sometimes your source track is longer than a pad will hold.
- Random starts keep things vibey; a `--seed` keeps it repeatable when you need
  determinism.

### Usage
```bash
./random_wav_clip.py path/to/source.wav \
  --seconds 3.5 \     # length of the clip you want
  --out out/clip.wav \ # where to write the snippet
  --seed 13            # optional: lock in a start point
```

If the file isn’t mono 16-bit PCM @ 22,050 Hz, the script bails—no silent
conversions, no surprises.

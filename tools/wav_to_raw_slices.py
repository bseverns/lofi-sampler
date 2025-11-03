
#!/usr/bin/env python3
import argparse, os, wave, struct

def convert_and_slice(wav_path, outdir, prefix, segments=8):
    w = wave.open(wav_path, 'rb')
    ch = w.getnchannels()
    sw = w.getsampwidth()
    fr = w.getframerate()
    if ch != 1 or sw != 2:
        raise SystemExit('WAV must be mono 16-bit PCM')
    if fr != 22050:
        raise SystemExit('Please resample to 22050 Hz first')
    nf = w.getnframes()
    seg = nf // segments
    os.makedirs(outdir, exist_ok=True)
    # write source.raw
    src = os.path.join(outdir, 'source.raw')
    with open(src, 'wb') as f:
        f.write(w.readframes(nf))
    # slice
    w.rewind()
    for i in range(segments):
        w.setpos(i*seg)
        frames = w.readframes(seg)
        with open(os.path.join(outdir, f'{prefix}{i+1}.raw'), 'wb') as o:
            o.write(frames)
    w.close()
    print('Wrote', src, 'and', segments, 'slices.')

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('wav', help='mono 16-bit PCM @ 22050 Hz')
    ap.add_argument('--outdir', required=True)
    ap.add_argument('--prefix', required=True, help='Row letter: A, B, C, or D')
    args = ap.parse_args()
    convert_and_slice(args.wav, args.outdir, args.prefix)

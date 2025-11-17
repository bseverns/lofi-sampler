#!/usr/bin/env python3
"""Send a dead-simple USB MIDI clock + transport to the NeoTrellis M4.

Requires `mido` + a backend such as `python-rtmidi`:

    python -m pip install mido python-rtmidi

Run with the device name exposed by the Trellis, e.g.

    python examples/midi_clock_sender.py --out "NTM4 Sampler"
"""
import argparse
import time
import mido

PPQN = 24
BPM_DEFAULT = 96


def main():
    ap = argparse.ArgumentParser(description="Send MIDI clock + transport to the Trellis")
    ap.add_argument("--out", required=True, help="MIDI output port name (see mido.get_output_names())")
    ap.add_argument("--bpm", type=float, default=BPM_DEFAULT, help="Tempo in beats per minute")
    args = ap.parse_args()

    period = 60.0 / (args.bpm * PPQN)
    print(f"Opening {args.out} @ {args.bpm:.1f} BPM (clock every {period*1000:.2f} ms)")

    with mido.open_output(args.out) as port:
        port.send(mido.Message('start'))
        try:
            while True:
                port.send(mido.Message('clock'))
                time.sleep(period)
        except KeyboardInterrupt:
            port.send(mido.Message('stop'))
            print("Stopped clock")


if __name__ == "__main__":
    main()

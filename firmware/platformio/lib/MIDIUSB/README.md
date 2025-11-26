# TinyUSB-friendly `MIDIUSB.h`

This folder ships a **shim**, not the legacy Arduino MIDIUSB stack. We still
tell PlatformIO to `lib_ignore = MIDIUSB` so the old PluggableUSB MIDI driver
stays out of the link. The shim simply re-exports the handful of types and
methods that *Adafruit_NeoTrellisM4* insists on, and punts the real work to the
TinyUSB-backed `Adafruit_USBD_MIDI` instance you already create in `main.cpp`.

The goal: keep includes like `#include <MIDIUSB.h>` compiling while avoiding
USB stack collisions. If you need to poke it yourself:

```cpp
// You should already have this somewhere in your sketch:
Adafruit_USBD_MIDI usb_midi;

// The shim makes MidiUSB behave like the old global, but forwards to usb_midi.
// It hunts for whatever transmit symbol TinyUSB publishes in this version:
// sendMIDI(...) -> send(...) -> write(...).
MidiUSB.sendMIDI({0x09, 0x90, 60, 127});
MidiUSB.flush();

// Clock follower? Keep reading usb_midi (or MidiUSB.read()) for 0xF8..0xFC
// realtime messages and let the sequencer march to that beat.
// Message sender? Call MidiUSB.sendMIDI(...) with a 4-byte packet and the
// shim will slam it through the TinyUSB path without inventing a second USB
// stack.
```

A little punk-rock but plenty practical: one header, no extra baggage, and your
TinyUSB pipeline stays the single source of truth for MIDI.

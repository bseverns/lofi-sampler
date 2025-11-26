#pragma once

// TinyUSB-first MIDIUSB compatibility shim.
// The stock Arduino MIDIUSB library fights with TinyUSB's USB stack, but
// Adafruit's TinyUSB implementation already exposes a rich MIDI interface.
// This header keeps legacy "MIDIUSB.h" includes happy while routing calls to
// the TinyUSB-backed Adafruit_USBD_MIDI instance you define in your sketch.

#include <Adafruit_TinyUSB.h>

// The firmware already instantiates a global `Adafruit_USBD_MIDI usb_midi;`.
// Declare it here so inline methods can forward to it without pulling in the
// original MIDIUSB implementation (which would collide with TinyUSB symbols).
extern Adafruit_USBD_MIDI usb_midi;

// The TinyUSB library defines midiEventPacket_t for us. We reuse it directly
// to stay ABI-compatible with the rest of the TinyUSB stack and avoid copying
// buffers around.
using midiEventPacket_t = decltype(usb_midi.read());

class MIDIUSB_t {
public:
  // Mirror the MIDIUSB API surface that the NeoTrellis M4 library expects.
  // Each call forwards straight to the TinyUSB MIDI device so behavior stays
  // identical, just without the duplicate USB stack.
  midiEventPacket_t read(void) { return usb_midi.read(); }

  void sendMIDI(const midiEventPacket_t &event) { usb_midi.sendMIDI(event); }

  void flush(void) { usb_midi.flush(); }

  uint32_t available(void) { return usb_midi.available(); }
};

// The legacy header exposes a global MidiUSB instance. We keep that contract,
// but implement it inline so there is no separate translation unit to ship.
[[maybe_unused]] static MIDIUSB_t MidiUSB;


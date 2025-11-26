#pragma once

// TinyUSB-first MIDIUSB compatibility shim.
// The stock Arduino MIDIUSB library fights with TinyUSB's USB stack, but
// Adafruit's TinyUSB implementation already exposes a rich MIDI interface.
// This header keeps legacy "MIDIUSB.h" includes happy while routing calls to
// the TinyUSB-backed Adafruit_USBD_MIDI instance you define in your sketch.

#include <Adafruit_TinyUSB.h>
#include "class/midi/midi_device.h"
#include <cstring>
#include <type_traits>

// The firmware already instantiates a global `Adafruit_USBD_MIDI usb_midi;`.
// Declare it here so inline methods can forward to it without pulling in the
// original MIDIUSB implementation (which would collide with TinyUSB symbols).
extern Adafruit_USBD_MIDI usb_midi;

// TinyUSB exposes a canonical 4-byte USB MIDI packet (tud_midi_packet_t).
// Re-export it here so callers see a concrete type rather than a decltype()
// deduction from whatever Adafruit_USBD_MIDI::read() happens to return.
using tinyusb_midi_packet_t = tud_midi_packet_t;
using midiEventPacket_t = tinyusb_midi_packet_t;

class MIDIUSB_t {
public:
  // Mirror the MIDIUSB API surface that the NeoTrellis M4 library expects.
  // Each call forwards straight to the TinyUSB MIDI device so behavior stays
  // identical, just without the duplicate USB stack.
  midiEventPacket_t read(void) { return adaptPacket(usb_midi.read()); }

  void sendMIDI(const midiEventPacket_t &event) {
    // TinyUSB's Adafruit_USBD_MIDI keeps evolving: older drops expose
    // sendMIDI(event), newer ones rename it to send(event), and the lowest
    // common denominator is a raw write() of the 4-byte packet. Use a tiny bit
    // of SFINAE to route to whatever symbol this build of Adafruit_TinyUSB
    // actually provides without forcing callers to update their includes.
    forwardSend(usb_midi, event, 0);
  }

  void flush(void) { usb_midi.flush(); }

  uint32_t available(void) { return usb_midi.available(); }

private:
  static midiEventPacket_t adaptPacket(const midiEventPacket_t &packet) {
    return packet;
  }

  template <typename RawPacket>
  static midiEventPacket_t adaptPacket(const RawPacket &raw) {
    static_assert(sizeof(RawPacket) == sizeof(midiEventPacket_t),
                  "Adafruit_USBD_MIDI::read() must return a 4-byte packet");

    midiEventPacket_t packet{};
    std::memcpy(&packet, &raw, sizeof(packet));
    return packet;
  }

  template <typename Midi>
  static auto forwardSend(Midi &midi, const midiEventPacket_t &event,
                          int) -> decltype(midi.sendMIDI(event), void()) {
    midi.sendMIDI(event);
  }

  template <typename Midi>
  static auto forwardSend(Midi &midi, const midiEventPacket_t &event,
                          long) -> decltype(midi.send(event), void()) {
    midi.send(event);
  }

  template <typename Midi>
  static auto forwardSend(Midi &midi, const midiEventPacket_t &event, char)
      -> decltype(midi.write(reinterpret_cast<const uint8_t *>(&event),
                             sizeof(event)),
                  void()) {
    midi.write(reinterpret_cast<const uint8_t *>(&event), sizeof(event));
  }

  template <typename Midi>
  static void forwardSend(Midi &, const midiEventPacket_t &, ...) {
#if defined(ARDUINO)
#warning "No Adafruit_USBD_MIDI send routine detected; MIDIUSB shim sendMIDI() will be a no-op."
#endif
  }
};

// The legacy header exposes a global MidiUSB instance. We keep that contract,
// but implement it inline so there is no separate translation unit to ship.
[[maybe_unused]] static MIDIUSB_t MidiUSB;


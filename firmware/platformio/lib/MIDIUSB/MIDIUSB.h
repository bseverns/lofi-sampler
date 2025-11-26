#pragma once

// TinyUSB-first MIDIUSB compatibility shim.
// The stock Arduino MIDIUSB library fights with TinyUSB's USB stack, but
// Adafruit's TinyUSB implementation already exposes a rich MIDI interface.
// This header keeps legacy "MIDIUSB.h" includes happy while routing calls to
// the TinyUSB-backed Adafruit_USBD_MIDI instance you define in your sketch.

#include <Adafruit_TinyUSB.h>
#include <cstring>
#include <type_traits>
#include <utility>

// Pull in the TinyUSB MIDI packet definition directly so we can surface the
// exact packet type TinyUSB exports (newer library releases renamed
// tud_midi_packet_t to midi_packet_t).
#if __has_include(<class/midi/midi_device.h>)
#include <class/midi/midi_device.h>
#elif __has_include(<tusb_midi.h>)
#include <tusb_midi.h>
#endif

// The firmware already instantiates a global `Adafruit_USBD_MIDI usb_midi;`.
// Declare it here so inline methods can forward to it without pulling in the
// original MIDIUSB implementation (which would collide with TinyUSB symbols).
extern Adafruit_USBD_MIDI usb_midi;

// TinyUSB exposes a canonical 4-byte USB MIDI packet; newer releases export it
// as midi_packet_t, while older drops used tud_midi_packet_t. Re-export the
// available symbol so callers see a concrete type rather than a decltype()
// deduction from whatever Adafruit_USBD_MIDI::read() happens to return.
#if __has_include(<class/midi/midi_device.h>)
using tinyusb_midi_packet_t = midi_packet_t;
#elif __has_include(<tusb_midi.h>)
using tinyusb_midi_packet_t = tud_midi_packet_t;
#else
#error "No TinyUSB MIDI packet type found; update MIDIUSB shim includes."
#endif
using midiEventPacket_t = tinyusb_midi_packet_t;
static_assert(sizeof(midiEventPacket_t) == 4,
              "USB MIDI packets must remain 4 bytes to expose the status byte");

namespace detail {
template <typename Midi, typename = void>
struct has_send_midi : std::false_type {};

template <typename Midi>
struct has_send_midi<Midi, std::void_t<decltype(
                               std::declval<Midi &>().send(
                                   std::declval<const midiEventPacket_t &>()))>>
    : std::true_type {};
} // namespace detail

class MIDIUSB_t {
public:
  // Mirror the MIDIUSB API surface that the NeoTrellis M4 library expects.
  // Each call forwards straight to the TinyUSB MIDI device so behavior stays
  // identical, just without the duplicate USB stack.
  midiEventPacket_t read(void) { return adaptPacket(usb_midi.read()); }

  void sendMIDI(const midiEventPacket_t &event) {
    // Adafruit TinyUSB Library v3.x exposes Adafruit_USBD_MIDI::send(packet).
    // This firmware targets that API. If a future TinyUSB drop renames the
    // transmit hook again, fail fast at compile time instead of silently
    // discarding outbound MIDI traffic.
    static_assert(
        detail::has_send_midi<Adafruit_USBD_MIDI>::value,
        "Adafruit_USBD_MIDI::send(midiEventPacket_t) missing; update the "
        "shim if TinyUSB changes its transmit API.");

    (void)usb_midi.send(event);
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


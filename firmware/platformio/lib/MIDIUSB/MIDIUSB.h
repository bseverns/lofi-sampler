#pragma once

// TinyUSB-first MIDIUSB compatibility shim.
// The stock Arduino MIDIUSB library fights with TinyUSB's USB stack, but
// Adafruit's TinyUSB implementation already exposes a rich MIDI interface.
// This header keeps legacy "MIDIUSB.h" includes happy while routing calls to
// the TinyUSB-backed Adafruit_USBD_MIDI instance you define in your sketch.

#include <Adafruit_TinyUSB.h>
#include <cstdint>
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

// TinyUSB exposes a canonical 4-byte USB MIDI packet, but the concrete type
// name has changed across releases. Prefer the return type of the TinyUSB MIDI
// device when it is a 4-byte packet; otherwise fall back to a minimal
// byte-for-byte struct so downstream code always has a concrete packet type.
struct midi_packet_fallback_t {
  uint8_t header;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
};

using raw_usb_midi_packet_t = decltype(std::declval<Adafruit_USBD_MIDI &>().read());

template <typename Packet, typename = void> struct midi_packet_or_fallback {
  using type = midi_packet_fallback_t;
};

template <typename Packet>
struct midi_packet_or_fallback<Packet, std::enable_if_t<sizeof(Packet) == 4>> {
  using type = Packet;
};

using midiEventPacket_t = midi_packet_or_fallback<raw_usb_midi_packet_t>::type;
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

template <typename Midi, typename = void>
struct has_send_midi_packet : std::false_type {};

template <typename Midi>
struct has_send_midi_packet<
    Midi, std::void_t<decltype(std::declval<Midi &>().sendMIDI(
        std::declval<const midiEventPacket_t &>()))>> : std::true_type {};

template <typename Midi, typename = void>
struct has_send_packet_ptr : std::false_type {};

template <typename Midi>
struct has_send_packet_ptr<
    Midi, std::void_t<decltype(std::declval<Midi &>().send(
        std::declval<const midiEventPacket_t *>()))>> : std::true_type {};

template <typename Midi, typename = void>
struct has_send_packet_call : std::false_type {};

template <typename Midi>
struct has_send_packet_call<
    Midi, std::void_t<decltype(std::declval<Midi &>().sendPacket(
        std::declval<const midiEventPacket_t &>()))>> : std::true_type {};

template <typename Midi, typename = void>
struct has_write_packet : std::false_type {};

template <typename Midi>
struct has_write_packet<Midi, std::void_t<decltype(
                               std::declval<Midi &>().write(
                                   reinterpret_cast<const uint8_t *>(
                                       std::declval<const midiEventPacket_t *>()),
                                   sizeof(midiEventPacket_t)))>> : std::true_type {};

template <typename Midi>
struct has_any_tx
    : std::integral_constant<
          bool, has_send_midi<Midi>::value ||
                    has_send_midi_packet<Midi>::value ||
                    has_send_packet_ptr<Midi>::value ||
                    has_send_packet_call<Midi>::value ||
                    has_write_packet<Midi>::value> {};

template <int I> struct priority_tag : priority_tag<I - 1> {};
template <> struct priority_tag<-1> {};
} // namespace detail

class MIDIUSB_t {
public:
  // Mirror the MIDIUSB API surface that the NeoTrellis M4 library expects.
  // Each call forwards straight to the TinyUSB MIDI device so behavior stays
  // identical, just without the duplicate USB stack.
  midiEventPacket_t read(void) { return adaptPacket(usb_midi.read()); }

  void sendMIDI(const midiEventPacket_t &event) {
    static_assert(detail::has_any_tx<Adafruit_USBD_MIDI>::value,
                  "Adafruit_USBD_MIDI needs a send-like routine; update the "
                  "shim to match the library's transmit API.");

    forwardSend(usb_midi, event, detail::priority_tag<4>{});
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
                          detail::priority_tag<4>)
      -> decltype(midi.sendMIDI(event), void()) {
    midi.sendMIDI(event);
  }

  template <typename Midi>
  static auto forwardSend(Midi &midi, const midiEventPacket_t &event,
                          detail::priority_tag<3>)
      -> decltype(midi.send(event), void()) {
    midi.send(event);
  }

  template <typename Midi>
  static auto forwardSend(Midi &midi, const midiEventPacket_t &event,
                          detail::priority_tag<2>)
      -> decltype(midi.send(&event), void()) {
    midi.send(&event);
  }

  template <typename Midi>
  static auto forwardSend(Midi &midi, const midiEventPacket_t &event,
                          detail::priority_tag<1>)
      -> decltype(midi.sendPacket(event), void()) {
    midi.sendPacket(event);
  }

  template <typename Midi>
  static auto forwardSend(Midi &midi, const midiEventPacket_t &event,
                          detail::priority_tag<0>)
      -> decltype(midi.write(reinterpret_cast<const uint8_t *>(&event),
                             sizeof(event)),
                  void()) {
    midi.write(reinterpret_cast<const uint8_t *>(&event), sizeof(event));
  }

  template <typename Midi>
  static void forwardSend(Midi &, const midiEventPacket_t &,
                          detail::priority_tag<-1>) {
#if defined(ARDUINO)
#warning "No Adafruit_USBD_MIDI send routine detected; MIDIUSB shim sendMIDI() will be a no-op."
#endif
  }
};

// The legacy header exposes a global MidiUSB instance. We keep that contract,
// but implement it inline so there is no separate translation unit to ship.
[[maybe_unused]] static MIDIUSB_t MidiUSB;


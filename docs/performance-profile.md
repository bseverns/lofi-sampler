# Performance Profile

Build the prepared-sample stage image with:

```sh
cd firmware/platformio
pio run -e adafruit_trellis_m4_performance
```

The legacy/studio image remains `adafruit_trellis_m4`; it retains live capture and storage-maintenance behavior.

## Stage contract

The performance image starts the 22.05 kHz audio timer without waiting for USB mount. It plays only the bundled, compiled slice pack through the existing known-good path. If storage cannot mount, boot fails rather than formatting or creating a new filesystem tree.

The performance image rejects record, restore/erase, and reslice storage actions; the corresponding modifier gestures are consumed and never fall through to gate edits. Its Storage mutation entry points fail closed, and all serial maintenance commands—including experimental factory restore—are disabled.

### Transport, panic, and clock loss

- **MIDI Stop:** immediately panics audio, cancels momentary stutter restoration, and latches transport stopped. Further clock messages do nothing; only Start or Continue resumes it.
- **Panic:** MIDI Stop or **Alt + Shift + step 8** clears queued commands, active voices, gain ramps, effects, and the self-test tone. It does not itself alter transport state.
- **Clock loss / USB disconnect:** after 500 ms without MIDI Clock, or when USB disconnects, the transport freezes. Buffered audio is allowed to drain. A subsequent clock reconnects from step 1; explicit Stop remains latched across clock messages.

## Gesture vocabulary: current performance contract

- **Tap:** edit a persistent gate, including physical columns 7 and 8.
- **Shift + any step:** direct slice retrigger. It never changes gate or velocity and uses one atomic audio trigger command.
- **Alt + lit step:** cycle probability.
- **Alt + Shift + steps 1–4:** filter, bitcrush, drive, and clear FX.
- **Alt + Shift + step 8:** immediate physical panic.
- Disabled recording, restore/erase, and reslice combinations are inert and consume the press.

Row mute/solo, pattern rotation, initial-pattern restoration, and persistent velocity editing remain proposed rather than implemented.

## Software evidence and physical boundary

Host tests cover Stop/Continue/clock-loss state transitions, performance gate-fallback prevention, direct retrigger routing, and the panic gesture. Both PlatformIO environments compile in CI and locally. This does **not** verify DAC behavior, queue timing, USB disconnect behavior, external flash behavior, LED contention, or audible headroom on a board.

### Required hardware matrix before performance use

| Check | Observable pass condition |
| --- | --- |
| Cold boot | Performance boot indication and all 32 bundled slices play without USB attached. |
| MIDI Stop | Current sound cuts immediately; continuing clock does not restart transport. |
| Panic gesture | Alt + Shift + step 8 cuts sound; the next retrigger plays only the newly pressed slice. |
| Clock loss/reconnect | Clock removal freezes steps; reconnect begins on step 1. |
| Read-only boundary | Record, restore, and reslice gestures leave the prepared set byte-for-byte unchanged. |

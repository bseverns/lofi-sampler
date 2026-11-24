# Performance Profile

Build the prepared-sample stage image with:

```sh
cd firmware/platformio
pio run -e adafruit_trellis_m4_performance
```

The legacy/studio image remains `adafruit_trellis_m4`; it retains live capture and storage-maintenance behavior.

## Stage contract

The performance image starts the 22.05 kHz audio timer without waiting for USB mount. It plays only the bundled, compiled slice pack through the existing known-good path. If storage cannot mount, boot fails rather than formatting or creating a new filesystem tree.

The performance image rejects record, restore/erase, and reslice pad gestures. It also disables all serial maintenance commands, including experimental factory restore. MIDI Stop invokes immediate audio panic and latches transport stopped; later clock messages do not restart it. Only MIDI Start or Continue resumes transport.

## Gesture vocabulary: first-stage contract

- **Tap:** edit a persistent gate, including physical columns 7 and 8.
- **Shift + lit step:** cycle velocity and momentarily retrigger that slice. This legacy coupling is preserved in the studio profile only; do not treat it as a prepared-performance audition design.
- **Alt + lit step:** cycle probability.
- **Alt + Shift + steps 1–4:** momentary effect macros / clear FX.
- **MIDI Stop:** immediate panic and stopped transport state.

Row mute/solo, pattern rotation, initial-pattern restoration, and a dedicated pad panic gesture are deliberately not assigned yet: they need an unambiguous visible-state design before touching the live surface.

## Verification boundaries

Host tests prove transport Stop/Continue behavior, atomic trigger queue structure, and that destructive pad gestures do not reach the recording controller when compiled with `NTM4_PERFORMANCE_BUILD`. They do not prove timing, audio headroom, LED contention, or hardware storage behavior; bench-test those on a NeoTrellis M4 before performance use.

# AudioEngine Flow

The AudioEngine is intentionally split into two worlds that should not bleed into each other.

- **Foreground `service()` path:** lives in [`firmware/platformio/src/AudioEngine.cpp`](../firmware/platformio/src/AudioEngine.cpp). It drains the job queue, primes buffers, advances gain ramps, and handles storage-facing work.
- **Timer ISR path:** also in `AudioEngine.cpp`. It mixes already-primed samples and writes the DAC at 22.05 kHz.

## Design Rule
Anything slow, stateful, or storage-heavy belongs in the foreground path. The ISR gets deterministic math only.

## Main Components
- `preloadAndPlay()`: queues a slice preload for a voice.
- `setLevel()` and `stopVoice()`: queue or arm gain changes.
- `triggerFilterSweep()`, `triggerBitcrush()`, `triggerDrive()`, `clearFx()`: build tables in the foreground so the ISR only reads current values.
- `pumpStreams()`: keeps per-voice buffers primed from storage.
- `pumpGains()`: advances ramps.
- `isr()`: mixes ready samples and writes the DAC.

## Why This Matters
The Trellis scan, USB stack, storage reads, and combo routing all live outside the ISR. That separation is why the board can keep playing while the rest of the firmware is busy.

## Current Reality
- The timer path is wired through `TC3_Handler()`.
- DAC writes are direct in the real-time path rather than repeated `analogWrite()` calls.
- The idle diagnostic spam that used to flood Serial was removed; diagnostics are no longer auto-enqueued every time an idle voice is cleaned up.

## Entry Points
- Foreground loop hook: [`main.cpp`](../firmware/platformio/src/main.cpp)
- Audio engine: [`AudioEngine.cpp`](../firmware/platformio/src/AudioEngine.cpp)
- Storage interface: [`Storage.cpp`](../firmware/platformio/src/Storage.cpp)
- Step playback routing: [`StepPlaybackController.cpp`](../firmware/platformio/src/StepPlaybackController.cpp)

For transport math and subsystem timing, see [`workflow.md`](workflow.md).

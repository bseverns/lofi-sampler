# AudioEngine Flow (job queue → RAM buffers → ISR mix)

Think of the AudioEngine as two co-conspirators who never step on each other’s cables:

- **`service()` (foreground, main loop):** lives in `firmware/arduino/lofi_sampler/AudioEngine.cpp`. It drains the tiny job queue, preloads slices from flash into per-voice RAM rings, and nudges gain ramps forward. All slow I/O and housekeeping happens here.
- **`isr()` (22.05 kHz timer interrupt):** also in `AudioEngine.cpp`. It only mixes already-ready samples with already-baked gain ramps and pushes them to the DAC. No filesystem calls, no Serial, no allocations—just deterministic math.

The job queue keeps those worlds synchronized: anything that needs flash, fades, or diagnostics becomes a job so the ISR can remain aggressively boring.

## Quick diagram
```
[ loop() ]
    |
    |  enqueue jobs via preloadAndPlay(), setLevel(), stopVoice(), requestDiagnostics()
    v
[ job queue (AudioEngine::Job) ]
    |
    |  service(): handleJob() -> handlePreload()/handleFade()/handleDiagnostics()
    |  service(): pumpStreams() slurps flash into vbuf[voice][]
    |  service(): pumpGains() advances gain ramps
    v
[ per-voice RAM ring buffers (vbuf) ]
    |
    |  isr(): reads vavailable/vpos, mixes int16 samples, applies vgainCurrent
    v
[ DAC write at 22.05 kHz ]
```

## How preloads and fades move
1. **Queue the work:**
   - `preloadAndPlay(voice, path)` pushes a `JobType::Preload` with the slice path (e.g., `/A/A1.raw`).
   - `setLevel(voice, level)` or `stopVoice(voice)` push `JobType::Fade` entries. If the queue is full, they arm the ramp directly as a fallback.
   - Live FX mashes (`triggerFilterSweep()`, `triggerBitcrush()`, `triggerDrive()`, `clearFx()`) push `JobType::FilterSweep`/`Bitcrush`/`Drive`/`FxClear`, which only ever precompute lookup tables for the ISR to sip.
2. **Drain in `service()`:** the main loop calls `AudioEngine::service()`, which pops jobs and routes them to `handlePreload()`, `handleFade()`, or the FX handlers.
   - `handlePreload()` resets the voice state, asks `Storage` for sample counts, marks the voice as streaming, and immediately calls `pumpStreams()` to seed the buffer so playback can start next ISR tick.
   - `handleFade()` arms gain ramps via `armGainRamp()`; a zero target also marks the voice for draining.
   - FX handlers build sine ramps, drive swells, or bitcrush hold tables. `pumpEffects()` walks those tables once per frame so the ISR only multiplies/bitmasks with precomputed values.
3. **Stream in the background:** `pumpStreams()` pulls chunks from flash with `Storage::readRawChunk()` into `vbuf` in circular fashion until `voiceLoadedSamples` catches `voiceTotalSamples`. On the first successful fill, it flags `voicePrimed`/`voiceActive` and triggers a fade-in ramp.
4. **Fade arithmetic:** `pumpGains()` advances per-voice `vgainCurrent` toward `vgainTarget` using precomputed steps so the ISR doesn’t think about envelopes.

## What the ISR does (and won’t do)
`AudioEngine::isr()` wakes at 22,050 Hz and loops over four voices. For any `voicePrimed` with `vavailable > 0`, it grabs the next sample from `vbuf`, multiplies by `vgainCurrent`, sums, clamps, and writes both DAC pins. When a buffer drains and streaming has finished, it marks the voice inactive. Anything slower than a multiply stays out of the ISR; add a job instead and let `service()` babysit it.

## Entry points for spelunking
- Foreground loop hook: `AudioEngine::service()` in `firmware/platformio/src/AudioEngine.cpp` (called from `loop()` in `main.cpp`).
- Job producers: `preloadAndPlay()`, `setLevel()`, `stopVoice()`, `triggerFilterSweep()/triggerBitcrush()/triggerDrive()/clearFx()`, and `requestDiagnostics()` in `AudioEngine.h`/`.cpp`—these enqueue `Job` structs consumed by `service()`.
- Job consumers and buffer plumbers: `handlePreload()`, `handleFade()`, `handleDiagnostics()`, the FX handlers, plus `pumpStreams()`/`pumpGains()`/`pumpEffects()` in `AudioEngine.cpp`.
- ISR mixdown: `AudioEngine::isr()` and the timer hook `onTimerISR()` in `AudioEngine.cpp`—the only code that actually touches the DAC.

Treat this as a studio notebook meets safety manual: queue the heavy work, let `service()` shovel bytes, and keep the interrupt path monk-like and predictable.

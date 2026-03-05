# Lo-Fi Sampler Roadmap

This roadmap tracks larger improvements that go beyond small bug fixes. It is
organized to preserve one hard rule: the 22.05 kHz ISR stays deterministic.

## 1) Modular firmware architecture

### Why
`firmware/platformio/src/main.cpp` is currently 470 lines and mixes UI scanning,
combo routing, record flow, storage calls, and effect triggers.

### Milestones
1. Extract a `PadActionRouter` module for combo-to-action dispatch.
2. Extract a `RecordingController` module for record/stop/overdub/reslice/undo.
3. Keep `main.cpp` as wiring + top-level `setup()`/`loop()` orchestration only.
4. Add host-side tests for routing logic (no hardware required).

### Guardrails
- No dynamic allocation in audio-critical paths.
- Keep ISR entry points unchanged while refactoring.

## 2) Storage abstraction beyond raw PCM

### Why
`.raw` is fast and deterministic, but compressed assets could improve capacity.

### Milestones
1. Introduce a `SampleSource` abstraction (`RawSource`, future `CompressedSource`).
2. Add optional offline decode/transcode in tooling (`tools/`) first.
3. If on-device decode is added, decode in foreground jobs only (never ISR).
4. Benchmark worst-case step preload latency against MIDI clock deadlines.

### Guardrails
- ISR continues to read ready-to-mix PCM buffers only.
- Compression remains opt-in until timing/CPU margins are proven.

## 3) Expanded timing modes

### Why
Current design is fixed at 8 steps and 4 voices. Optional modes could expand
musical range.

### Candidate features
1. Variable slice counts per row (8/12/16) with clear LED state mapping.
2. Free-running row mode that ignores global step advances.
3. Per-row clock division/multiplication while preserving deterministic scheduling.

### Guardrails
- Keep existing 8-step global mode as default and compatibility baseline.
- Any new mode must specify exact transport behavior for Start/Stop/Continue.

## 4) Documentation discoverability

### Why
Core docs exist, but users should not need to hunt through references.

### Milestones
1. Maintain a "Documentation map" in `README.md`.
2. Ensure demo docs link both directions (`demo-exercises` <-> `demo-script`).
3. Link workflow and audio architecture from feature descriptions.

## 5) Hardware accessibility

### Why
Build assumes a NeoTrellis M4 and line-in bias circuit knowledge.

### Milestones
1. Publish a parts list + shopping links for a "minimum working rig."
2. Add optional PCB/perfboard files for the analog front-end.
3. Document calibration checks (bias midpoint, clipping test, noise floor).

---

If you want to execute this in pull requests, start with section 1 (modular
firmware extraction) because it lowers risk for sections 2 and 3.


#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "MIDIUSB.h"
#include "Config.h"
#include "AudioEngine.h"
#include "Storage.h"
#include "Slicer.h"
#include "RecorderADC.h"
#include "TrellisUI.h"
#include "PadInput.h"

// ---------- Globals ----------
Adafruit_USBD_MIDI usb_midi;
AudioEngine audio;
Storage storage;
RecorderADC rec;
TrellisUI ui;
ManifestCheck manifestStatus;

volatile bool playing = false;      // latched by MIDI Start/Stop/Continue
volatile uint8_t stepIndex = 0;     // which of the 8 columns is hot
volatile uint16_t midiClockCount = 0; // counts 24 PPQN clock ticks until the next step

bool gates[4][8] = {{0}}; // rows A..D
ModifierTracker modifierTracker;
static uint8_t recordingRow = 255; // which row owns the live capture
static bool recordHoldCandidate = false; // true when a long-press overdub could stop on release
static uint32_t recordStartMillis = 0;

enum class RecordMode : uint8_t {
  Replace = 0,
  Overdub
};

static constexpr uint16_t OVERDUB_HOLD_MS = 250;

static const float DEFAULT_VOICE_LEVEL = 0.9f;
static uint32_t stutterReleaseAt[4] = {0,0,0,0};

// ---------- Helpers ----------
[[maybe_unused]] static const char* rowPath(char row) {
  switch(row) {
    case 'A': return PATH_A;
    case 'B': return PATH_B;
    case 'C': return PATH_C;
    case 'D': return PATH_D;
  }
  return PATH_A;
}

static void playStep() {
  const char rowL[4] = {'A','B','C','D'};
  for (uint8_t r=0; r<4; r++) {
    if (gates[r][stepIndex]) {
      char path[16];
      snprintf(path, sizeof(path), "/%c/%c%d.raw", rowL[r], rowL[r], stepIndex+1);
      audio.preloadAndPlay(r, path);
    } else {
      audio.stopVoice(r);
    }
  }
}

static bool resliceRow(uint8_t row) {
  if (row >= 4) return false;
  int16_t* scratch = rec.mutableData();
  if (!scratch) return false;
  char rowL = "ABCD"[row];
  char src[16];
  snprintf(src, sizeof(src), "/%c/source.raw", rowL);
  int32_t count = storage.readRawInto(src, scratch, MAX_RECORD_SAMPLES);
  if (count <= 0) {
    // Nothing to reslice? Fall back to the previous take if one exists.
    if (!storage.swapInPreviousSource(rowL)) {
      return false;
    }
    count = storage.readRawInto(src, scratch, MAX_RECORD_SAMPLES);
  }
  if (count <= 0) {
    return false;
  }
  return Slicer::writeEight(&rowL, scratch, (uint32_t)count, false);
}

static bool overdubAndSlice(uint8_t row, uint32_t newSamples) {
  if (row >= 4) return false;
  char rowL = "ABCD"[row];
  char src[16];
  snprintf(src, sizeof(src), "/%c/source.raw", rowL);

  int32_t existingCount = storage.rawSampleCount(src);
  if (existingCount <= 0) {
    return Slicer::writeEight(&rowL, rec.data(), newSamples);
  }

  uint32_t total = (uint32_t)existingCount;
  if (newSamples > total) total = newSamples;
  if (total > MAX_RECORD_SAMPLES) {
    total = MAX_RECORD_SAMPLES; // respect the RAM budget table
  }

  // Mix the existing take into the capture buffer in-place.
  int16_t* mix = rec.mutableData();
  static const uint16_t CHUNK = 256;
  int16_t base[CHUNK];
  uint32_t offset = 0;
  while (offset < total) {
    uint32_t want = total - offset;
    if (want > CHUNK) want = CHUNK;
    int32_t got = storage.readRawChunk(src, offset, base, want);
    uint32_t baseCount = got > 0 ? (uint32_t)got : 0;
    for (uint32_t i = 0; i < want; ++i) {
      int32_t cur = (i < baseCount) ? base[i] : 0;
      int32_t incoming = (offset + i < newSamples) ? mix[offset + i] : 0;
      int32_t sum = cur + incoming;
      if (sum > 32767) sum = 32767;
      if (sum < -32768) sum = -32768;
      mix[offset + i] = (int16_t)sum;
    }
    offset += want;
  }
  return Slicer::writeEight(&rowL, mix, total);
}

static bool commitRecording(uint8_t row, uint32_t samples, RecordMode mode) {
  if (samples == 0 || row >= 4) {
    recordingRow = 255;
    recordHoldCandidate = false;
    return false;
  }
  bool ok = false;
  if (mode == RecordMode::Overdub) {
    ok = overdubAndSlice(row, samples);
  }
  if (mode == RecordMode::Replace || !ok) {
    char rowL = "ABCD"[row];
    ok = Slicer::writeEight(&rowL, rec.data(), samples);
  }
  recordingRow = 255;
  recordHoldCandidate = false;
  return ok;
}

static void serviceStutterDecay() {
  uint32_t now = millis();
  for (uint8_t r = 0; r < 4; ++r) {
    uint32_t expire = stutterReleaseAt[r];
    if (expire && (int32_t)(now - expire) >= 0) {
      audio.setLevel(r, DEFAULT_VOICE_LEVEL);
      stutterReleaseAt[r] = 0;
    }
  }
}

static void logManifest(const ManifestCheck& status) {
  if (status.ok) {
    Serial.print(F("[manifest] OK: "));
  } else {
    Serial.print(F("[manifest] WARN: "));
  }
  Serial.println(status.message);
}

static void handleFactoryResetCommand() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'f' || c == 'F') {
      Serial.println(F("Factory demo reset requested"));
      playing = false;
      audio.stop();
      auto restore = storage.restoreFactoryDemo();
      Serial.println(restore.message);
      manifestStatus = storage.checkManifest();
      logManifest(manifestStatus);
      audio.start();
    }
  }
}

// ---------- Combo actions ----------

// SHIFT+ALT combo: reload the saved source + carve new slices without touching gates.
static PadActionResult actionReslice(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || !mods.shift) return PadActionResult::NoMatch;
  if (col != (STEPS_PER_BAR - 3)) return PadActionResult::NoMatch; // map to the last "normal" step pad
  if (resliceRow(row)) {
    return PadActionResult::MatchedStop;
  }
  return PadActionResult::MatchedStop;
}

// SHIFT+ALT+step 1..4: performance FX (pre-baked lookup tables in AudioEngine)
static PadActionResult actionFx(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || !mods.shift) return PadActionResult::NoMatch;
  if (col >= COL_ALT) return PadActionResult::NoMatch; // ignore modifier columns

  switch (col) {
    case 0:
      audio.triggerFilterSweep(row);
      return PadActionResult::MatchedStop;
    case 1:
      audio.triggerBitcrush(row);
      return PadActionResult::MatchedStop;
    case 2:
      audio.triggerDrive(row);
      return PadActionResult::MatchedStop;
    case 3:
      audio.clearFx(row);
      return PadActionResult::MatchedStop;
    default:
      return PadActionResult::NoMatch;
  }
}

// SHIFT combo riff: momentary "manual retrigger" that leans on whatever gate is already live.
static PadActionResult actionStutter(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.shift || mods.alt) return PadActionResult::NoMatch;
  if (col >= STEPS_PER_BAR) return PadActionResult::NoMatch;
  if (!gates[row][col]) return PadActionResult::NoMatch; // treat stutter as "riff on an active gate"
  char rowL = "ABCD"[row];
  char path[16];
  snprintf(path, sizeof(path), "/%c/%c%d.raw", rowL, rowL, col + 1);
  float velocity = 0.35f + (0.08f * col);
  if (velocity > 1.0f) velocity = 1.0f;
  audio.setLevel(row, velocity);
  if (audio.preloadAndPlay(row, path)) {
    stutterReleaseAt[row] = millis() + 160;
    return PadActionResult::MatchedStop;
  }
  audio.setLevel(row, DEFAULT_VOICE_LEVEL);
  stutterReleaseAt[row] = 0;
  return PadActionResult::MatchedStop;
}

static PadActionResult actionRecord(uint8_t row, uint8_t col, const PadModifiers& mods) {
  (void)col;
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.shift || mods.alt) return PadActionResult::NoMatch;
  // SHIFT press on an "empty" step still arms recording; stutter handlers bail
  // early when they detect an unlit gate, so we get the classic hold-Shift-then-pad flow.
  if (!rec.isRecording()) {
    recordingRow = row;
    recordHoldCandidate = true;
    recordStartMillis = millis();
    rec.start();
  } else if (recordingRow == row) {
    uint32_t n = rec.stop();
    recordHoldCandidate = false;
    commitRecording(row, n, RecordMode::Replace);
  }
  return PadActionResult::MatchedStop;
}

static PadActionResult actionErase(uint8_t row, uint8_t col, const PadModifiers& mods) {
  (void)col;
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || mods.shift) return PadActionResult::NoMatch;
  char rowL = "ABCD"[row];
  if (storage.swapInPreviousSource(rowL)) {
    resliceRow(row);
  } else {
    for (uint8_t i=0;i<8;i++) {
      char path[16]; snprintf(path,sizeof(path),"/%c/%c%d.raw",rowL,rowL,i+1);
      storage.remove(path);
    }
    char src[16]; snprintf(src,sizeof(src),"/%c/source.raw",rowL);
    char prev[20]; snprintf(prev, sizeof(prev), "/%c/source_prev.raw", rowL);
    storage.remove(src);
    storage.remove(prev);
  }
  return PadActionResult::MatchedStop;
}

// ---------- MIDI parsing ----------
// USB TinyUSB surfaces raw USB-MIDI packets. We only care about realtime 0xF8..0xFC commands.
void handleMidi() {
  while (usb_midi.available()) {
    midiEventPacket_t packet = MidiUSB.read();
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&packet);
    uint8_t b0 = bytes[1];
    // Realtime messages can appear anywhere
    if (b0 == 0xF8) { // Timing Clock (24 PPQN)
      if (playing) {
        midiClockCount++;
        if (midiClockCount >= CLOCKS_PER_STEP) {
          midiClockCount = 0;
          stepIndex = (stepIndex + 1) % STEPS_PER_BAR;
          playStep();
        }
      }
    } else if (b0 == 0xFA) { // Start (rewind to step 0 on next clock)
      playing = true;
      midiClockCount = 0;
      stepIndex = STEPS_PER_BAR - 1; // so first clock advance goes to step 0
    } else if (b0 == 0xFB) { // Continue (keep current step)
      playing = true;
    } else if (b0 == 0xFC) { // Stop (freeze UI animation but keep last gates)
      playing = false;
    }
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(F("NTM4 Sampler boot"));

  usb_midi.setStringDescriptor("NTM4 Sampler");
  usb_midi.begin();

  if (!storage.begin()) {
    Serial.println(F("Storage init failed; LittleFS unavailable"));
    while (1) { delay(10); }
  }
  manifestStatus = storage.checkManifest();
  logManifest(manifestStatus);
  ui.begin();
  audio.begin();
  audio.attachStorage(&storage);
  rec.begin();

  modifierTracker.reset();
  resetPadActionRegistry();
  registerPadAction(actionFx);
  registerPadAction(actionReslice);
  registerPadAction(actionStutter);
  registerPadAction(actionRecord);
  registerPadAction(actionErase);

  audio.start();
}

// ---------- Loop ----------
void loop() {
  handleMidi();
  handleFactoryResetCommand();

  // UI input
  int32_t ev = ui.pollEvent();
  if (ev != -1) {
    uint8_t r = (ev >> 8) & 0xFF;
    uint8_t c = ev & 0xFF;
    bool pressed = (ev & 0x8000);
    if (pressed) {
      if (!modifierTracker.handlePress(r, c)) {
        PadModifiers mods = modifierTracker.modifiersFor(r);
        bool consumed = handlePadCombo(r, c, mods);
        if (!consumed) {
          gates[r][c] = !gates[r][c];
          ui.setGate(r,c,gates[r][c]);
        }
      }
    } else {
      // Overdub gesture: Shift + hold a row, then release to commit the mix.
      if (rec.isRecording() && recordHoldCandidate && recordingRow == r && c < COL_ALT) {
        PadModifiers mods = modifierTracker.modifiersFor(r);
        if (mods.shift && (millis() - recordStartMillis) >= OVERDUB_HOLD_MS) {
          uint32_t n = rec.stop();
          recordHoldCandidate = false;
          commitRecording(r, n, RecordMode::Overdub);
        }
      }
      modifierTracker.handleRelease(r, c);
    }
  }

  // Service recorder during record
  if (rec.isRecording()) {
    rec.service();
  }

  serviceStutterDecay();

  audio.service();
  ui.draw(playing ? stepIndex : 255, rec.isRecording() ? 0 : -1);
}

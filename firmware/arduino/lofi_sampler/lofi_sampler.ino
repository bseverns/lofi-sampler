
#include <Adafruit_TinyUSB.h>
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

volatile bool playing = false;
volatile uint8_t stepIndex = 0;
volatile uint16_t midiClockCount = 0;

bool gates[4][8] = {{0}}; // rows A..D
ModifierTracker modifierTracker;

static const float DEFAULT_VOICE_LEVEL = 0.9f;
static uint32_t stutterReleaseAt[4] = {0,0,0,0};

// ---------- Helpers ----------
static const char* rowPath(char row) {
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
    return false;
  }
  return Slicer::writeEight(&rowL, scratch, (uint32_t)count);
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

// ---------- Combo actions ----------

// SHIFT+ALT combo: reload the saved source + carve new slices without touching gates.
static PadActionResult actionReslice(uint8_t row, uint8_t col, const PadModifiers& mods) {
  (void)col;
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || !mods.shift) return PadActionResult::NoMatch;
  if (resliceRow(row)) {
    return PadActionResult::MatchedStop;
  }
  return PadActionResult::MatchedStop;
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
    rec.start();
  } else {
    uint32_t n = rec.stop();
    if (n > 0) {
      char rowL = "ABCD"[row];
      Slicer::writeEight(&rowL, rec.data(), n);
    }
  }
  return PadActionResult::MatchedStop;
}

static PadActionResult actionErase(uint8_t row, uint8_t col, const PadModifiers& mods) {
  (void)col;
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || mods.shift) return PadActionResult::NoMatch;
  char rowL = "ABCD"[row];
  for (uint8_t i=0;i<8;i++) {
    char path[16]; snprintf(path,sizeof(path),"/%c/%c%d.raw",rowL,rowL,i+1);
    storage.remove(path);
  }
  char src[16]; snprintf(src,sizeof(src),"/%c/source.raw",rowL);
  storage.remove(src);
  return PadActionResult::MatchedStop;
}

// ---------- MIDI parsing ----------
void handleMidi() {
  uint8_t packet[4];
  while (usb_midi.available()) {
    usb_midi.read(packet);
    uint8_t b0 = packet[1];
    // Realtime messages can appear anywhere
    if (b0 == 0xF8) { // Timing Clock
      if (playing) {
        midiClockCount++;
        if (midiClockCount >= CLOCKS_PER_STEP) {
          midiClockCount = 0;
          stepIndex = (stepIndex + 1) % STEPS_PER_BAR;
          playStep();
        }
      }
    } else if (b0 == 0xFA) { // Start
      playing = true;
      midiClockCount = 0;
      stepIndex = STEPS_PER_BAR - 1; // so first clock advance goes to step 0
    } else if (b0 == 0xFB) { // Continue
      playing = true;
    } else if (b0 == 0xFC) { // Stop
      playing = false;
    }
  }
}

// ---------- Setup ----------
void setup() {
  usb_midi.setStringDescriptor("NTM4 Sampler");
  usb_midi.begin();

  storage.begin();
  ui.begin();
  audio.begin();
  audio.attachStorage(&storage);
  rec.begin();

  modifierTracker.reset();
  resetPadActionRegistry();
  registerPadAction(actionReslice);
  registerPadAction(actionStutter);
  registerPadAction(actionRecord);
  registerPadAction(actionErase);

  audio.start();
}

// ---------- Loop ----------
void loop() {
  handleMidi();

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

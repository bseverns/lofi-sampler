
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "MIDIUSB.h"
#include "ClockTransport.h"
#include "Config.h"
#include "AudioEngine.h"
#include "PadActionRouter.h"
#include "RecordingController.h"
#include "StepPlaybackController.h"
#include "Storage.h"
#include "RecorderADC.h"
#include "TrellisUI.h"
#include "PadInput.h"

#ifndef NTM4_LOOP_DIAG_STAGE
#define NTM4_LOOP_DIAG_STAGE 4
#endif

#ifndef NTM4_SETUP_DIAG_STAGE
#define NTM4_SETUP_DIAG_STAGE 0
#endif

// ---------- Globals ----------
Adafruit_USBD_MIDI usb_midi;
AudioEngine audio;
Storage storage;
RecorderADC rec;
TrellisUI ui;
ManifestCheck manifestStatus;
static const float DEFAULT_VOICE_LEVEL = 0.9f;
static bool audioStarted = false;
static uint32_t usbMountedAtMs = 0;
static bool uiDirty = true;
static uint8_t lastDrawStep = 255;
static int lastDrawRecRow = -2;
static PadModifiers lastDrawModifiers[4] = {
  {false, false},
  {false, false},
  {false, false},
  {false, false},
};

StepState gates[4][8] = {{{0}}}; // rows A..D
ModifierTracker modifierTracker;
ClockTransport clockTransport(STEPS_PER_BAR, CLOCKS_PER_STEP, GLOBAL_SWING_AMOUNT);
RecordingController recordingController(storage, rec);
PadActionRouter padRouter(audio, ui, gates, recordingController, DEFAULT_VOICE_LEVEL);
StepPlaybackController stepPlayback(audio, gates, DEFAULT_VOICE_LEVEL);

static StepState defaultStepState() {
  StepState s = {false, STEP_DEFAULT_VELOCITY, STEP_DEFAULT_PROBABILITY};
  return s;
}

static void toggleStepGate(uint8_t row, uint8_t col) {
  StepState step = gates[row][col];
  if (!step.gate) {
    if (step.velocity == 0) step.velocity = STEP_DEFAULT_VELOCITY;
    if (step.probability == 0) step.probability = STEP_DEFAULT_PROBABILITY;
    step.gate = true;
  } else {
    step.gate = false;
  }
  gates[row][col] = step;
  ui.setStep(row, col, step);
}

// ---------- Helpers ----------
static void logManifest(const ManifestCheck& status) {
  if (status.ok) {
    Serial.print(F("[manifest] OK: "));
  } else {
    Serial.print(F("[manifest] WARN: "));
  }
  Serial.println(status.message);
}

static void handleSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'f' || c == 'F') {
      Serial.println(F("Factory demo reset requested"));
      clockTransport.stop();
      audio.stop();
      auto restore = storage.restoreFactoryDemo();
      Serial.println(restore.message);
      manifestStatus = storage.checkManifest();
      logManifest(manifestStatus);
      audio.start();
    } else if (c == 't' || c == 'T') {
      Serial.println(F("DAC self-test tone"));
      audio.playSelfTestTone();
    }
  }
}

// ---------- MIDI parsing ----------
// USB TinyUSB surfaces raw USB-MIDI packets. We only care about realtime 0xF8..0xFC commands.
void handleMidi() {
  static uint32_t clockLogCount = 0;
  uint8_t packet[4];
  while (usb_midi.readPacket(packet)) {
    uint8_t b0 = packet[1];
    if (b0 == 0xFA) {
      Serial.println(F("[midi] start"));
    } else if (b0 == 0xFB) {
      Serial.println(F("[midi] continue"));
    } else if (b0 == 0xFC) {
      Serial.println(F("[midi] stop"));
    } else if (b0 == 0xF8) {
      ++clockLogCount;
      if ((clockLogCount % 24u) == 0u) {
        Serial.print(F("[midi] clock x"));
        Serial.println(clockLogCount);
      }
    }
    if (clockTransport.handleRealtime(b0)) {
      Serial.print(F("[step] "));
      Serial.println(clockTransport.currentStep());
      stepPlayback.playStep(clockTransport.currentStep());
    }
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(F("NTM4 Sampler boot"));

#if NTM4_SETUP_DIAG_STAGE > 0
  ui.begin();
  Serial.print(F("Setup diagnostic stage "));
  Serial.println(NTM4_SETUP_DIAG_STAGE);

#if NTM4_SETUP_DIAG_STAGE >= 1
  usb_midi.setStringDescriptor("NTM4 Sampler");
  usb_midi.begin();
#endif

#if NTM4_SETUP_DIAG_STAGE >= 2
  if (!storage.begin()) {
    Serial.println(F("Storage init failed; LittleFS unavailable"));
    while (1) { delay(10); }
  }
  manifestStatus = storage.checkManifest();
  logManifest(manifestStatus);
#endif

#if NTM4_SETUP_DIAG_STAGE >= 3
  audio.begin();
  audio.attachStorage(&storage);
#endif

#if NTM4_SETUP_DIAG_STAGE >= 4
  rec.begin();
#endif

#if NTM4_SETUP_DIAG_STAGE >= 5
  if (TinyUSBDevice.mounted()) {
    audio.start();
  }
#endif
  return;
#endif

  usb_midi.setStringDescriptor("NTM4 Sampler");
  usb_midi.begin();

  if (!storage.begin()) {
    Serial.println(F("Storage init failed; LittleFS unavailable"));
    while (1) { delay(10); }
  }
  for (uint8_t r = 0; r < 4; ++r) {
    for (uint8_t c = 0; c < STEPS_PER_BAR; ++c) {
      gates[r][c] = defaultStepState();
    }
  }
  manifestStatus = storage.checkManifest();
  logManifest(manifestStatus);
  ui.begin();
  audio.begin();
  audio.attachStorage(&storage);
  rec.begin();

  modifierTracker.reset();

  audioStarted = false;
  usbMountedAtMs = 0;
  uiDirty = true;
  lastDrawStep = 255;
  lastDrawRecRow = -2;
  for (uint8_t r = 0; r < 4; ++r) {
    lastDrawModifiers[r] = {false, false};
  }
  Serial.println(F("Serial commands: t=self-test tone, f=factory demo restore"));
}

// ---------- Loop ----------
void loop() {
#if NTM4_SETUP_DIAG_STAGE > 0
  int32_t ev = ui.pollEvent();
  if (ev != -1) {
    uint8_t r = (ev >> 8) & 0xFF;
    uint8_t c = ev & 0xFF;
    bool pressed = (ev & 0x10000);
    Serial.print(F("[setup] "));
    Serial.print(pressed ? F("down") : F("up"));
    Serial.print(F(" r="));
    Serial.print(r);
    Serial.print(F(" c="));
    Serial.println(c);
  }
  delay(5);
  return;
#else

  if (!audioStarted && TinyUSBDevice.mounted()) {
    if (usbMountedAtMs == 0) {
      usbMountedAtMs = millis();
    } else if (millis() - usbMountedAtMs >= 250) {
      audio.start();
      audioStarted = true;
      Serial.println(F("Audio engine started"));
    }
  }

#if NTM4_LOOP_DIAG_STAGE < 1
  return;
#endif

  handleMidi();
  handleSerialCommands();

#if NTM4_LOOP_DIAG_STAGE < 2
  return;
#endif

  // UI input
  int32_t ev = ui.pollEvent();
  if (ev != -1) {
    uint8_t r = (ev >> 8) & 0xFF;
    uint8_t c = ev & 0xFF;
    bool pressed = (ev & 0x10000);
    if (pressed) {
      if (!modifierTracker.handlePress(r, c)) {
        PadModifiers mods = modifierTracker.modifiersFor(r);
        modifierTracker.noteModifierUse(r, mods);
        bool consumed = padRouter.handlePress(r, c, mods);
        if (!consumed) {
          toggleStepGate(r, c);
        }
        uiDirty = true;
      } else {
        uiDirty = true;
      }
    } else {
      PadModifiers mods = modifierTracker.modifiersFor(r);
      ModifierReleaseResult releaseResult = modifierTracker.handleRelease(r, c);
      if (releaseResult == ModifierReleaseResult::NotModifier) {
        recordingController.maybeCommitOverdubOnRelease(r, c, mods);
      } else if (releaseResult == ModifierReleaseResult::TapAsStep) {
        toggleStepGate(r, c);
      }
      uiDirty = true;
    }
  }

#if NTM4_LOOP_DIAG_STAGE < 3
  return;
#endif

  // Service recorder during record
  if (rec.isRecording()) {
    rec.service();
  }

  padRouter.serviceStutterDecay();

#if NTM4_LOOP_DIAG_STAGE < 4
  return;
#endif

  audio.service();
  uint8_t drawStep = clockTransport.isPlaying() ? clockTransport.currentStep() : 255;
  int recRow = recordingController.isRecording() ? (int)recordingController.activeRow() : -1;
  if (drawStep != lastDrawStep || recRow != lastDrawRecRow) {
    uiDirty = true;
  }

  for (uint8_t r = 0; r < 4; ++r) {
    PadModifiers mods = modifierTracker.modifiersFor(r);
    if (mods.alt != lastDrawModifiers[r].alt || mods.shift != lastDrawModifiers[r].shift) {
      uiDirty = true;
    }
    ui.setModifiers(r, mods);
  }

  if (!uiDirty) {
    return;
  }

  ui.draw(drawStep, recRow);
  lastDrawStep = drawStep;
  lastDrawRecRow = recRow;
  for (uint8_t r = 0; r < 4; ++r) {
    lastDrawModifiers[r] = modifierTracker.modifiersFor(r);
  }
  uiDirty = false;
#endif
}

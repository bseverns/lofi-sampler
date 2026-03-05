
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

// ---------- Globals ----------
Adafruit_USBD_MIDI usb_midi;
AudioEngine audio;
Storage storage;
RecorderADC rec;
TrellisUI ui;
ManifestCheck manifestStatus;
static const float DEFAULT_VOICE_LEVEL = 0.9f;

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

// ---------- Helpers ----------
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
      clockTransport.stop();
      audio.stop();
      auto restore = storage.restoreFactoryDemo();
      Serial.println(restore.message);
      manifestStatus = storage.checkManifest();
      logManifest(manifestStatus);
      audio.start();
    }
  }
}

// ---------- MIDI parsing ----------
// USB TinyUSB surfaces raw USB-MIDI packets. We only care about realtime 0xF8..0xFC commands.
void handleMidi() {
  while (usb_midi.available()) {
    midiEventPacket_t packet = MidiUSB.read();
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&packet);
    uint8_t b0 = bytes[1];
    if (clockTransport.handleRealtime(b0)) {
      stepPlayback.playStep(clockTransport.currentStep());
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
        bool consumed = padRouter.handlePress(r, c, mods);
        if (!consumed) {
          StepState step = gates[r][c];
          if (!step.gate) {
            if (step.velocity == 0) step.velocity = STEP_DEFAULT_VELOCITY;
            if (step.probability == 0) step.probability = STEP_DEFAULT_PROBABILITY;
            step.gate = true;
          } else {
            step.gate = false;
          }
          gates[r][c] = step;
          ui.setStep(r, c, step);
        }
      }
    } else {
      PadModifiers mods = modifierTracker.modifiersFor(r);
      recordingController.maybeCommitOverdubOnRelease(r, c, mods);
      modifierTracker.handleRelease(r, c);
    }
  }

  // Service recorder during record
  if (rec.isRecording()) {
    rec.service();
  }

  padRouter.serviceStutterDecay();

  audio.service();
  for (uint8_t r = 0; r < 4; ++r) {
    ui.setModifiers(r, modifierTracker.modifiersFor(r));
  }
  int recRow = recordingController.isRecording() ? (int)recordingController.activeRow() : -1;
  ui.draw(clockTransport.isPlaying() ? clockTransport.currentStep() : 255, recRow);
}


#include <Adafruit_TinyUSB.h>
#include "Config.h"
#include "AudioEngine.h"
#include "Storage.h"
#include "Slicer.h"
#include "RecorderADC.h"
#include "TrellisUI.h"

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
      bool alt   = gates[r][COL_ALT];
      bool shift = gates[r][COL_SHIFT];
      // Note: using dedicated columns as modifiers - hold the pad to keep active
      if (c == COL_ALT) { gates[r][COL_ALT] = true; }
      else if (c == COL_SHIFT) { gates[r][COL_SHIFT] = true; }
      else {
        if (shift && alt) {
          // reslice row from source.raw
          // read back source.raw into RAM? not needed; Slicer writes from RAM only.
          // For now, no-op unless we recorded this session.
        } else if (shift) {
          // REC/STOP
          if (!rec.isRecording()) {
            rec.start();
          } else {
            uint32_t n = rec.stop();
            if (n > 0) {
              // write /<Row>/source.raw and slices
              char rowL = "ABCD"[r];
              Slicer::writeEight(&rowL, rec.data(), n);
            }
          }
        } else if (alt) {
          // erase row files
          char rowL = "ABCD"[r];
          for (uint8_t i=0;i<8;i++) {
            char path[16]; snprintf(path,sizeof(path),"/%c/%c%d.raw",rowL,rowL,i+1);
            storage.remove(path);
          }
          char src[16]; snprintf(src,sizeof(src),"/%c/source.raw",rowL);
          storage.remove(src);
        } else {
          gates[r][c] = !gates[r][c];
          ui.setGate(r,c,gates[r][c]);
        }
      }
    } else {
      if (c == COL_ALT) gates[r][COL_ALT] = false;
      if (c == COL_SHIFT) gates[r][COL_SHIFT] = false;
    }
  }

  // Service recorder during record
  if (rec.isRecording()) {
    rec.service();
  }

  audio.service();
  ui.draw(playing ? stepIndex : 255, rec.isRecording() ? 0 : -1);
}

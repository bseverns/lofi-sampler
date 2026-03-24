
#pragma once
#include <Arduino.h>
#include <Adafruit_NeoTrellisM4.h>
#include "Config.h"
#include "PadInput.h"

struct StepState {
  bool gate;
  uint8_t velocity;    // MIDI-ish 0-127 feel; used to scale per-step levels
  uint8_t probability; // 0-100 percent
};

class TrellisUI {
public:
  bool begin();
  void setStep(uint8_t row, uint8_t col, const StepState& state);
  StepState getStep(uint8_t row, uint8_t col) const { return steps[row][col]; }
  void setModifiers(uint8_t row, const PadModifiers& mods);
  void draw(uint8_t step, int recRow); // recRow = -1 if none
  // returns -1 if no event; otherwise packed as:
  // bits 0..7   = col
  // bits 8..15  = row
  // bit  16     = pressed flag
  int32_t pollEvent();

private:
  Adafruit_NeoTrellisM4 trellis;
  PadModifiers modifiers[4] = {{false,false},{false,false},{false,false},{false,false}};
  StepState steps[4][8] = {{{0}}};
};

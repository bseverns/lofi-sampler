
#pragma once
#include <Arduino.h>
#include <Adafruit_NeoTrellisM4.h>
#include "Config.h"

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
  void draw(uint8_t step, int recRow); // recRow = -1 if none
  // returns -1 if no event; otherwise packed (row<<8) | col | (0x8000 for press)
  int32_t pollEvent();

private:
  Adafruit_NeoTrellisM4 trellis;
  StepState steps[4][8] = {{{0}}};
};

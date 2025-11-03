
#pragma once
#include <Arduino.h>
#include <Adafruit_NeoTrellisM4.h>
#include "Config.h"

class TrellisUI {
public:
  bool begin();
  void setGate(uint8_t row, uint8_t col, bool on);
  bool getGate(uint8_t row, uint8_t col) const { return gates[row][col]; }
  void draw(uint8_t step, int recRow); // recRow = -1 if none
  // returns -1 if no event; otherwise packed (row<<8) | col | (0x8000 for press)
  int32_t pollEvent();

private:
  Adafruit_NeoTrellisM4 trellis;
  bool gates[4][8] = {{0}};
};

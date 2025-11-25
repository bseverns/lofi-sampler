
#pragma once
#include <Arduino.h>

namespace Slicer {
  // Slice 'samples' into 8 equal segments and write to /<Row>/<Row>1.raw..8.raw
  bool writeEight(const char* rowLetter, const int16_t* samples, uint32_t count);
}

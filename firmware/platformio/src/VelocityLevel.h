#pragma once

#include <stdint.h>

inline float levelFromStepVelocity(uint8_t velocity, float defaultVoiceLevel) {
  float norm = velocity / 127.0f;
  float shaped = 0.45f + (norm * 0.55f);
  return shaped * defaultVoiceLevel;
}

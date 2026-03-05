#pragma once

#include <Arduino.h>
#include "AudioEngine.h"
#include "TrellisUI.h"

class StepPlaybackController {
public:
  StepPlaybackController(AudioEngine& audio,
                         StepState (&gates)[4][8],
                         float defaultVoiceLevel = 0.9f);

  void playStep(uint8_t stepIndex);

private:
  bool rollProbability(uint8_t probability) const;
  float levelFromVelocity(uint8_t velocity) const;

  AudioEngine& audio;
  StepState (&gates)[4][8];
  float defaultVoiceLevel;
};

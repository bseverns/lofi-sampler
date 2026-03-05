#include "StepPlaybackController.h"

StepPlaybackController::StepPlaybackController(AudioEngine& audioRef,
                                               StepState (&gatesRef)[4][8],
                                               float defaultVoiceLevelRef)
  : audio(audioRef), gates(gatesRef), defaultVoiceLevel(defaultVoiceLevelRef) {}

void StepPlaybackController::playStep(uint8_t stepIndex) {
  static const char ROW_LETTERS[4] = {'A', 'B', 'C', 'D'};
  for (uint8_t row = 0; row < 4; ++row) {
    const StepState& step = gates[row][stepIndex];
    if (step.gate && rollProbability(step.probability)) {
      char path[16];
      snprintf(path, sizeof(path), "/%c/%c%d.raw", ROW_LETTERS[row], ROW_LETTERS[row], stepIndex + 1);
      audio.setLevel(row, levelFromVelocity(step.velocity));
      audio.preloadAndPlay(row, path);
    } else {
      audio.stopVoice(row);
    }
  }
}

bool StepPlaybackController::rollProbability(uint8_t probability) const {
  if (probability >= 100) return true;
  if (probability == 0) return false;
  long roll = random(0, 100);
  return roll < probability;
}

float StepPlaybackController::levelFromVelocity(uint8_t velocity) const {
  float norm = velocity / 127.0f;
  float shaped = 0.45f + (norm * 0.55f);
  return shaped * defaultVoiceLevel;
}

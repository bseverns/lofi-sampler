#include "PadActionRouter.h"
#include <cstdio>

namespace {
static const uint8_t VELOCITY_LANES[] = {80, 108, 127};
static const uint8_t PROBABILITY_LANES[] = {35, 60, 85, 100};
}

PadActionRouter::PadActionRouter(AudioEngine& audioRef,
                                 TrellisUI& uiRef,
                                 StepState (&gatesRef)[4][8],
                                 RecordingController& recordingRef,
                                 float defaultVoiceLevelRef)
  : audio(audioRef),
    ui(uiRef),
    gates(gatesRef),
    recording(recordingRef),
    defaultVoiceLevel(defaultVoiceLevelRef) {}

bool PadActionRouter::handlePress(uint8_t row, uint8_t col, const PadModifiers& mods) {
  static const ActionFn ACTIONS[] = {
    &PadActionRouter::actionFx,
    &PadActionRouter::actionReslice,
    &PadActionRouter::actionCycleVelocity,
    &PadActionRouter::actionProbability,
    &PadActionRouter::actionStutter,
    &PadActionRouter::actionRecord,
    &PadActionRouter::actionErase,
  };

  bool consumed = false;
  for (ActionFn action : ACTIONS) {
    PadActionResult result = (this->*action)(row, col, mods);
    if (result == PadActionResult::MatchedContinue) {
      consumed = true;
      continue;
    }
    if (result == PadActionResult::MatchedStop) {
      consumed = true;
      break;
    }
  }
  return consumed;
}

void PadActionRouter::serviceStutterDecay() {
  uint32_t now = millis();
  for (uint8_t row = 0; row < 4; ++row) {
    uint32_t expire = stutterReleaseAt[row];
    if (expire && (int32_t)(now - expire) >= 0) {
      audio.setLevel(row, defaultVoiceLevel);
      stutterReleaseAt[row] = 0;
    }
  }
}

PadActionResult PadActionRouter::actionFx(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || !mods.shift) return PadActionResult::NoMatch;
  if (col >= STEPS_PER_BAR) return PadActionResult::NoMatch;

  switch (col) {
    case 0:
      audio.triggerFilterSweep(row);
      return PadActionResult::MatchedStop;
    case 1:
      audio.triggerBitcrush(row);
      return PadActionResult::MatchedStop;
    case 2:
      audio.triggerDrive(row);
      return PadActionResult::MatchedStop;
    case 3:
      audio.clearFx(row);
      return PadActionResult::MatchedStop;
    default:
      return PadActionResult::NoMatch;
  }
}

PadActionResult PadActionRouter::actionReslice(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || !mods.shift) return PadActionResult::NoMatch;
  if (col != (STEPS_PER_BAR - 3)) return PadActionResult::NoMatch;
  recording.resliceRow(row);
  return PadActionResult::MatchedStop;
}

PadActionResult PadActionRouter::actionCycleVelocity(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.shift || mods.alt) return PadActionResult::NoMatch;
  if (col >= STEPS_PER_BAR) return PadActionResult::NoMatch;

  StepState& step = gates[row][col];
  if (!step.gate) return PadActionResult::NoMatch;

  step.velocity = nextFromLanes(step.velocity, VELOCITY_LANES, sizeof(VELOCITY_LANES));
  ui.setStep(row, col, step);
  return PadActionResult::MatchedContinue;
}

PadActionResult PadActionRouter::actionProbability(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || mods.shift) return PadActionResult::NoMatch;
  if (col >= STEPS_PER_BAR) return PadActionResult::NoMatch;

  StepState& step = gates[row][col];
  if (!step.gate) return PadActionResult::NoMatch;

  step.probability = nextFromLanes(step.probability, PROBABILITY_LANES, sizeof(PROBABILITY_LANES));
  ui.setStep(row, col, step);
  return PadActionResult::MatchedStop;
}

PadActionResult PadActionRouter::actionStutter(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.shift || mods.alt) return PadActionResult::NoMatch;
  if (col >= STEPS_PER_BAR) return PadActionResult::NoMatch;
  if (!gates[row][col].gate) return PadActionResult::NoMatch;

  char rowL = "ABCD"[row];
  char path[16];
  snprintf(path, sizeof(path), "/%c/%c%d.raw", rowL, rowL, col + 1);
  float velocity = 0.35f + (0.08f * col);
  if (velocity > 1.0f) velocity = 1.0f;

  audio.setLevel(row, velocity);
  if (audio.preloadAndPlay(row, path)) {
    stutterReleaseAt[row] = millis() + 160;
    return PadActionResult::MatchedStop;
  }
  audio.setLevel(row, defaultVoiceLevel);
  stutterReleaseAt[row] = 0;
  return PadActionResult::MatchedStop;
}

PadActionResult PadActionRouter::actionRecord(uint8_t row, uint8_t col, const PadModifiers& mods) {
  (void)col;
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.shift || mods.alt) return PadActionResult::NoMatch;

  if (!recording.isRecording()) {
    recording.begin(row);
  } else if (recording.activeRow() == row) {
    recording.stopAndCommitReplace(row);
  }
  return PadActionResult::MatchedStop;
}

PadActionResult PadActionRouter::actionErase(uint8_t row, uint8_t col, const PadModifiers& mods) {
  (void)col;
  if (row >= 4) return PadActionResult::NoMatch;
  if (!mods.alt || mods.shift) return PadActionResult::NoMatch;
  recording.restoreOrEraseRow(row);
  return PadActionResult::MatchedStop;
}

uint8_t PadActionRouter::nextFromLanes(uint8_t current, const uint8_t* lanes, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (current == lanes[i]) {
      return lanes[(i + 1) % count];
    }
  }
  return lanes[0];
}

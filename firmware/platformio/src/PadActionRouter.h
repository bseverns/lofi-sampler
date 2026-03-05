#pragma once

#ifdef PAD_ROUTER_HOST_TEST
#include "PadRouterTestFakes.h"
#else
#include <Arduino.h>
#include "AudioEngine.h"
#include "PadInput.h"
#include "RecordingController.h"
#include "TrellisUI.h"
#endif

class PadActionRouter {
public:
  PadActionRouter(AudioEngine& audio,
                  TrellisUI& ui,
                  StepState (&gates)[4][8],
                  RecordingController& recording,
                  float defaultVoiceLevel = 0.9f);

  bool handlePress(uint8_t row, uint8_t col, const PadModifiers& mods);
  void serviceStutterDecay();

private:
  typedef PadActionResult (PadActionRouter::*ActionFn)(uint8_t row, uint8_t col, const PadModifiers& mods);

  PadActionResult actionFx(uint8_t row, uint8_t col, const PadModifiers& mods);
  PadActionResult actionReslice(uint8_t row, uint8_t col, const PadModifiers& mods);
  PadActionResult actionCycleVelocity(uint8_t row, uint8_t col, const PadModifiers& mods);
  PadActionResult actionProbability(uint8_t row, uint8_t col, const PadModifiers& mods);
  PadActionResult actionStutter(uint8_t row, uint8_t col, const PadModifiers& mods);
  PadActionResult actionRecord(uint8_t row, uint8_t col, const PadModifiers& mods);
  PadActionResult actionErase(uint8_t row, uint8_t col, const PadModifiers& mods);

  static uint8_t nextFromLanes(uint8_t current, const uint8_t* lanes, size_t count);

  AudioEngine& audio;
  TrellisUI& ui;
  StepState (&gates)[4][8];
  RecordingController& recording;
  float defaultVoiceLevel;
  uint32_t stutterReleaseAt[4] = {0, 0, 0, 0};
};

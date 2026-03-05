#pragma once

#include <array>
#include <cstdint>
#include <string>

static constexpr uint8_t STEPS_PER_BAR = 8;

struct StepState {
  bool gate;
  uint8_t velocity;
  uint8_t probability;
};

struct PadModifiers {
  bool alt;
  bool shift;
};

enum class PadActionResult : uint8_t {
  NoMatch = 0,
  MatchedContinue,
  MatchedStop
};

uint32_t millis();

class AudioEngine {
public:
  bool preloadAndPlay(uint8_t row, const char* path) {
    preloadCalls++;
    lastPreloadRow = row;
    lastPreloadPath = path ? path : "";
    return preloadShouldSucceed;
  }

  void setLevel(uint8_t row, float level) {
    if (row < levels.size()) {
      levels[row] = level;
    }
  }

  void stopVoice(uint8_t row) {
    stopVoiceCalls++;
    lastStopVoiceRow = row;
  }

  void triggerFilterSweep(uint8_t row) {
    filterCalls++;
    lastFxRow = row;
  }

  void triggerBitcrush(uint8_t row) {
    bitcrushCalls++;
    lastFxRow = row;
  }

  void triggerDrive(uint8_t row) {
    driveCalls++;
    lastFxRow = row;
  }

  void clearFx(uint8_t row) {
    clearFxCalls++;
    lastFxRow = row;
  }

  bool preloadShouldSucceed = true;
  std::array<float, 4> levels = {0.0f, 0.0f, 0.0f, 0.0f};
  uint32_t preloadCalls = 0;
  uint8_t lastPreloadRow = 255;
  std::string lastPreloadPath;
  uint32_t stopVoiceCalls = 0;
  uint8_t lastStopVoiceRow = 255;
  uint32_t filterCalls = 0;
  uint32_t bitcrushCalls = 0;
  uint32_t driveCalls = 0;
  uint32_t clearFxCalls = 0;
  uint8_t lastFxRow = 255;
};

class TrellisUI {
public:
  void setStep(uint8_t row, uint8_t col, const StepState& step) {
    setStepCalls++;
    lastRow = row;
    lastCol = col;
    lastStep = step;
  }

  uint32_t setStepCalls = 0;
  uint8_t lastRow = 255;
  uint8_t lastCol = 255;
  StepState lastStep = {false, 0, 0};
};

class RecordingController {
public:
  bool begin(uint8_t row) {
    beginCalls++;
    lastBeginRow = row;
    isRecordingFlag = true;
    activeRowValue = row;
    return true;
  }

  bool stopAndCommitReplace(uint8_t row) {
    stopCommitCalls++;
    lastStopCommitRow = row;
    isRecordingFlag = false;
    return true;
  }

  bool maybeCommitOverdubOnRelease(uint8_t, uint8_t, const PadModifiers&) { return false; }

  bool resliceRow(uint8_t row) {
    resliceCalls++;
    lastResliceRow = row;
    return resliceReturnValue;
  }

  void restoreOrEraseRow(uint8_t row) {
    restoreCalls++;
    lastRestoreRow = row;
  }

  bool isRecording() const { return isRecordingFlag; }
  uint8_t activeRow() const { return activeRowValue; }

  bool isRecordingFlag = false;
  uint8_t activeRowValue = 255;
  bool resliceReturnValue = true;
  uint32_t beginCalls = 0;
  uint8_t lastBeginRow = 255;
  uint32_t stopCommitCalls = 0;
  uint8_t lastStopCommitRow = 255;
  uint32_t resliceCalls = 0;
  uint8_t lastResliceRow = 255;
  uint32_t restoreCalls = 0;
  uint8_t lastRestoreRow = 255;
};

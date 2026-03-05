#pragma once

#include <Arduino.h>
#include "Config.h"
#include "PadInput.h"
#include "RecorderADC.h"
#include "Storage.h"

class RecordingController {
public:
  RecordingController(Storage& storage, RecorderADC& recorder);

  bool begin(uint8_t row);
  bool stopAndCommitReplace(uint8_t row);
  bool maybeCommitOverdubOnRelease(uint8_t row, uint8_t col, const PadModifiers& mods);

  bool resliceRow(uint8_t row);
  void restoreOrEraseRow(uint8_t row);

  bool isRecording() const { return recorder.isRecording(); }
  uint8_t activeRow() const { return recordingRow; }

private:
  enum class RecordMode : uint8_t {
    Replace = 0,
    Overdub
  };

  static constexpr uint16_t OVERDUB_HOLD_MS = 250;

  bool overdubAndSlice(uint8_t row, uint32_t newSamples);
  bool commitRecording(uint8_t row, uint32_t samples, RecordMode mode);
  void clearState();

  Storage& storage;
  RecorderADC& recorder;
  uint8_t recordingRow = 255;
  bool recordHoldCandidate = false;
  uint32_t recordStartMillis = 0;
};

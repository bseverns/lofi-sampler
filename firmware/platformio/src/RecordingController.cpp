#include "RecordingController.h"
#include "Slicer.h"

namespace {
char rowLetter(uint8_t row) {
  return "ABCD"[row];
}
}

RecordingController::RecordingController(Storage& storageRef, RecorderADC& recorderRef)
  : storage(storageRef), recorder(recorderRef) {}

bool RecordingController::begin(uint8_t row) {
  if (row >= 4) return false;
  if (recorder.isRecording()) return false;
  recordingRow = row;
  recordHoldCandidate = true;
  recordStartMillis = millis();
  recorder.start();
  return true;
}

bool RecordingController::stopAndCommitReplace(uint8_t row) {
  if (!recorder.isRecording()) return false;
  if (row >= 4 || recordingRow != row) return false;
  uint32_t samples = recorder.stop();
  recordHoldCandidate = false;
  return commitRecording(row, samples, RecordMode::Replace);
}

bool RecordingController::maybeCommitOverdubOnRelease(uint8_t row, uint8_t col, const PadModifiers& mods) {
  if (!recorder.isRecording()) return false;
  if (!recordHoldCandidate) return false;
  if (row >= 4 || recordingRow != row) return false;
  if (col >= STEPS_PER_BAR) return false;
  if (!mods.shift) return false;
  if ((millis() - recordStartMillis) < OVERDUB_HOLD_MS) return false;

  uint32_t samples = recorder.stop();
  recordHoldCandidate = false;
  return commitRecording(row, samples, RecordMode::Overdub);
}

bool RecordingController::resliceRow(uint8_t row) {
  if (row >= 4) return false;
  int16_t* scratch = recorder.mutableData();
  if (!scratch) return false;

  char rowL = rowLetter(row);
  char src[16];
  snprintf(src, sizeof(src), "/%c/source.raw", rowL);
  int32_t count = storage.readRawInto(src, scratch, MAX_RECORD_SAMPLES);
  if (count <= 0) {
    if (!storage.swapInPreviousSource(rowL)) {
      return false;
    }
    count = storage.readRawInto(src, scratch, MAX_RECORD_SAMPLES);
  }
  if (count <= 0) return false;

  return Slicer::writeEight(&rowL, scratch, (uint32_t)count, false);
}

void RecordingController::restoreOrEraseRow(uint8_t row) {
  if (row >= 4) return;
  char rowL = rowLetter(row);
  if (storage.swapInPreviousSource(rowL)) {
    resliceRow(row);
    return;
  }

  for (uint8_t i = 0; i < STEPS_PER_BAR; ++i) {
    char path[16];
    snprintf(path, sizeof(path), "/%c/%c%d.raw", rowL, rowL, i + 1);
    storage.remove(path);
  }
  char src[16];
  snprintf(src, sizeof(src), "/%c/source.raw", rowL);
  char prev[20];
  snprintf(prev, sizeof(prev), "/%c/source_prev.raw", rowL);
  storage.remove(src);
  storage.remove(prev);
}

bool RecordingController::overdubAndSlice(uint8_t row, uint32_t newSamples) {
  if (row >= 4) return false;
  char rowL = rowLetter(row);
  char src[16];
  snprintf(src, sizeof(src), "/%c/source.raw", rowL);

  int32_t existingCount = storage.rawSampleCount(src);
  if (existingCount <= 0) {
    return Slicer::writeEight(&rowL, recorder.data(), newSamples);
  }

  uint32_t total = (uint32_t)existingCount;
  if (newSamples > total) total = newSamples;
  if (total > MAX_RECORD_SAMPLES) total = MAX_RECORD_SAMPLES;

  int16_t* mix = recorder.mutableData();
  static const uint16_t CHUNK = 256;
  int16_t base[CHUNK];
  uint32_t offset = 0;
  while (offset < total) {
    uint32_t want = total - offset;
    if (want > CHUNK) want = CHUNK;
    int32_t got = storage.readRawChunk(src, offset, base, want);
    uint32_t baseCount = got > 0 ? (uint32_t)got : 0;
    for (uint32_t i = 0; i < want; ++i) {
      int32_t cur = (i < baseCount) ? base[i] : 0;
      int32_t incoming = (offset + i < newSamples) ? mix[offset + i] : 0;
      int32_t sum = cur + incoming;
      if (sum > 32767) sum = 32767;
      if (sum < -32768) sum = -32768;
      mix[offset + i] = (int16_t)sum;
    }
    offset += want;
  }
  return Slicer::writeEight(&rowL, mix, total);
}

bool RecordingController::commitRecording(uint8_t row, uint32_t samples, RecordMode mode) {
  if (samples == 0 || row >= 4) {
    clearState();
    return false;
  }

  bool ok = false;
  if (mode == RecordMode::Overdub) {
    ok = overdubAndSlice(row, samples);
  }
  if (mode == RecordMode::Replace || !ok) {
    char rowL = rowLetter(row);
    ok = Slicer::writeEight(&rowL, recorder.data(), samples);
  }

  clearState();
  return ok;
}

void RecordingController::clearState() {
  recordingRow = 255;
  recordHoldCandidate = false;
}

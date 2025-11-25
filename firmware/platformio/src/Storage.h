
#pragma once
#include <Arduino.h>

class Storage {
public:
  bool begin();
  // Read RAW 16-bit little-endian mono into dst, up to maxSamples.
  // Returns number of samples read.
  int32_t readRawInto(const char* path, int16_t* dst, uint32_t maxSamples);

  // Read a portion of a RAW file, starting at offsetSamples, into dst.
  // Returns number of samples copied, or negative on error.
  int32_t readRawChunk(const char* path, uint32_t offsetSamples, int16_t* dst, uint32_t maxSamples);

  // Query the total number of 16-bit samples in a RAW file.
  int32_t rawSampleCount(const char* path);

  // Write raw buffer to path
  bool writeRaw(const char* path, const int16_t* src, uint32_t samples);

  // Remove a file if exists
  void remove(const char* path);

  // Ensure row folders exist
  void ensureTree();

private:
  bool mounted = false;
};

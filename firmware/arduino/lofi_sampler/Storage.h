
#pragma once
#include <Arduino.h>

class Storage {
public:
  bool begin();
  // Read RAW 16-bit little-endian mono into dst, up to maxSamples.
  // Returns number of samples read.
  int32_t readRawInto(const char* path, int16_t* dst, uint32_t maxSamples);

  // Write raw buffer to path
  bool writeRaw(const char* path, const int16_t* src, uint32_t samples);

  // Remove a file if exists
  void remove(const char* path);

  // Ensure row folders exist
  void ensureTree();

private:
  bool mounted = false;
};

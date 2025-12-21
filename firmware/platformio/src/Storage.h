
#pragma once
#include <Arduino.h>
#include <vector>

struct ManifestCheck {
  bool manifestFound = false;   // true when manifest.json was present and parsed
  bool filesPresent = false;    // true when every listed file exists
  bool ok = false;              // shorthand for manifestFound && filesPresent
  uint16_t missingCount = 0;    // how many files failed the existence check/copy
  String version;               // optional version string pulled from the manifest
  String message;               // human-readable diagnostic
};

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

  // Write /<Row>/source.raw while copying the previous take to source_prev.raw.
  bool writeSourceWithBackup(char row, const int16_t* src, uint32_t samples);

  // Swap /<Row>/source_prev.raw back into source.raw (and vice-versa).
  bool swapInPreviousSource(char row);

  // Remove a file if exists
  void remove(const char* path);

  // Ensure row folders exist
  void ensureTree();

  // Probe the demo manifest (manifest.json) and confirm the listed files exist.
  ManifestCheck checkManifest(const char* manifestPath = "/manifest.json");

  // Attempt to repopulate the demo data from a bundled copy (default: /factory/*).
  ManifestCheck restoreFactoryDemo(const char* manifestPath = "/manifest.json",
                                   const char* factoryPrefix = "/factory");

private:
  bool readFileToString(const char* path, String& out);
  bool parseManifest(const String& payload, std::vector<String>& required, String& version);
  void buildDefaultRequired(std::vector<String>& required);
  bool fileExists(const char* path);
  bool copyIfExists(const String& src, const String& dst);
  String sourcePathFor(char row) const;
  String prevSourcePathFor(char row) const;
  uint16_t countMissing(const std::vector<String>& required);
  bool copyFile(const char* src, const char* dst);
  void ensureParentDir(const String& path);

  bool mounted = false;
};

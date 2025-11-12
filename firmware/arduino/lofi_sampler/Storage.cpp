
#include "Storage.h"
#include "Config.h"
#include <Adafruit_SPIFlash.h>
#include <Adafruit_LittleFS.h>
using namespace Adafruit_LittleFS_Namespace;

Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
LittleFS_QSPIFlash lfs(flash);

bool Storage::begin() {
  if (!flash.begin()) {
    return false;
  }
  if (!lfs.begin()) {
    // try to format
    if (!lfs.format()) return false;
    if (!lfs.begin()) return false;
  }
  mounted = true;
  ensureTree();
  return true;
}

int32_t Storage::readRawInto(const char* path, int16_t* dst, uint32_t maxSamples) {
  File f = lfs.open(path, FILE_O_READ);
  if (!f) return -1;
  // bytes to samples
  uint32_t avail = f.size() / 2;
  if (avail > maxSamples) avail = maxSamples;
  int32_t nread = f.read((uint8_t*)dst, avail*2);
  f.close();
  return nread/2;
}

int32_t Storage::readRawChunk(const char* path, uint32_t offsetSamples, int16_t* dst, uint32_t maxSamples) {
  File f = lfs.open(path, FILE_O_READ);
  if (!f) return -1;
  uint32_t totalSamples = f.size() / 2;
  if (offsetSamples >= totalSamples) {
    f.close();
    return 0;
  }
  uint32_t remaining = totalSamples - offsetSamples;
  if (remaining > maxSamples) remaining = maxSamples;
  if (!f.seek(offsetSamples * 2u)) {
    f.close();
    return -1;
  }
  // AudioEngine pulls in bite-sized chunks; keep it tight and synchronous.
  int32_t nread = f.read((uint8_t*)dst, remaining * 2u);
  f.close();
  return nread / 2;
}

int32_t Storage::rawSampleCount(const char* path) {
  File f = lfs.open(path, FILE_O_READ);
  if (!f) return -1;
  int32_t samples = f.size() / 2;
  f.close();
  return samples;
}

bool Storage::writeRaw(const char* path, const int16_t* src, uint32_t samples) {
  File f = lfs.open(path, FILE_O_WRITE | FILE_O_TRUNCATE | FILE_O_CREAT);
  if (!f) return false;
  uint32_t bytes = samples * 2;
  uint32_t wr = f.write((const uint8_t*)src, bytes);
  f.close();
  return wr == bytes;
}

void Storage::remove(const char* path) {
  lfs.remove(path);
}

void Storage::ensureTree() {
  lfs.mkdir(PATH_A);
  lfs.mkdir(PATH_B);
  lfs.mkdir(PATH_C);
  lfs.mkdir(PATH_D);
}

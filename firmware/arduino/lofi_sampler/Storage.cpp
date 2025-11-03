
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

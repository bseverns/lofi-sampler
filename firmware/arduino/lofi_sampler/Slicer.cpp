
#include "Slicer.h"
#include "Storage.h"
#include "Config.h"
#include <Adafruit_LittleFS.h>
using namespace Adafruit_LittleFS_Namespace;

extern Storage storage;

static String makePath(char row, uint8_t idx) {
  String p = "/";
  p += row;
  p += "/";
  p += row;
  p += String(idx);
  p += ".raw";
  return p;
}

bool Slicer::writeEight(const char* rowLetter, const int16_t* samples, uint32_t count) {
  if (!rowLetter || !rowLetter[0]) return false;
  char row = rowLetter[0];
  uint32_t seg = count / 8;
  for (uint8_t i=0; i<8; i++) {
    String path = makePath(row, i+1);
    const int16_t* start = samples + i*seg;
    if (!storage.writeRaw(path.c_str(), start, seg)) {
      return false;
    }
  }
  // also write source.raw
  String src = String("/") + row + "/source.raw";
  storage.writeRaw(src.c_str(), samples, count);
  return true;
}

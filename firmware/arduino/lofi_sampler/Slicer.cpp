
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
  if (!samples) return false;
  char row = rowLetter[0];
  // Base slice length; the final segment scoops up any remainder so nothing is lost.
  uint32_t seg = count / 8;
  uint32_t remainder = count - (seg * 8);
  uint32_t offset = 0;
  uint32_t totalWritten = 0;
  for (uint8_t i=0; i<8; i++) {
    String path = makePath(row, i+1);
    uint32_t segLen = seg;
    if (i == 7) {
      segLen += remainder;
    }
    if (offset >= count) {
      segLen = 0;
    } else if (segLen > (count - offset)) {
      segLen = count - offset;
    }
    const int16_t* start = samples + offset;
    if (!storage.writeRaw(path.c_str(), start, segLen)) {
      return false;
    }
    offset += segLen;
    totalWritten += segLen;
  }
#ifdef SLICER_DEBUG_TRACE
  Serial.print(F("Slicer wrote "));
  Serial.print(totalWritten);
  Serial.print(F(" samples across 8 slices"));
  if (totalWritten != count) {
    Serial.print(F(" (expected "));
    Serial.print(count);
    Serial.print(F(")"));
  }
  Serial.println();
#endif
  // also write source.raw
  String src = String("/") + row + "/source.raw";
  storage.writeRaw(src.c_str(), samples, count);
  return true;
}

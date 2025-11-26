#pragma once

#include <Arduino.h>
#include <Adafruit_SPIFlash.h>
#include <map>
#include <vector>

// Basic open flags mirroring the upstream Adafruit_LittleFS semantics.
#ifndef FILE_O_READ
#define FILE_O_READ      (0x01)
#endif
#ifndef FILE_O_WRITE
#define FILE_O_WRITE     (0x02)
#endif
#ifndef FILE_O_APPEND
#define FILE_O_APPEND    (0x04)
#endif
#ifndef FILE_O_TRUNCATE
#define FILE_O_TRUNCATE  (0x10)
#endif
#ifndef FILE_O_CREAT
#define FILE_O_CREAT     (0x20)
#endif

namespace Adafruit_LittleFS_Namespace {

class File {
 public:
  File();
  File(const File& other);
  File& operator=(const File& other);
  ~File();

  operator bool() const;

  size_t size() const;
  size_t read(uint8_t* buffer, size_t len);
  size_t write(const uint8_t* buffer, size_t len);
  bool seek(uint32_t pos);
  void close();

 private:
  // Grant the filesystem driver access to the in-memory bookkeeping so the
  // shim can stay lightweight without a bespoke allocator or opaque handle
  // layer.
  friend class LittleFS_QSPIFlash;

  struct MemoryFileRecord* record;
  size_t cursor;
  bool writable;
};

class LittleFS_QSPIFlash {
 public:
  explicit LittleFS_QSPIFlash(Adafruit_SPIFlash& flashTransport);

  bool begin();
  bool format();
  File open(const char* path, uint8_t mode = FILE_O_READ);
  bool remove(const char* path);
  bool mkdir(const char* path);

 private:
  Adafruit_SPIFlash* flash;
};

}  // namespace Adafruit_LittleFS_Namespace

using namespace Adafruit_LittleFS_Namespace;

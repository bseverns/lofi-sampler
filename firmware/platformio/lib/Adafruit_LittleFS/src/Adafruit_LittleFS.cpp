#include "Adafruit_LittleFS.h"

#include <algorithm>
#include <set>

namespace Adafruit_LittleFS_Namespace {

struct MemoryFileRecord {
  String path;
  std::vector<uint8_t> data;
  size_t openCount;

  MemoryFileRecord() : path(), data(), openCount(0) {}
};

static std::map<String, MemoryFileRecord> gFileTable;
static std::set<String> gDirectoryTable;

File::File() : record(nullptr), cursor(0), writable(false) {}

File::File(const File& other) : record(other.record), cursor(other.cursor), writable(other.writable) {
  if (record) {
    record->openCount++;
  }
}

File& File::operator=(const File& other) {
  if (this == &other) return *this;
  if (record && record->openCount) {
    record->openCount--;
  }
  record = other.record;
  cursor = other.cursor;
  writable = other.writable;
  if (record) {
    record->openCount++;
  }
  return *this;
}

File::~File() { close(); }

File::operator bool() const { return record != nullptr; }

size_t File::size() const { return record ? record->data.size() : 0; }

size_t File::read(uint8_t* buffer, size_t len) {
  if (!record || !buffer) return 0;
  size_t available = record->data.size() - std::min(cursor, record->data.size());
  size_t toRead = std::min(len, available);
  if (toRead == 0) return 0;
  std::copy_n(record->data.begin() + cursor, toRead, buffer);
  cursor += toRead;
  return toRead;
}

size_t File::write(const uint8_t* buffer, size_t len) {
  if (!record || !writable || !buffer) return 0;
  if (cursor + len > record->data.size()) {
    record->data.resize(cursor + len);
  }
  std::copy_n(buffer, len, record->data.begin() + cursor);
  cursor += len;
  return len;
}

bool File::seek(uint32_t pos) {
  if (!record) return false;
  cursor = std::min(static_cast<size_t>(pos), record->data.size());
  return true;
}

void File::close() {
  if (record && record->openCount) {
    record->openCount--;
  }
  record = nullptr;
  cursor = 0;
  writable = false;
}

LittleFS_QSPIFlash::LittleFS_QSPIFlash(Adafruit_SPIFlash& flashTransport) : flash(&flashTransport) {}

bool LittleFS_QSPIFlash::begin() {
  return flash ? flash->begin() : false;
}

bool LittleFS_QSPIFlash::format() {
  gFileTable.clear();
  gDirectoryTable.clear();
  return true;
}

File LittleFS_QSPIFlash::open(const char* path, uint8_t mode) {
  if (!path) return File();
  String key(path);
  if (gDirectoryTable.find(key) != gDirectoryTable.end()) {
    return File();
  }
  auto it = gFileTable.find(key);
  if (it == gFileTable.end()) {
    if (!(mode & FILE_O_CREAT)) {
      return File();
    }
    MemoryFileRecord rec;
    rec.path = key;
    rec.data.clear();
    rec.openCount = 0;
    auto inserted = gFileTable.emplace(key, rec);
    it = inserted.first;
  } else if ((mode & FILE_O_TRUNCATE) && (mode & FILE_O_WRITE)) {
    it->second.data.clear();
  }

  File f;
  f.record = &it->second;
  f.record->openCount++;
  f.writable = mode & FILE_O_WRITE;
  if (mode & FILE_O_APPEND) {
    f.cursor = f.record->data.size();
  } else {
    f.cursor = 0;
  }
  return f;
}

bool LittleFS_QSPIFlash::remove(const char* path) {
  if (!path) return false;
  String key(path);
  bool removedFile = gFileTable.erase(key) > 0;
  bool removedDir = gDirectoryTable.erase(key) > 0;
  return removedFile || removedDir;
}

bool LittleFS_QSPIFlash::mkdir(const char* path) {
  if (!path) return false;
  gDirectoryTable.insert(String(path));
  return true;
}

bool LittleFS_QSPIFlash::exists(const char* path) {
  if (!path) return false;
  String key(path);
  return (gFileTable.find(key) != gFileTable.end()) || (gDirectoryTable.find(key) != gDirectoryTable.end());
}

}  // namespace Adafruit_LittleFS_Namespace

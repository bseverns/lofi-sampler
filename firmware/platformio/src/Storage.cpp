
#include "Storage.h"
#include "Config.h"
#include <Adafruit_SPIFlash.h>
#include <Adafruit_LittleFS.h>
#include <vector>

namespace ALFS = Adafruit_LittleFS_Namespace;

Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
ALFS::LittleFS_QSPIFlash lfs(flash);

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
  ALFS::File f = lfs.open(path, FILE_O_READ);
  if (!f) return -1;
  // bytes to samples
  uint32_t avail = f.size() / 2;
  if (avail > maxSamples) avail = maxSamples;
  int32_t nread = f.read((uint8_t*)dst, avail*2);
  f.close();
  return nread/2;
}

int32_t Storage::readRawChunk(const char* path, uint32_t offsetSamples, int16_t* dst, uint32_t maxSamples) {
  ALFS::File f = lfs.open(path, FILE_O_READ);
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
  ALFS::File f = lfs.open(path, FILE_O_READ);
  if (!f) return -1;
  int32_t samples = f.size() / 2;
  f.close();
  return samples;
}

bool Storage::writeRaw(const char* path, const int16_t* src, uint32_t samples) {
  ALFS::File f = lfs.open(path, FILE_O_WRITE | FILE_O_TRUNCATE | FILE_O_CREAT);
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

bool Storage::readFileToString(const char* path, String& out) {
  ALFS::File f = lfs.open(path, FILE_O_READ);
  if (!f) {
    return false;
  }
  out.reserve(f.size() + 1);
  uint8_t buf[64];
  while (true) {
    size_t n = f.read(buf, sizeof(buf));
    if (n == 0) break;
    for (size_t i = 0; i < n; ++i) {
      out += (char)buf[i];
    }
  }
  f.close();
  return true;
}

bool Storage::parseManifest(const String& payload, std::vector<String>& required, String& version) {
  // Optional: pull a version string out of the manifest for logging.
  int vKey = payload.indexOf("\"version\"");
  if (vKey >= 0) {
    int colon = payload.indexOf(':', vKey);
    int vStart = payload.indexOf('"', colon);
    int vEnd = payload.indexOf('"', vStart + 1);
    if (vStart >= 0 && vEnd > vStart) {
      version = payload.substring(vStart + 1, vEnd);
    }
  }

  // The required file list lives in any quoted string that looks like an absolute path.
  int idx = 0;
  while (true) {
    int q1 = payload.indexOf('"', idx);
    if (q1 < 0) break;
    int q2 = payload.indexOf('"', q1 + 1);
    if (q2 < 0) break;
    String token = payload.substring(q1 + 1, q2);
    if (token.startsWith("/")) {
      required.push_back(token);
    }
    idx = q2 + 1;
  }

  return !required.empty();
}

void Storage::buildDefaultRequired(std::vector<String>& required) {
  const char rows[4] = {'A','B','C','D'};
  for (uint8_t r = 0; r < 4; ++r) {
    for (uint8_t i = 0; i < 8; ++i) {
      String path = String("/") + rows[r] + "/" + rows[r] + String(i + 1) + ".raw";
      required.push_back(path);
    }
    String src = String("/") + rows[r] + "/source.raw";
    required.push_back(src);
  }
}

bool Storage::fileExists(const char* path) {
  ALFS::File f = lfs.open(path, FILE_O_READ);
  bool ok = (bool)f;
  if (f) {
    f.close();
  }
  return ok;
}

uint16_t Storage::countMissing(const std::vector<String>& required) {
  uint16_t missing = 0;
  for (const auto& path : required) {
    if (!fileExists(path.c_str())) {
      ++missing;
    }
  }
  return missing;
}

bool Storage::copyFile(const char* src, const char* dst) {
  ALFS::File in = lfs.open(src, FILE_O_READ);
  if (!in) return false;
  ensureParentDir(String(dst));
  ALFS::File out = lfs.open(dst, FILE_O_WRITE | FILE_O_TRUNCATE | FILE_O_CREAT);
  if (!out) {
    in.close();
    return false;
  }

  uint8_t buf[128];
  bool ok = true;
  while (true) {
    size_t n = in.read(buf, sizeof(buf));
    if (n == 0) break;
    size_t w = out.write(buf, n);
    if (w != n) {
      ok = false;
      break;
    }
  }
  in.close();
  out.close();
  return ok;
}

void Storage::ensureParentDir(const String& path) {
  int slash = path.lastIndexOf('/');
  if (slash <= 0) return;
  String parent = path.substring(0, slash);
  if (parent.length() > 1) {
    ensureParentDir(parent); // build nested parents (/factory/A)
  }
  lfs.mkdir(parent.c_str());
}

ManifestCheck Storage::checkManifest(const char* manifestPath) {
  ManifestCheck report;
  if (!mounted) {
    report.message = F("Storage not mounted");
    return report;
  }

  String payload;
  std::vector<String> required;
  String version;
  if (readFileToString(manifestPath, payload)) {
    report.manifestFound = true;
    if (!parseManifest(payload, required, version)) {
      report.message = F("manifest.json unreadable (no required paths detected)");
      return report;
    }
  } else {
    report.message = F("manifest.json missing; using default slice map");
    buildDefaultRequired(required);
  }

  report.version = version;
  report.missingCount = countMissing(required);
  report.filesPresent = (report.missingCount == 0);
  report.ok = report.manifestFound && report.filesPresent;

  if (report.ok) {
    report.message = F("manifest ok");
    if (report.version.length() > 0) {
      report.message += " (v" + report.version + ")";
    }
  } else if (report.manifestFound) {
    report.message = String("manifest incomplete: ") + report.missingCount + " missing file(s)";
  } else {
    report.message += String("; ") + report.missingCount + " missing file(s) detected";
  }

  return report;
}

ManifestCheck Storage::restoreFactoryDemo(const char* manifestPath, const char* factoryPrefix) {
  ManifestCheck report;
  if (!mounted) {
    report.message = F("Storage not mounted");
    return report;
  }

  ensureTree();

  String payload;
  std::vector<String> required;
  String version;
  if (readFileToString(manifestPath, payload)) {
    report.manifestFound = true;
    parseManifest(payload, required, version); // best effort; may fall back
  }
  if (required.empty()) {
    buildDefaultRequired(required);
  }

  if (required.empty()) {
    report.message = F("No manifest entries to restore");
    return report;
  }

  uint16_t restored = 0;
  uint16_t failures = 0;
  for (const auto& path : required) {
    String src = String(factoryPrefix) + path;
    if (copyFile(src.c_str(), path.c_str())) {
      ++restored;
    } else {
      ++failures;
    }
  }

  report.missingCount = failures;
  report.filesPresent = (failures == 0);
  report.ok = report.filesPresent;
  report.version = version;

  if (report.ok) {
    report.message = String("Factory restore ok (copied ") + restored + ")";
  } else {
    report.message = String("Factory restore incomplete: ") + failures + " missing source file(s)";
  }
  return report;
}

#include <iostream>
#include <fstream>
#include <regex>
#include <string>

namespace {
int failures = 0;

bool expectTrue(bool condition, const std::string& name) {
  if (!condition) {
    ++failures;
    std::cerr << "[FAIL] " << name << "\n";
    return false;
  }
  return true;
}

void expectEq(uint32_t actual, uint32_t expected, const std::string& name) {
  if (actual != expected) {
    ++failures;
    std::cerr << "[FAIL] " << name << " expected=" << expected
              << " actual=" << actual << "\n";
  }
}

uint32_t readJobQueueSize() {
  std::ifstream in(AUDIO_ENGINE_HEADER);
  if (!expectTrue(in.good(), "AudioEngine.h opens")) {
    return 0;
  }

  std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::smatch match;
  if (!expectTrue(std::regex_search(body, match, std::regex("JOB_QUEUE_SIZE\\s*=\\s*([0-9]+)")),
                  "JOB_QUEUE_SIZE constant found")) {
    return 0;
  }
  return static_cast<uint32_t>(std::stoul(match[1].str()));
}

void testFourVoiceStepBurstFitsQueue() {
  static constexpr uint8_t voices = 4;
  static constexpr uint8_t jobsPerActiveVoice = 2; // setLevel + preload
  static constexpr uint8_t requiredJobs = voices * jobsPerActiveVoice;
  uint32_t usableRingSlots = readJobQueueSize() - 1u;

  expectEq(usableRingSlots, requiredJobs, "four active rows fit one queued step burst");
}
} // namespace

int main() {
  testFourVoiceStepBurstFitsQueue();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "All AudioEngine queue tests passed\n";
  return 0;
}

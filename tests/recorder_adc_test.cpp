#include <iostream>
#include <string>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#include "RecorderADC.h"
#undef private
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

uint32_t g_micros = 0;
uint32_t g_reads = 0;

void analogReadResolution(int) {}
void analogReference(eAnalogReference) {}
uint32_t micros() { return g_micros; }
int analogRead(int) {
  ++g_reads;
  return 2048;
}
void noInterrupts() {}
void interrupts() {}

namespace {
int failures = 0;

void expectTrue(bool condition, const std::string& name) {
  if (!condition) {
    ++failures;
    std::cerr << "[FAIL] " << name << "\n";
  }
}

void expectEq(uint32_t actual, uint32_t expected, const std::string& name) {
  if (actual != expected) {
    ++failures;
    std::cerr << "[FAIL] " << name << " expected=" << expected
              << " actual=" << actual << "\n";
  }
}

void testForegroundServiceDoesNotCaptureSamples() {
  RecorderADC recorder;
  expectTrue(recorder.begin(), "recorder buffer allocates");

  recorder.start();
  g_micros += (1000000UL / SAMPLE_RATE_HZ) * 10u;

  expectEq(recorder.service(), 0, "service reports progress without foreground sampling");
  expectEq(g_reads, 0, "foreground service does not call analogRead");
  expectEq(recorder.stop(), 0, "stop returns timer-captured sample count only");
}

void testTimerCompareTracksConfiguredSampleRate() {
  expectEq(RecorderADC::timerCompareForSampleRate(48000000UL, SAMPLE_RATE_HZ),
           2176,
           "48 MHz timer compare rounds to 22.05 kHz");
}
} // namespace

int main() {
  testForegroundServiceDoesNotCaptureSamples();
  testTimerCompareTracksConfiguredSampleRate();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "All RecorderADC tests passed\n";
  return 0;
}

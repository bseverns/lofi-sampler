#include <iostream>
#include <string>
#include "ClockTransport.h"

namespace {
int failures = 0;

void expectTrue(bool condition, const std::string& name) {
  if (!condition) {
    ++failures;
    std::cerr << "[FAIL] " << name << "\n";
  }
}

void expectEq(uint8_t actual, uint8_t expected, const std::string& name) {
  if (actual != expected) {
    ++failures;
    std::cerr << "[FAIL] " << name << " expected=" << static_cast<int>(expected)
              << " actual=" << static_cast<int>(actual) << "\n";
  }
}

void expectEqU32(uint32_t actual, uint32_t expected, const std::string& name) {
  if (actual != expected) {
    ++failures;
    std::cerr << "[FAIL] " << name << " expected=" << expected
              << " actual=" << actual << "\n";
  }
}

uint8_t clocksUntilNextStep(ClockTransport& transport) {
  uint8_t clocks = 0;
  while (true) {
    ++clocks;
    if (transport.handleRealtime(0xF8)) {
      return clocks;
    }
    if (clocks > 64) {
      ++failures;
      std::cerr << "[FAIL] transport did not advance within 64 clocks\n";
      return clocks;
    }
  }
}

void testStartAdvancesImmediately() {
  ClockTransport transport(8, 12, 0.15f);
  expectTrue(!transport.isPlaying(), "initially stopped");

  expectTrue(transport.handleRealtime(0xFA), "start advances immediately");
  expectTrue(transport.isPlaying(), "start sets playing");
  expectEq(transport.currentStep(), 0, "start lands on step zero");

  expectEq(clocksUntilNextStep(transport), 14, "first swung interval after start");
  expectEq(transport.currentStep(), 1, "advanced to step one");
}

void testClockOnlyAutoStart() {
  ClockTransport transport(8, 12, 0.15f);

  expectTrue(transport.handleRealtime(0xF8), "first clock auto-starts transport");
  expectTrue(transport.isPlaying(), "clock-only source sets playing");
  expectEq(transport.currentStep(), 0, "clock-only start lands on step zero");
}

void testSwingStepsNeedExtraTicks() {
  ClockTransport transport(8, 12, 0.15f); // rounded swing = 2 ticks
  transport.handleRealtime(0xFA);         // step = 0

  expectEq(clocksUntilNextStep(transport), 14, "swung step waits for extra ticks");
  expectEq(transport.currentStep(), 1, "arrived at swung step");

  expectEq(clocksUntilNextStep(transport), 10, "compensated step returns borrowed ticks");
  expectEq(transport.currentStep(), 2, "arrived at straight step");
}

void testSwingIntervalsPreserveBarLength() {
  ClockTransport transport(8, 12, 0.15f);
  static const uint8_t expectedIntervals[8] = {14, 10, 14, 10, 14, 10, 14, 10};

  transport.handleRealtime(0xFA);
  uint32_t total = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    uint8_t clocks = clocksUntilNextStep(transport);
    total += clocks;
    expectEq(clocks, expectedIntervals[i], "paired swing interval");
  }
  expectEq(transport.currentStep(), 0, "bar wrap lands on step zero");
  expectEqU32(total, 96, "paired swing preserves 96 clocks per bar");
}

void testContinuePreservesStepPosition() {
  ClockTransport transport(8, 12, 0.15f);
  transport.handleRealtime(0xFA);
  clocksUntilNextStep(transport);
  expectEq(transport.currentStep(), 1, "at step 1 before stop");

  transport.handleRealtime(0xFC);
  expectTrue(!transport.isPlaying(), "stop clears playing");
  expectEq(transport.currentStep(), 1, "step freezes on stop");

  transport.handleRealtime(0xFB);
  expectTrue(transport.isPlaying(), "continue resumes playing");
  expectEq(clocksUntilNextStep(transport), 10, "continue keeps prior step position");
  expectEq(transport.currentStep(), 2, "continued from frozen step");
}

void testClockAfterStopAutoRestartsFromZero() {
  ClockTransport transport(8, 12, 0.15f);
  transport.handleRealtime(0xFA);
  clocksUntilNextStep(transport);
  expectEq(transport.currentStep(), 1, "at step 1 before stop-auto-restart check");

  transport.handleRealtime(0xFC);
  expectTrue(transport.handleRealtime(0xF8), "first bare clock after stop auto-restarts");
  expectEq(transport.currentStep(), 0, "bare clock after stop restarts from step zero");
}

void testSwingClamp() {
  ClockTransport transport(8, 12, 1.0f); // clamp should cap at +6 ticks
  transport.handleRealtime(0xFA);
  expectEq(transport.currentStep(), 0, "start lands on step 0 before clamped swing check");

  expectEq(clocksUntilNextStep(transport), 18, "clamped swung step advances at 18 clocks");
  expectEq(transport.currentStep(), 1, "clamped swung step reached");
  expectEq(clocksUntilNextStep(transport), 6, "clamped compensated step returns six clocks");
  expectEq(transport.currentStep(), 2, "clamped compensated step reached");
}
} // namespace

int main() {
  testStartAdvancesImmediately();
  testClockOnlyAutoStart();
  testSwingStepsNeedExtraTicks();
  testSwingIntervalsPreserveBarLength();
  testContinuePreservesStepPosition();
  testClockAfterStopAutoRestartsFromZero();
  testSwingClamp();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "All ClockTransport tests passed\n";
  return 0;
}

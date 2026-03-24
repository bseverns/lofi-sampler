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

void testStartAdvancesImmediately() {
  ClockTransport transport(8, 12, 0.15f);
  expectTrue(!transport.isPlaying(), "initially stopped");

  expectTrue(transport.handleRealtime(0xFA), "start advances immediately");
  expectTrue(transport.isPlaying(), "start sets playing");
  expectEq(transport.currentStep(), 0, "start lands on step zero");

  for (int i = 0; i < 13; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "swung step waits for extra ticks after start");
  }
  expectTrue(transport.handleRealtime(0xF8), "14th clock advances first swung step");
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

  for (int i = 0; i < 13; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "swung step waits for extra ticks");
  }
  expectTrue(transport.handleRealtime(0xF8), "14th clock advances swung step");
  expectEq(transport.currentStep(), 1, "arrived at swung step");

  for (int i = 0; i < 11; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "straight step waits for base ticks");
  }
  expectTrue(transport.handleRealtime(0xF8), "12th clock advances straight step");
  expectEq(transport.currentStep(), 2, "arrived at straight step");
}

void testContinuePreservesStepPosition() {
  ClockTransport transport(8, 12, 0.15f);
  transport.handleRealtime(0xFA);
  for (int i = 0; i < 14; ++i) {
    transport.handleRealtime(0xF8);
  }
  expectEq(transport.currentStep(), 1, "at step 1 before stop");

  transport.handleRealtime(0xFC);
  expectTrue(!transport.isPlaying(), "stop clears playing");
  expectEq(transport.currentStep(), 1, "step freezes on stop");

  transport.handleRealtime(0xFB);
  expectTrue(transport.isPlaying(), "continue resumes playing");
  for (int i = 0; i < 11; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "continue keeps prior step position");
  }
  expectTrue(transport.handleRealtime(0xF8), "12th clock after continue advances");
  expectEq(transport.currentStep(), 2, "continued from frozen step");
}

void testClockAfterStopAutoRestartsFromZero() {
  ClockTransport transport(8, 12, 0.15f);
  transport.handleRealtime(0xFA);
  for (int i = 0; i < 14; ++i) {
    transport.handleRealtime(0xF8);
  }
  expectEq(transport.currentStep(), 1, "at step 1 before stop-auto-restart check");

  transport.handleRealtime(0xFC);
  expectTrue(transport.handleRealtime(0xF8), "first bare clock after stop auto-restarts");
  expectEq(transport.currentStep(), 0, "bare clock after stop restarts from step zero");
}

void testSwingClamp() {
  ClockTransport transport(8, 12, 1.0f); // clamp should cap at +6 ticks
  transport.handleRealtime(0xFA);
  expectEq(transport.currentStep(), 0, "start lands on step 0 before clamped swing check");

  for (int i = 0; i < 17; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "clamped swung step does not advance early");
  }
  expectTrue(transport.handleRealtime(0xF8), "clamped swung step advances at 18 clocks");
  expectEq(transport.currentStep(), 1, "clamped swung step reached");
}
} // namespace

int main() {
  testStartAdvancesImmediately();
  testClockOnlyAutoStart();
  testSwingStepsNeedExtraTicks();
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

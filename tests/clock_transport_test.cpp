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

void testStartAndAdvance() {
  ClockTransport transport(8, 12, 0.15f);
  expectTrue(!transport.isPlaying(), "initially stopped");

  expectTrue(!transport.handleRealtime(0xFA), "start does not advance immediately");
  expectTrue(transport.isPlaying(), "start sets playing");
  expectEq(transport.currentStep(), 7, "start sets pre-roll step");

  for (int i = 0; i < 11; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "pre-advance clocks do not step");
  }
  expectTrue(transport.handleRealtime(0xF8), "12th clock advances");
  expectEq(transport.currentStep(), 0, "advanced to step zero");
}

void testSwingStepsNeedExtraTicks() {
  ClockTransport transport(8, 12, 0.15f); // rounded swing = 2 ticks
  transport.handleRealtime(0xFA);         // step = 7

  // Advance to step 0
  for (int i = 0; i < 12; ++i) {
    transport.handleRealtime(0xF8);
  }
  expectEq(transport.currentStep(), 0, "arrived at step 0");

  // Next step is swung (step 1) so it should require 14 clocks.
  for (int i = 0; i < 13; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "swung step waits for extra ticks");
  }
  expectTrue(transport.handleRealtime(0xF8), "14th clock advances swung step");
  expectEq(transport.currentStep(), 1, "arrived at swung step");

  // Next step (2) is straight, so 12 clocks.
  for (int i = 0; i < 11; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "straight step waits for base ticks");
  }
  expectTrue(transport.handleRealtime(0xF8), "12th clock advances straight step");
  expectEq(transport.currentStep(), 2, "arrived at straight step");
}

void testContinueAndStop() {
  ClockTransport transport(8, 12, 0.15f);
  transport.handleRealtime(0xFA);
  for (int i = 0; i < 12; ++i) {
    transport.handleRealtime(0xF8);
  }
  expectEq(transport.currentStep(), 0, "at step 0 before stop");

  transport.handleRealtime(0xFC);
  expectTrue(!transport.isPlaying(), "stop clears playing");

  for (int i = 0; i < 30; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "clock ignored while stopped");
  }
  expectEq(transport.currentStep(), 0, "step frozen while stopped");

  transport.handleRealtime(0xFB);
  expectTrue(transport.isPlaying(), "continue resumes playing");
  for (int i = 0; i < 13; ++i) {
    transport.handleRealtime(0xF8);
  }
  expectTrue(transport.handleRealtime(0xF8), "continue keeps prior step position");
  expectEq(transport.currentStep(), 1, "continued from frozen step");
}

void testSwingClamp() {
  ClockTransport transport(8, 12, 1.0f); // clamp should cap at +6 ticks
  transport.handleRealtime(0xFA);
  for (int i = 0; i < 12; ++i) {
    transport.handleRealtime(0xF8);
  }
  expectEq(transport.currentStep(), 0, "at step 0 before clamped swing check");

  for (int i = 0; i < 17; ++i) {
    expectTrue(!transport.handleRealtime(0xF8), "clamped swung step does not advance early");
  }
  expectTrue(transport.handleRealtime(0xF8), "clamped swung step advances at 18 clocks");
  expectEq(transport.currentStep(), 1, "clamped swung step reached");
}
}

int main() {
  testStartAndAdvance();
  testSwingStepsNeedExtraTicks();
  testContinueAndStop();
  testSwingClamp();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "All ClockTransport tests passed\n";
  return 0;
}

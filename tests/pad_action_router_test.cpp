#include <iostream>
#include <string>
#include "PadActionRouter.h"

uint32_t g_now_ms = 0;
uint32_t millis() { return g_now_ms; }

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

void expectEqU8(uint8_t actual, uint8_t expected, const std::string& name) {
  if (actual != expected) {
    ++failures;
    std::cerr << "[FAIL] " << name << " expected=" << static_cast<int>(expected)
              << " actual=" << static_cast<int>(actual) << "\n";
  }
}

void expectNear(float actual, float expected, float eps, const std::string& name) {
  float delta = actual - expected;
  if (delta < 0.0f) delta = -delta;
  if (delta > eps) {
    ++failures;
    std::cerr << "[FAIL] " << name << " expected~=" << expected
              << " actual=" << actual << "\n";
  }
}

PadActionRouter makeRouter(AudioEngine& audio, TrellisUI& ui, StepState (&gates)[4][8], RecordingController& rec) {
  return PadActionRouter(audio, ui, gates, rec, 0.9f);
}

void testFxCombos() {
  AudioEngine audio;
  TrellisUI ui;
  RecordingController rec;
  StepState gates[4][8] = {};
  auto router = makeRouter(audio, ui, gates, rec);
  PadModifiers mods = {true, true};

  expectTrue(router.handlePress(2, 0, mods), "fx filter consumed");
  expectEq(audio.filterCalls, 1, "filter triggered");
  expectEqU8(audio.lastFxRow, 2, "fx row tracked");

  expectTrue(router.handlePress(2, 1, mods), "fx bitcrush consumed");
  expectEq(audio.bitcrushCalls, 1, "bitcrush triggered");

  expectTrue(router.handlePress(2, 2, mods), "fx drive consumed");
  expectEq(audio.driveCalls, 1, "drive triggered");

  expectTrue(router.handlePress(2, 3, mods), "fx clear consumed");
  expectEq(audio.clearFxCalls, 1, "clear fx triggered");
}

void testResliceCombo() {
  AudioEngine audio;
  TrellisUI ui;
  RecordingController rec;
  StepState gates[4][8] = {};
  auto router = makeRouter(audio, ui, gates, rec);
  PadModifiers mods = {true, true};

  expectTrue(router.handlePress(1, 5, mods), "reslice combo consumed");
  expectEq(rec.resliceCalls, 1, "reslice called once");
  expectEqU8(rec.lastResliceRow, 1, "reslice row");
}

void testVelocityThenStutterChain() {
  AudioEngine audio;
  TrellisUI ui;
  RecordingController rec;
  StepState gates[4][8] = {};
  gates[0][2] = {true, 80, 100};
  auto router = makeRouter(audio, ui, gates, rec);
  PadModifiers mods = {false, true};

  g_now_ms = 1000;
  expectTrue(router.handlePress(0, 2, mods), "shift lit-step consumed");
  expectEqU8(gates[0][2].velocity, 108, "velocity lane advanced");
  expectEq(ui.setStepCalls, 1, "ui updated after velocity change");
  expectEq(audio.preloadCalls, 1, "stutter fired preload");
  expectEqU8(audio.lastPreloadRow, 0, "stutter row");
  expectTrue(audio.lastPreloadPath == "/A/A3.raw", "stutter path");
  float expectedLevel = (0.45f + ((108.0f / 127.0f) * 0.55f)) * 0.9f;
  expectNear(audio.levels[0], expectedLevel, 0.001f, "stutter uses sequenced velocity curve");

  // After stutter timeout, level should return to default.
  g_now_ms = 1200;
  router.serviceStutterDecay();
  expectNear(audio.levels[0], 0.9f, 0.001f, "stutter decay restores default level");
}

void testProbabilityLane() {
  AudioEngine audio;
  TrellisUI ui;
  RecordingController rec;
  StepState gates[4][8] = {};
  gates[3][4] = {true, 108, 35};
  auto router = makeRouter(audio, ui, gates, rec);
  PadModifiers mods = {true, false};

  expectTrue(router.handlePress(3, 4, mods), "alt lit-step consumed");
  expectEqU8(gates[3][4].probability, 60, "probability lane advanced");
  expectEq(ui.setStepCalls, 1, "ui updated after probability change");
  expectEq(rec.restoreCalls, 0, "restore fallback not triggered for lit step probability cycle");
}

void testRecordAndRestoreFallback() {
  AudioEngine audio;
  TrellisUI ui;
  RecordingController rec;
  StepState gates[4][8] = {};
  auto router = makeRouter(audio, ui, gates, rec);

  PadModifiers shiftOnly = {false, true};
  expectTrue(router.handlePress(2, 6, shiftOnly), "record start consumed");
  expectEq(rec.beginCalls, 1, "record begin called");
  expectEqU8(rec.lastBeginRow, 2, "record begin row");

  expectTrue(router.handlePress(2, 6, shiftOnly), "record stop consumed");
  expectEq(rec.stopCommitCalls, 1, "record stop commit called");
  expectEqU8(rec.lastStopCommitRow, 2, "record stop row");

  PadModifiers altOnly = {true, false};
  expectTrue(router.handlePress(1, 7, altOnly), "restore fallback consumed");
  expectEq(rec.restoreCalls, 1, "restore-or-blank called");
  expectEqU8(rec.lastRestoreRow, 1, "restore-or-blank row");
}
}

int main() {
  testFxCombos();
  testResliceCombo();
  testVelocityThenStutterChain();
  testProbabilityLane();
  testRecordAndRestoreFallback();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "All PadActionRouter tests passed\n";
  return 0;
}

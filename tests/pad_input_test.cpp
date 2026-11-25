#include <iostream>
#include "PadInput.h"

namespace {
int failures = 0;

void expectTrue(bool condition, const char* name) {
  if (!condition) {
    ++failures;
    std::cerr << "[FAIL] " << name << "\n";
  }
}

void testOffsetModifiersOwnTheirPrecedingRow() {
  ModifierTracker tracker;
  tracker.reset();

  expectTrue(tracker.handlePress(1, COL_ALT), "row B col 7 is a modifier press");
  expectTrue(tracker.handlePress(1, COL_SHIFT), "row B col 8 is a modifier press");
  PadModifiers rowA = tracker.modifiersFor(0);
  expectTrue(rowA.alt && rowA.shift, "row B modifiers target row A");

  tracker.noteModifierUse(0, rowA);
  expectTrue(tracker.handleRelease(1, COL_ALT) == ModifierReleaseResult::UsedAsModifier,
             "used Alt does not become a step tap");
  expectTrue(tracker.handleRelease(1, COL_SHIFT) == ModifierReleaseResult::UsedAsModifier,
             "used Shift does not become a step tap");
}

void testUnchordedModifierColumnsRemainStepsOnRelease() {
  ModifierTracker tracker;
  tracker.reset();

  expectTrue(tracker.handlePress(3, COL_ALT), "col 7 press arms tap fallback");
  expectTrue(tracker.handleRelease(3, COL_ALT) == ModifierReleaseResult::TapAsStep,
             "unchorded col 7 releases as a normal step");
  expectTrue(tracker.handlePress(3, COL_SHIFT), "col 8 press arms tap fallback");
  expectTrue(tracker.handleRelease(3, COL_SHIFT) == ModifierReleaseResult::TapAsStep,
             "unchorded col 8 releases as a normal step");
}
}

int main() {
  testOffsetModifiersOwnTheirPrecedingRow();
  testUnchordedModifierColumnsRemainStepsOnRelease();
  if (failures) return 1;
  std::cout << "All PadInput tests passed\n";
  return 0;
}

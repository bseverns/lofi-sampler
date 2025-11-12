#include "PadInput.h"

namespace {
  static const uint8_t MAX_PAD_ACTIONS = 8;
  // Simple FIFO registry — small enough that a linear walk keeps things obvious
  // on a microcontroller without dynamic allocation footguns.
  PadComboAction registry[MAX_PAD_ACTIONS] = {nullptr};
  uint8_t registryCount = 0;
}

void ModifierTracker::reset() {
  for (uint8_t i = 0; i < 4; ++i) {
    altState[i] = false;
    shiftState[i] = false;
  }
}

bool ModifierTracker::handlePress(uint8_t row, uint8_t col) {
  if (row >= 4) return false;
  if (col == COL_ALT) {
    altState[row] = true;
    return true;
  }
  if (col == COL_SHIFT) {
    shiftState[row] = true;
    return true;
  }
  return false;
}

bool ModifierTracker::handleRelease(uint8_t row, uint8_t col) {
  if (row >= 4) return false;
  if (col == COL_ALT) {
    altState[row] = false;
    return true;
  }
  if (col == COL_SHIFT) {
    shiftState[row] = false;
    return true;
  }
  return false;
}

PadModifiers ModifierTracker::modifiersFor(uint8_t row) const {
  PadModifiers mods = {false, false};
  if (row < 4) {
    mods.alt = altState[row];
    mods.shift = shiftState[row];
  }
  return mods;
}

void resetPadActionRegistry() {
  for (uint8_t i = 0; i < MAX_PAD_ACTIONS; ++i) {
    registry[i] = nullptr;
  }
  registryCount = 0;
}

bool registerPadAction(PadComboAction action) {
  if (!action) return false;
  if (registryCount >= MAX_PAD_ACTIONS) return false;
  registry[registryCount++] = action;
  return true;
}

bool handlePadCombo(uint8_t row, uint8_t col, const PadModifiers& mods) {
  bool consumed = false;
  for (uint8_t i = 0; i < registryCount; ++i) {
    PadComboAction fn = registry[i];
    if (!fn) continue;
    PadActionResult res = fn(row, col, mods);
    if (res == PadActionResult::MatchedContinue) {
      consumed = true; // handler wants other callbacks but the UI should skip fallbacks
      continue;
    }
    if (res == PadActionResult::MatchedStop) {
      consumed = true;
      break;
    }
  }
  return consumed;
}

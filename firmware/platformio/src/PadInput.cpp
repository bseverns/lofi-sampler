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
    altTapPending[i] = false;
    shiftTapPending[i] = false;
    altChordUsed[i] = false;
    shiftChordUsed[i] = false;
  }
}

uint8_t ModifierTracker::owningRow(uint8_t modifierRow) const {
  return (modifierRow + 4 - MOD_ROW_OFFSET) % 4;
}

bool ModifierTracker::isModifierPad(uint8_t row, uint8_t col) const {
  return row < 4 && (col == COL_ALT || col == COL_SHIFT);
}

bool ModifierTracker::handlePress(uint8_t row, uint8_t col) {
  if (!isModifierPad(row, col)) return false;
  uint8_t owner = owningRow(row);
  if (col == COL_ALT) {
    altState[owner] = true;
    altTapPending[owner] = true;
    altChordUsed[owner] = false;
    return true;
  }
  shiftState[owner] = true;
  shiftTapPending[owner] = true;
  shiftChordUsed[owner] = false;
  return true;
}

ModifierReleaseResult ModifierTracker::handleRelease(uint8_t row, uint8_t col) {
  if (!isModifierPad(row, col)) return ModifierReleaseResult::NotModifier;
  uint8_t owner = owningRow(row);
  if (col == COL_ALT) {
    bool tapAsStep = altTapPending[owner] && !altChordUsed[owner];
    altState[owner] = false;
    altTapPending[owner] = false;
    altChordUsed[owner] = false;
    return tapAsStep ? ModifierReleaseResult::TapAsStep : ModifierReleaseResult::UsedAsModifier;
  }
  bool tapAsStep = shiftTapPending[owner] && !shiftChordUsed[owner];
  shiftState[owner] = false;
  shiftTapPending[owner] = false;
  shiftChordUsed[owner] = false;
  return tapAsStep ? ModifierReleaseResult::TapAsStep : ModifierReleaseResult::UsedAsModifier;
}

void ModifierTracker::noteModifierUse(uint8_t row, const PadModifiers& mods) {
  if (row >= 4) return;
  if (mods.alt && altTapPending[row]) {
    altChordUsed[row] = true;
  }
  if (mods.shift && shiftTapPending[row]) {
    shiftChordUsed[row] = true;
  }
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

#pragma once
#include <Arduino.h>
#include "Config.h"

struct PadModifiers {
  bool alt;
  bool shift;
};

// Result contract for PadComboAction callbacks:
// - NoMatch: callback ignored the press, fall through to the next handler/default gate toggle.
// - MatchedContinue: callback handled the press but wants the chain to continue (layered FX, logging, etc.).
//   The event is still considered consumed for default UI logic.
// - MatchedStop: callback handled the press and short-circuits the chain.
enum class PadActionResult : uint8_t {
  NoMatch = 0,
  MatchedContinue,
  MatchedStop
};

// Lightweight helper that mirrors the state of the per-row modifier columns so
// combo handlers can query "shift/alt" without polluting the playback gate
// matrix. Intended to be reset on setup() and fed every key press/release.
class ModifierTracker {
public:
  void reset();
  bool handlePress(uint8_t row, uint8_t col);
  bool handleRelease(uint8_t row, uint8_t col);
  PadModifiers modifiersFor(uint8_t row) const;

private:
  bool altState[4] = {false, false, false, false};
  bool shiftState[4] = {false, false, false, false};
};

typedef PadActionResult (*PadComboAction)(uint8_t row, uint8_t col, const PadModifiers& mods);

void resetPadActionRegistry();
bool registerPadAction(PadComboAction action);
bool handlePadCombo(uint8_t row, uint8_t col, const PadModifiers& mods);

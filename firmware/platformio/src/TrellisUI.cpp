
#include "TrellisUI.h"

bool TrellisUI::begin() {
  trellis.begin();
  trellis.setBrightness(255);
  for (uint8_t r=0;r<4;r++)
    for (uint8_t c=0;c<8;c++) {
      StepState s = {false, STEP_DEFAULT_VELOCITY, STEP_DEFAULT_PROBABILITY};
      setStep(r,c,s);
    }
  draw(255,-1);
  return true;
}

void TrellisUI::setStep(uint8_t row, uint8_t col, const StepState& state) {
  steps[row][col] = state;
}

void TrellisUI::setModifiers(uint8_t row, const PadModifiers& mods) {
  if (row >= 4) return;
  modifiers[row] = mods;
}

void TrellisUI::draw(uint8_t step, int recRow) {
  for (uint8_t r=0;r<4;r++) {
    for (uint8_t c=0;c<8;c++) {
      const StepState& s = steps[r][c];
      float velBoost = (s.gate ? (s.velocity / 127.0f) * 0.3f : 0.0f);
      float probAttenuation = 0.6f + (s.probability / 100.0f) * 0.4f; // 0.6..1.0
      float m = (s.gate ? BRIGHT_ON : BRIGHT_OFF);
      m = (m + velBoost) * probAttenuation;
      if (c == step) m = BRIGHT_STEP;
      uint32_t color = trellis.Color(ROW_COLOR[r].r*m, ROW_COLOR[r].g*m, ROW_COLOR[r].b*m);
      if (recRow == r) {
        // red pulse overlay
        color = trellis.Color(255, 40, 40);
      }
      uint8_t pixel = (r * 8u) + c;
      trellis.setPixelColor(pixel, color);
    }
  }
  // Overlay modifier latches onto the offset row so the step grid stays full width.
  for (uint8_t owner = 0; owner < 4; ++owner) {
    uint8_t modRow = (owner + MOD_ROW_OFFSET) % 4;
    const PadModifiers& mods = modifiers[owner];
    if (mods.alt) {
      uint8_t pixel = (modRow * 8u) + COL_ALT;
      float m = BRIGHT_STEP;
      uint32_t color = trellis.Color(ROW_COLOR[owner].r*m, ROW_COLOR[owner].g*m, ROW_COLOR[owner].b*m);
      trellis.setPixelColor(pixel, color);
    }
    if (mods.shift) {
      uint8_t pixel = (modRow * 8u) + COL_SHIFT;
      float m = BRIGHT_STEP;
      uint32_t color = trellis.Color(ROW_COLOR[owner].r*m, ROW_COLOR[owner].g*m, ROW_COLOR[owner].b*m);
      trellis.setPixelColor(pixel, color);
    }
  }
  trellis.show();
}

int32_t TrellisUI::pollEvent() {
  int32_t out = -1;
  if (trellis.available()) {
    keypadEvent e = trellis.read();
    uint8_t k = e.bit.KEY;
    uint8_t r = k / 8, c = k % 8;
    bool pressed = e.bit.EVENT == KEY_JUST_PRESSED;
    out = ((int32_t)r<<8) | c | (pressed ? 0x8000 : 0);
  }
  return out;
}

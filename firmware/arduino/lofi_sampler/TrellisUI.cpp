
#include "TrellisUI.h"

bool TrellisUI::begin() {
  trellis.begin();
  trellis.setBrightness(255);
  for (uint8_t r=0;r<4;r++)
    for (uint8_t c=0;c<8;c++)
      setGate(r,c,false);
  draw(255,-1);
  return true;
}

void TrellisUI::setGate(uint8_t row, uint8_t col, bool on) {
  gates[row][col] = on;
}

void TrellisUI::draw(uint8_t step, int recRow) {
  for (uint8_t r=0;r<4;r++) {
    for (uint8_t c=0;c<8;c++) {
      float m = gates[r][c] ? BRIGHT_ON : BRIGHT_OFF;
      if (c == step) m = BRIGHT_STEP;
      uint32_t color = trellis.Color(ROW_COLOR[r].r*m, ROW_COLOR[r].g*m, ROW_COLOR[r].b*m);
      if (recRow == r) {
        // red pulse overlay
        color = trellis.Color(255, 40, 40);
      }
      trellis.setPixelColor(c, r, color);
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

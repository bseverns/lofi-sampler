
#include "RecorderADC.h"

bool RecorderADC::begin() {
  buf = (int16_t*)malloc(CAP * sizeof(int16_t));
  // Ensure the ADC hands us the 12-bit values the rest of the math expects.
  analogReadResolution(12);
  // Light averaging = less hiss without smearing transients; tweak if needed.
  analogReadAveraging(4);
  // Stay explicit about the reference so 0..4095 maps to 0..3.3 V bias network.
  analogReference(AR_DEFAULT);
  return buf != nullptr;
}

void RecorderADC::start() {
  if (!buf) return;
  idx = 0;
  rec = true;
  nextMicros = micros();
}

uint32_t RecorderADC::stop() {
  rec = false;
  return idx;
}

uint32_t RecorderADC::service() {
  if (!rec) return idx;
  // naive timed sampling; good enough for short takes
  uint32_t period = 1000000UL / SAMPLE_RATE_HZ;
  uint32_t now = micros();
  if ((int32_t)(now - nextMicros) >= 0) {
    nextMicros += period;
    int v = analogRead(analogPin) - 2048; // 12-bit centered
    // scale 12-bit to 16-bit
    int16_t s = (int16_t)(v << 4);
    if (idx < CAP) buf[idx++] = s;
    else rec = false;
  }
  return idx;
}

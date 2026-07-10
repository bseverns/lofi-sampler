
#include "RecorderADC.h"

#ifndef RECORDER_ADC_HOST_TEST
#include <Adafruit_ZeroTimer.h>

namespace {
static constexpr uint8_t RECORDER_TIMER_NUM = 4;
// Adafruit_ZeroTimer uses GCLK1 at 48 MHz for SAMD51 TC3/4/5.
static constexpr uint32_t RECORDER_TIMER_HZ = 48000000UL;
static Adafruit_ZeroTimer recorderTimer(RECORDER_TIMER_NUM);
static RecorderADC* s_recorder = nullptr;

static void stopRecorderTimer() {
  recorderTimer.enable(false);
}
}

extern "C" void TC4_Handler() {
  Adafruit_ZeroTimer::timerHandler(RECORDER_TIMER_NUM);
}
#else
namespace {
static void stopRecorderTimer() {}
}
#endif

bool RecorderADC::begin() {
  if (!buf) {
    buf = (int16_t*)malloc(CAP * sizeof(int16_t));
  }
  // Ensure the ADC hands us the 12-bit values the rest of the math expects.
  analogReadResolution(12);
  // Light averaging = less hiss without smearing transients; tweak if needed.
#if defined(analogReadAveraging)
  analogReadAveraging(4);
#endif
  // Stay explicit about the reference so 0..4095 maps to 0..3.3 V bias network.
  analogReference(AR_DEFAULT);
#ifndef RECORDER_ADC_HOST_TEST
  s_recorder = this;
  recorderTimer.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  recorderTimer.setCompare(0, timerCompareForSampleRate(RECORDER_TIMER_HZ, SAMPLE_RATE_HZ));
  recorderTimer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, onTimerISR);
  recorderTimer.enable(false);
#if defined(TC4_IRQn) && defined(__NVIC_PRIO_BITS)
  NVIC_SetPriority(TC4_IRQn, (1UL << __NVIC_PRIO_BITS) - 1);
#endif
#endif
  return buf != nullptr;
}

void RecorderADC::start() {
  if (!buf) return;
#ifndef RECORDER_ADC_HOST_TEST
  stopRecorderTimer();
#endif
  noInterrupts();
  idx = 0;
  rec = true;
  interrupts();
#ifndef RECORDER_ADC_HOST_TEST
  recorderTimer.enable(true);
#endif
}

uint32_t RecorderADC::stop() {
  noInterrupts();
  rec = false;
  uint32_t count = idx;
  interrupts();
#ifndef RECORDER_ADC_HOST_TEST
  stopRecorderTimer();
#endif
  return count;
}

uint32_t RecorderADC::service() {
  noInterrupts();
  uint32_t count = idx;
  interrupts();
  return count;
}

uint32_t RecorderADC::timerCompareForSampleRate(uint32_t timerHz, uint32_t sampleRateHz) {
  if (timerHz == 0 || sampleRateHz == 0) return 0;
  uint32_t divisor = (timerHz + (sampleRateHz / 2u)) / sampleRateHz;
  if (divisor == 0) divisor = 1;
  if (divisor > 65536u) divisor = 65536u;
  return divisor - 1u;
}

void RecorderADC::onTimerISR() {
#ifndef RECORDER_ADC_HOST_TEST
  if (s_recorder) {
    s_recorder->captureSample();
  }
#endif
}

void RecorderADC::captureSample() {
  if (!rec || !buf) return;

  uint32_t writeIndex = idx;
  if (writeIndex >= CAP) {
    rec = false;
    stopRecorderTimer();
    return;
  }

  int v = analogRead(analogPin) - 2048; // 12-bit centered
  buf[writeIndex] = (int16_t)(v << 4);
  idx = writeIndex + 1u;

  if (idx >= CAP) {
    rec = false;
    stopRecorderTimer();
  }
}


#include "AudioEngine.h"
#include "Storage.h"
#include <Adafruit_ZeroTimer.h>

static Adafruit_ZeroTimer zt = Adafruit_ZeroTimer(3); // TC3/4/5 depend on chip; 3 works on M4

static AudioEngine* s_self = nullptr;

bool AudioEngine::begin() {
  analogWriteResolution(12);
  pinMode(DAC_PIN_L, OUTPUT);
  pinMode(DAC_PIN_R, OUTPUT);
  s_self = this;

  // Configure ZeroTimer to fire at SAMPLE_RATE_HZ
  // Use setPeriod with HERTZ mode
  zt.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  zt.setCompare(0, (F_CPU / SAMPLE_RATE_HZ) - 1);
  zt.setCallback(true, TC_CALLBACK_CC_CHANNEL0, (tc_callback_t)onTimerISR);
  return true;
}

void AudioEngine::start() {
  running = true;
  zt.enable(true);
}

void AudioEngine::stop() {
  running = false;
  zt.enable(false);
}

void AudioEngine::setLevel(uint8_t v, float lv) {
  if (v<4) vgain[v] = lv;
}

bool AudioEngine::preloadAndPlay(uint8_t voice, const char* path) {
  if (!storage) return false;
  if (voice>=4) return false;
  // Read whole slice (<= ~20KB typically) into vbuf
  int32_t n = storage->readRawInto(path, vbuf[voice], BUF_SAMPLES);
  if (n <= 0) return false;
  vlen[voice] = (uint32_t)n;
  vpos[voice] = 0;
  return true;
}

void AudioEngine::stopVoice(uint8_t voice) {
  if (voice>=4) return;
  vpos[voice] = vlen[voice]; // run to end
}

void AudioEngine::service() {
  // nothing yet — reserved for future prefetch
}

void AudioEngine::onTimerISR(AudioEngine* self) {
  if (self) self->isr();
}

void AudioEngine::isr() {
  if (!running) return;
  int32_t mix = 0;
  for (uint8_t v=0; v<4; v++) {
    uint32_t p = vpos[v];
    if (p < vlen[v]) {
      int32_t s = vbuf[v][p];
      mix += (int32_t)(s * vgain[v]);
      vpos[v] = p + 1;
    }
  }
  // saturate to 12-bit DAC range centered
  int32_t out = mix >> 1; // soft gain
  if (out < -2047) out = -2047;
  if (out >  2047) out =  2047;
  uint16_t dac = (uint16_t)(out + 2048); // 0..4095
  analogWrite(DAC_PIN_L, dac);
  analogWrite(DAC_PIN_R, dac);
}

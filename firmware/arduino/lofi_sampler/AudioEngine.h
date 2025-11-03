
#pragma once
#include <Arduino.h>
#include "Config.h"

// Forward decl for Storage read
class Storage;

class AudioEngine {
public:
  bool begin();
  void attachStorage(Storage* s) { storage = s; }
  void start();
  void stop();

  // schedule to play a raw slice file (e.g., "/A/A1.raw") on a voice (0..3)
  bool preloadAndPlay(uint8_t voice, const char* path);

  // stop a voice
  void stopVoice(uint8_t voice);

  // service to refill timing (called from loop)
  void service();

  // debug level
  void setLevel(uint8_t voice, float level);

private:
  static void onTimerISR(AudioEngine* self);
  void isr();

  Storage* storage = nullptr;
  volatile bool running = false;

  // Simple per-voice RAM buffer for current slice. Big enough to slurp an entire
  // recorded slice (MAX_RECORD_SAMPLES chopped into 8 pieces, rounded up).
  static constexpr uint32_t BUF_SAMPLES = (MAX_RECORD_SAMPLES + 7u) / 8u;
  int16_t  vbuf[4][BUF_SAMPLES];
  uint32_t vlen[4] = {0,0,0,0};
  volatile uint32_t vpos[4] = {0,0,0,0};
  float    vgain[4] = {0.9f,0.9f,0.9f,0.9f};
};


#pragma once
#include <Arduino.h>
#include "Config.h"


// Forward decl for Storage read
class Storage;

// AudioEngine is the mixer + transport glue. The main loop calls service()
// to shovel jobs and buffers around; the ISR only mixes ready samples.
class AudioEngine {
public:
  bool begin();
  void attachStorage(Storage* s) { storage = s; }
  void start();
  void stop();
  void playSelfTestTone(uint16_t durationMs = 250, uint16_t frequencyHz = 440);

  // schedule to play a raw slice file (e.g., "/A/A1.raw") on a voice (0..3)
  bool preloadAndPlay(uint8_t voice, const char* path);

  // stop a voice
  void stopVoice(uint8_t voice);

  // service to refill timing (called from loop)
  void service();

  // debug level
  void setLevel(uint8_t voice, float level);

  // FX triggers (queued so the ISR never builds tables on the fly)
  void triggerFilterSweep(uint8_t voice);
  void triggerBitcrush(uint8_t voice);
  void triggerDrive(uint8_t voice);
  void clearFx(uint8_t voice);

  // Request a state dump for a voice (queued to avoid ISR clashes).
  void requestDiagnostics(uint8_t voice);

private:
  static constexpr uint8_t  JOB_QUEUE_SIZE = 8;
  static constexpr uint8_t  MAX_PATH_LEN   = 32;
  static constexpr int16_t  SELF_TEST_AMPLITUDE = 384;
  // Simple per-voice RAM buffer for current slice. Big enough to slurp an entire
  // recorded slice (MAX_RECORD_SAMPLES chopped into 8 pieces, rounded up).
  static constexpr uint32_t BUF_SAMPLES = (MAX_RECORD_SAMPLES + 7u) / 8u;
  static constexpr uint32_t STREAM_CHUNK   =
      (BUF_SAMPLES > 256u) ? 256u : ((BUF_SAMPLES > 64u) ? (BUF_SAMPLES / 2u) : BUF_SAMPLES);
  static void onTimerISR();
  void isr();

  enum class JobType : uint8_t {
    None,
    Preload,
    Fade,
    Diagnostics,
    FilterSweep,
    Bitcrush,
    Drive,
    FxClear,
  };

  struct Job {
    // Lightweight payload: enough to describe a preload path, target gain, etc.
    JobType type = JobType::None;
    uint8_t voice = 0;
    char path[MAX_PATH_LEN] = {0};
    float value = 0.0f;
    uint16_t frames = 0;
  };

  bool enqueueJob(const Job& job);
  bool popJob(Job& jobOut);
  void handleJob(const Job& job);
  void handlePreload(const Job& job);
  void handleFade(const Job& job);
  void handleDiagnostics(const Job& job);
  void handleFilterSweep(const Job& job);
  void handleBitcrush(const Job& job);
  void handleDrive(const Job& job);
  void handleFxClear(const Job& job);
  void pumpStreams();
  void pumpGains();
  void pumpEffects();
  void cleanupVoice(uint8_t voice);
  void armGainRamp(uint8_t voice, float target, uint16_t frames);
  void resetFx(uint8_t voice);

  Storage* storage = nullptr;
  volatile bool running = false;

  Job jobQueue[JOB_QUEUE_SIZE];
  volatile uint8_t jobHead = 0;
  volatile uint8_t jobTail = 0;

  int16_t  vbuf[4][BUF_SAMPLES];
  volatile uint32_t vavailable[4] = {0,0,0,0};
  volatile uint32_t vpos[4] = {0,0,0,0};
  uint32_t vwrite[4] = {0,0,0,0};

  // Flags touch both the ISR and service(); keep them volatile and tidy.
  volatile bool voiceActive[4] = {false,false,false,false};
  volatile bool voicePrimed[4] = {false,false,false,false};
  volatile bool voiceStreaming[4] = {false,false,false,false};
  volatile bool voiceDraining[4] = {false,false,false,false};

  uint32_t voiceTotalSamples[4] = {0,0,0,0};
  uint32_t voiceLoadedSamples[4] = {0,0,0,0};
  bool     voiceDiagPending[4] = {false,false,false,false};
  bool     voiceNeedsFadeIn[4] = {false,false,false,false};
  char     voicePath[4][MAX_PATH_LEN];

  volatile float vgainCurrent[4] = {0.0f,0.0f,0.0f,0.0f};
  float    vgainTarget[4] = {0.9f,0.9f,0.9f,0.9f};
  float    vgainDesired[4] = {0.9f,0.9f,0.9f,0.9f};
  float    vgainStep[4]   = {0,0,0,0};
  uint16_t vgainFrames[4] = {0,0,0,0};

  // FX tables are precomputed in handleJob() so the ISR only lifts current slots.
  static constexpr uint16_t FX_TABLE_SIZE = 128;
  float filterTable[4][FX_TABLE_SIZE];
  float driveTable[4][FX_TABLE_SIZE];
  uint16_t crushTable[4][FX_TABLE_SIZE];

  uint16_t filterLen[4] = {0,0,0,0};
  uint16_t driveLen[4]  = {0,0,0,0};
  uint16_t crushLen[4]  = {0,0,0,0};

  volatile uint16_t filterIndex[4] = {0,0,0,0};
  volatile uint16_t driveIndex[4]  = {0,0,0,0};
  volatile uint16_t crushIndex[4]  = {0,0,0,0};

  volatile float    filterCurrent[4] = {1.0f,1.0f,1.0f,1.0f};
  volatile float    driveCurrent[4]  = {1.0f,1.0f,1.0f,1.0f};
  volatile uint16_t crushCurrentHold[4] = {0,0,0,0};
  volatile uint16_t crushCountdown[4]   = {0,0,0,0};
  int16_t           crushLatchedSample[4] = {0,0,0,0};
  uint8_t           crushShift[4] = {0,0,0,0};

  volatile uint32_t selfTestPhase = 0;
  volatile uint32_t selfTestPhaseStep = 0;
  volatile uint32_t selfTestRemaining = 0;
};

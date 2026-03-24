#include "AudioEngine.h"
#include "Storage.h"
#include <Adafruit_ZeroTimer.h>
#include <math.h>
#include <string.h>

/*
 * The sampler is now split into two clear personalities:
 *   • service() – runs in the main loop, drains the job queue, streams flash
 *     into circular voice buffers, and nudges gain ramps along.
 *   • isr()      – the 22.05 kHz timer interrupt that simply mixes whatever
 *     service() already staged. No filesystem calls, no math surprises.
 *
 * Jobs give us a scratchpad for everything that needs coordination (preloads,
 * fades, diagnostics) without letting the ISR touch slow code paths. The queue
 * is tiny on purpose; if we overflow it, something upstream is spamming work
 * faster than the main loop can keep up and we log it loudly.
 */

static Adafruit_ZeroTimer zt = Adafruit_ZeroTimer(3); // TC3/4/5 depend on chip; 3 works on M4

static AudioEngine* s_self = nullptr;

extern "C" void TC3_Handler() {
  Adafruit_ZeroTimer::timerHandler(3);
}

namespace {
static constexpr uint16_t DEFAULT_FADE_FRAMES = 96;
static constexpr uint16_t STOP_FADE_FRAMES    = 128;

#if defined(DAC) && defined(__SAMD51__)
static void primeDacOutputs() {
  // Use the core helper once in setup context so pin muxing and DAC enable
  // happen outside the audio ISR.
  analogWrite(DAC_PIN_L, 2048);
  analogWrite(DAC_PIN_R, 2048);
}

static inline void writeDacSample(uint16_t value) {
  if (DAC->STATUS.bit.READY0 && !DAC->SYNCBUSY.bit.DATA0) {
    DAC->DATA[0].reg = value;
  }
  if (DAC->STATUS.bit.READY1 && !DAC->SYNCBUSY.bit.DATA1) {
    DAC->DATA[1].reg = value;
  }
}
#else
static void primeDacOutputs() {
  analogWrite(DAC_PIN_L, 2048);
  analogWrite(DAC_PIN_R, 2048);
}

static inline void writeDacSample(uint16_t value) {
  analogWrite(DAC_PIN_L, value);
  analogWrite(DAC_PIN_R, value);
}
#endif
}

bool AudioEngine::begin() {
  analogWriteResolution(12);
  primeDacOutputs();
  s_self = this;
  selfTestPhase = 0;
  selfTestPhaseStep = 0;
  selfTestRemaining = 0;

  jobHead = 0;
  jobTail = 0;
  for (uint8_t v = 0; v < 4; ++v) {
    vavailable[v] = 0;
    vpos[v] = 0;
    vwrite[v] = 0;
    voiceActive[v] = false;
    voicePrimed[v] = false;
    voiceStreaming[v] = false;
    voiceDraining[v] = false;
    voiceTotalSamples[v] = 0;
    voiceLoadedSamples[v] = 0;
    voiceDiagPending[v] = false;
    voiceNeedsFadeIn[v] = false;
    memset(voicePath[v], 0, MAX_PATH_LEN);
    vgainCurrent[v] = 0.0f;
    vgainTarget[v] = 0.9f;
    vgainDesired[v] = 0.9f;
    vgainStep[v] = 0.0f;
    vgainFrames[v] = 0;
    filterLen[v] = 0;
    driveLen[v] = 0;
    crushLen[v] = 0;
    filterIndex[v] = 0;
    driveIndex[v] = 0;
    crushIndex[v] = 0;
    filterCurrent[v] = 1.0f;
    driveCurrent[v] = 1.0f;
    crushCurrentHold[v] = 0;
    crushCountdown[v] = 0;
    crushLatchedSample[v] = 0;
    crushShift[v] = 0;
  }

  // Configure ZeroTimer to fire at SAMPLE_RATE_HZ
  zt.configure(TC_CLOCK_PRESCALER_DIV1, TC_COUNTER_SIZE_16BIT, TC_WAVE_GENERATION_MATCH_FREQ);
  zt.setCompare(0, (F_CPU / SAMPLE_RATE_HZ) - 1);
  zt.setCallback(true, TC_CALLBACK_CC_CHANNEL0, onTimerISR);
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

void AudioEngine::playSelfTestTone(uint16_t durationMs, uint16_t frequencyHz) {
  if (durationMs == 0) durationMs = 1;
  if (frequencyHz == 0) frequencyHz = 440;

  uint32_t samples = ((uint32_t)durationMs * SAMPLE_RATE_HZ + 999u) / 1000u;
  uint32_t phaseStep = (uint32_t)(((uint64_t)frequencyHz << 32) / SAMPLE_RATE_HZ);

  noInterrupts();
  selfTestPhase = 0;
  selfTestPhaseStep = phaseStep;
  selfTestRemaining = samples;
  interrupts();
}

void AudioEngine::setLevel(uint8_t v, float lv) {
  if (v >= 4) return;
  vgainDesired[v] = lv;
  Job job;
  job.type = JobType::Fade;
  job.voice = v;
  job.value = lv;
  job.frames = DEFAULT_FADE_FRAMES;
  if (!enqueueJob(job)) {
    armGainRamp(v, lv, DEFAULT_FADE_FRAMES);
  }
}

void AudioEngine::triggerFilterSweep(uint8_t voice) {
  if (voice >= 4) return;
  Job job;
  job.type = JobType::FilterSweep;
  job.voice = voice;
  job.value = FILTER_SWEEP_DEPTH;
  job.frames = FILTER_SWEEP_TABLE_SIZE;
  enqueueJob(job);
}

void AudioEngine::triggerBitcrush(uint8_t voice) {
  if (voice >= 4) return;
  Job job;
  job.type = JobType::Bitcrush;
  job.voice = voice;
  job.value = BITCRUSH_DEPTH_BITS;
  job.frames = BITCRUSH_RATE_TABLE;
  enqueueJob(job);
}

void AudioEngine::triggerDrive(uint8_t voice) {
  if (voice >= 4) return;
  Job job;
  job.type = JobType::Drive;
  job.voice = voice;
  job.value = DRIVE_DEPTH_MULT;
  job.frames = DRIVE_SWELL_TABLE;
  enqueueJob(job);
}

void AudioEngine::clearFx(uint8_t voice) {
  if (voice >= 4) return;
  Job job;
  job.type = JobType::FxClear;
  job.voice = voice;
  enqueueJob(job);
}

void AudioEngine::requestDiagnostics(uint8_t voice) {
  if (voice >= 4) return;
  Job job;
  job.type = JobType::Diagnostics;
  job.voice = voice;
  enqueueJob(job);
}

bool AudioEngine::preloadAndPlay(uint8_t voice, const char* path) {
  if (!storage) return false;
  if (voice >= 4 || !path) return false;
  Job job;
  job.type = JobType::Preload;
  job.voice = voice;
  strncpy(job.path, path, MAX_PATH_LEN - 1);
  job.path[MAX_PATH_LEN - 1] = '\0';
  return enqueueJob(job);
}

void AudioEngine::stopVoice(uint8_t voice) {
  if (voice >= 4) return;
  noInterrupts();
  voiceStreaming[voice] = false;
  voiceDraining[voice] = true;
  interrupts();

  Job job;
  job.type = JobType::Fade;
  job.voice = voice;
  job.value = 0.0f;
  job.frames = STOP_FADE_FRAMES;
  if (!enqueueJob(job)) {
    armGainRamp(voice, 0.0f, STOP_FADE_FRAMES);
  }
}

void AudioEngine::service() {
  // The main loop calls this once per frame. We clear the queue first so
  // freshly scheduled preloads/fades don't stall behind streaming work.
  Job job;
  for (uint8_t i = 0; i < JOB_QUEUE_SIZE; ++i) {
    if (!popJob(job)) break;
    handleJob(job);
  }

  // After the paperwork, keep the buffers primed and gains gliding.
  pumpStreams();
  pumpGains();
  pumpEffects();

  // Voices that drained out get recycled back to a clean slate.
  for (uint8_t v = 0; v < 4; ++v) {
    cleanupVoice(v);
  }
}

void AudioEngine::onTimerISR() {
  if (s_self) s_self->isr();
}

void AudioEngine::isr() {
  if (!running) return;
  int32_t mix = 0;
  for (uint8_t v = 0; v < 4; ++v) {
    if (!voicePrimed[v]) continue;
    uint32_t avail = vavailable[v];
    if (avail == 0) {
      if (!voiceStreaming[v]) {
        voiceActive[v] = false;
      }
      continue;
    }
    uint32_t readIdx = vpos[v];
    // Buffers are pre-filled with signed 16-bit PCM; no disk reads here.
    int32_t sample = vbuf[v][readIdx];
    float filtered = sample * filterCurrent[v];
    float driven = filtered * driveCurrent[v];
    int32_t effected = (int32_t)driven;

    uint16_t hold = crushCurrentHold[v];
    if (hold > 0) {
      if (crushCountdown[v] == 0) {
        crushLatchedSample[v] = (int16_t)((effected >> crushShift[v]) << crushShift[v]);
        crushCountdown[v] = hold;
      } else {
        crushCountdown[v]--;
      }
      effected = crushLatchedSample[v];
    }

    mix += (int32_t)(effected * vgainCurrent[v]);
    readIdx++;
    if (readIdx >= BUF_SAMPLES) readIdx = 0;
    vpos[v] = readIdx;
    vavailable[v] = avail - 1;
    if ((avail - 1u) == 0u && !voiceStreaming[v]) {
      voiceActive[v] = false;
    }
  }

  if (selfTestRemaining > 0 && selfTestPhaseStep > 0) {
    uint32_t phase = selfTestPhase + selfTestPhaseStep;
    uint32_t remaining = selfTestRemaining;
    int32_t amplitude = SELF_TEST_AMPLITUDE;
    if (remaining < 64u) {
      amplitude = (amplitude * (int32_t)remaining) / 64;
    }
    mix += (phase & 0x80000000u) ? amplitude : -amplitude;
    selfTestPhase = phase;
    remaining--;
    selfTestRemaining = remaining;
    if (remaining == 0) {
      selfTestPhaseStep = 0;
    }
  }

  int32_t out = mix >> 1; // soft gain
  if (out < -2047) out = -2047;
  if (out >  2047) out =  2047;
  uint16_t dac = (uint16_t)(out + 2048); // 0..4095
  writeDacSample(dac);
}

bool AudioEngine::enqueueJob(const Job& job) {
  uint8_t next = (jobHead + 1) % JOB_QUEUE_SIZE;
  if (next == jobTail) {
#if defined(SERIAL_PORT_MONITOR)
    Serial.println(F("AudioEngine: job queue overflow"));
#endif
    return false;
  }
  jobQueue[jobHead] = job;
  jobHead = next;
  return true;
}

bool AudioEngine::popJob(Job& jobOut) {
  if (jobTail == jobHead) {
    return false;
  }
  jobOut = jobQueue[jobTail];
  jobQueue[jobTail] = Job();
  jobTail = (jobTail + 1) % JOB_QUEUE_SIZE;
  return true;
}

void AudioEngine::handleJob(const Job& job) {
  switch (job.type) {
    case JobType::Preload:
      handlePreload(job);
      break;
    case JobType::Fade:
      handleFade(job);
      break;
    case JobType::Diagnostics:
      handleDiagnostics(job);
      break;
    case JobType::FilterSweep:
      handleFilterSweep(job);
      break;
    case JobType::Bitcrush:
      handleBitcrush(job);
      break;
    case JobType::Drive:
      handleDrive(job);
      break;
    case JobType::FxClear:
      handleFxClear(job);
      break;
    case JobType::None:
    default:
      break;
  }
}

void AudioEngine::handlePreload(const Job& job) {
  uint8_t voice = job.voice;
  if (voice >= 4 || !storage) return;

  noInterrupts();
  vavailable[voice] = 0;
  vpos[voice] = 0;
  voiceActive[voice] = false;
  voicePrimed[voice] = false;
  voiceStreaming[voice] = false;
  interrupts();

  vwrite[voice] = 0;
  voiceLoadedSamples[voice] = 0;
  voiceTotalSamples[voice] = 0;
  voiceDiagPending[voice] = false;
  voiceNeedsFadeIn[voice] = true;

  strncpy(voicePath[voice], job.path, MAX_PATH_LEN - 1);
  voicePath[voice][MAX_PATH_LEN - 1] = '\0';

  int32_t total = storage->rawSampleCount(voicePath[voice]);
  if (total <= 0) {
#if defined(SERIAL_PORT_MONITOR)
    Serial.print(F("AudioEngine: missing slice "));
    Serial.println(voicePath[voice]);
#endif
    voiceNeedsFadeIn[voice] = false;
    return;
  }

  voiceTotalSamples[voice] = (uint32_t)total;
  vgainCurrent[voice] = 0.0f;
  armGainRamp(voice, 0.0f, 1);

  voiceStreaming[voice] = true;

  // Try to immediately seed the buffer so playback starts on the very next tick.
  pumpStreams();
}

void AudioEngine::handleFade(const Job& job) {
  uint8_t voice = job.voice;
  if (voice >= 4) return;
  vgainDesired[voice] = job.value;
  armGainRamp(voice, job.value, job.frames ? job.frames : 1);
  if (job.value <= 0.0001f) {
    voiceDraining[voice] = true;
  }
}

void AudioEngine::handleDiagnostics(const Job& job) {
  uint8_t voice = job.voice;
  if (voice >= 4) return;
#if defined(SERIAL_PORT_MONITOR)
  Serial.print(F("[AudioEngine] v"));
  Serial.print(voice);
  Serial.print(F(" active:"));
  Serial.print(voiceActive[voice]);
  Serial.print(F(" streaming:"));
  Serial.print(voiceStreaming[voice]);
  Serial.print(F(" available:"));
  Serial.print((unsigned long)vavailable[voice]);
  Serial.print(F(" loaded:"));
  Serial.print((unsigned long)voiceLoadedSamples[voice]);
  Serial.print(F(" total:"));
  Serial.println((unsigned long)voiceTotalSamples[voice]);
#endif
  voiceDiagPending[voice] = false;
}

void AudioEngine::handleFilterSweep(const Job& job) {
  uint8_t voice = job.voice;
  if (voice >= 4) return;
  uint16_t len = job.frames;
  if (len == 0 || len > FX_TABLE_SIZE) len = FX_TABLE_SIZE;
  float depth = job.value;
  if (depth < 0.0f) depth = 0.0f;
  if (depth > 1.0f) depth = 1.0f;
  for (uint16_t i = 0; i < len; ++i) {
    float t = (float)i / (float)len;
    float lfo = 0.5f - 0.5f * cosf(2.0f * PI * t); // 0..1 raised sine bump
    filterTable[voice][i] = 1.0f - (depth * lfo);
  }
  filterLen[voice] = len;
  filterIndex[voice] = 0;
}

void AudioEngine::handleBitcrush(const Job& job) {
  uint8_t voice = job.voice;
  if (voice >= 4) return;
  uint16_t len = job.frames;
  if (len == 0 || len > FX_TABLE_SIZE) len = FX_TABLE_SIZE;
  uint8_t bits = (job.value <= 0.0f) ? BITCRUSH_DEPTH_BITS : (uint8_t)job.value;
  if (bits > 15) bits = 15;
  if (bits < 4) bits = 4;
  crushShift[voice] = (uint8_t)(16u - bits);
  for (uint16_t i = 0; i < len; ++i) {
    float t = (float)i / (float)len;
    uint16_t hold = 1u + (uint16_t)(8.0f * (0.2f + 0.8f * sinf(PI * t)));
    crushTable[voice][i] = hold;
  }
  crushLen[voice] = len;
  crushIndex[voice] = 0;
  crushCountdown[voice] = 0;
}

void AudioEngine::handleDrive(const Job& job) {
  uint8_t voice = job.voice;
  if (voice >= 4) return;
  uint16_t len = job.frames;
  if (len == 0 || len > FX_TABLE_SIZE) len = FX_TABLE_SIZE;
  float depth = job.value <= 0.0f ? DRIVE_DEPTH_MULT : job.value;
  if (depth < 1.0f) depth = 1.0f;
  for (uint16_t i = 0; i < len; ++i) {
    float t = (float)i / (float)len;
    float rise = 1.0f + (depth - 1.0f) * (0.5f - 0.5f * cosf(2.0f * PI * t));
    driveTable[voice][i] = rise;
  }
  driveLen[voice] = len;
  driveIndex[voice] = 0;
}

void AudioEngine::handleFxClear(const Job& job) {
  (void)job;
  uint8_t voice = job.voice;
  resetFx(voice);
}

void AudioEngine::pumpStreams() {
  if (!storage) return;
  for (uint8_t v = 0; v < 4; ++v) {
    if (!voiceStreaming[v]) continue;

    uint32_t avail;
    noInterrupts();
    avail = vavailable[v];
    interrupts();

    uint32_t freeSpace = BUF_SAMPLES - avail;
    if (freeSpace == 0) {
      continue;
    }

    uint32_t remaining = voiceTotalSamples[v] - voiceLoadedSamples[v];
    if (remaining == 0) {
      voiceStreaming[v] = false;
      continue;
    }

    uint32_t chunk = STREAM_CHUNK;
    if (chunk > remaining) chunk = remaining;
    if (chunk > freeSpace) chunk = freeSpace;
    if (chunk == 0) {
      continue;
    }

    // Split the request if we would wrap the circular buffer.
    uint32_t firstPart = chunk;
    uint32_t spaceToEnd = BUF_SAMPLES - vwrite[v];
    if (firstPart > spaceToEnd) {
      firstPart = spaceToEnd;
    }

    uint32_t totalRead = 0;
    if (firstPart > 0) {
      // Pull the next slice straight from flash into the buffer tail.
      int32_t read1 = storage->readRawChunk(voicePath[v], voiceLoadedSamples[v], &vbuf[v][vwrite[v]], firstPart);
      if (read1 < 0) {
#if defined(SERIAL_PORT_MONITOR)
        Serial.print(F("AudioEngine: read fail "));
        Serial.println(voicePath[v]);
#endif
        voiceStreaming[v] = false;
        continue;
      }
      totalRead += (uint32_t)read1;
      voiceLoadedSamples[v] += (uint32_t)read1;
      vwrite[v] = (vwrite[v] + (uint32_t)read1) % BUF_SAMPLES;
      if ((uint32_t)read1 < firstPart) {
        // Hit EOF early.
        chunk = totalRead;
      }
    }

    if (totalRead < chunk) {
      uint32_t secondPart = chunk - totalRead;
      if (secondPart > 0) {
        // Wrap-around case: finish writing at the head of the ring buffer.
        int32_t read2 = storage->readRawChunk(voicePath[v], voiceLoadedSamples[v], &vbuf[v][vwrite[v]], secondPart);
        if (read2 > 0) {
          totalRead += (uint32_t)read2;
          voiceLoadedSamples[v] += (uint32_t)read2;
          vwrite[v] = (vwrite[v] + (uint32_t)read2) % BUF_SAMPLES;
        }
      }
    }

    if (totalRead > 0) {
      noInterrupts();
      vavailable[v] += totalRead;
      voicePrimed[v] = true;
      voiceActive[v] = true;
      interrupts();

      if (voiceNeedsFadeIn[v]) {
        voiceNeedsFadeIn[v] = false;
        armGainRamp(v, vgainDesired[v], DEFAULT_FADE_FRAMES);
      }
    }

    if (voiceLoadedSamples[v] >= voiceTotalSamples[v]) {
      voiceStreaming[v] = false;
    }
  }
}

void AudioEngine::pumpGains() {
  for (uint8_t v = 0; v < 4; ++v) {
    if (vgainFrames[v] > 0) {
      vgainCurrent[v] += vgainStep[v];
      vgainFrames[v]--;
      if (vgainFrames[v] == 0) {
        vgainCurrent[v] = vgainTarget[v];
        vgainStep[v] = 0.0f;
      }
    } else {
      vgainCurrent[v] = vgainTarget[v];
    }
  }
}

void AudioEngine::pumpEffects() {
  for (uint8_t v = 0; v < 4; ++v) {
    float filt = 1.0f;
    float drv = 1.0f;
    uint16_t crushHold = 0;

    if (filterLen[v] > 0) {
      uint16_t idx = filterIndex[v];
      filt = filterTable[v][idx];
      idx++;
      if (idx >= filterLen[v]) idx = 0;
      filterIndex[v] = idx;
    }

    if (driveLen[v] > 0) {
      uint16_t idx = driveIndex[v];
      drv = driveTable[v][idx];
      idx++;
      if (idx >= driveLen[v]) idx = 0;
      driveIndex[v] = idx;
    }

    if (crushLen[v] > 0) {
      uint16_t idx = crushIndex[v];
      crushHold = crushTable[v][idx];
      idx++;
      if (idx >= crushLen[v]) idx = 0;
      crushIndex[v] = idx;
    }

    noInterrupts();
    filterCurrent[v] = filt;
    driveCurrent[v] = drv;
    crushCurrentHold[v] = crushHold;
    if (crushHold == 0) {
      crushCountdown[v] = 0;
    }
    interrupts();
  }
}

void AudioEngine::cleanupVoice(uint8_t voice) {
  uint32_t avail;
  noInterrupts();
  avail = vavailable[voice];
  interrupts();

  bool streaming = voiceStreaming[voice];
  bool active = voiceActive[voice];
  if (!streaming && !active && avail == 0) {
    if (voicePrimed[voice]) {
      voicePrimed[voice] = false;
    }
    voiceNeedsFadeIn[voice] = false;
    voiceLoadedSamples[voice] = 0;
    voiceTotalSamples[voice] = 0;
    vwrite[voice] = 0;
    vgainCurrent[voice] = 0.0f;
    vgainTarget[voice] = vgainDesired[voice];
    vgainStep[voice] = 0.0f;
    vgainFrames[voice] = 0;
    voiceDraining[voice] = false;
    resetFx(voice);

    noInterrupts();
    vpos[voice] = 0;
    interrupts();
  }
}

void AudioEngine::armGainRamp(uint8_t voice, float target, uint16_t frames) {
  if (voice >= 4) return;
  vgainTarget[voice] = target;
  if (frames == 0) {
    vgainCurrent[voice] = target;
    vgainStep[voice] = 0.0f;
    vgainFrames[voice] = 0;
    return;
  }
  vgainFrames[voice] = frames;
  vgainStep[voice] = (target - vgainCurrent[voice]) / (float)frames;
}

void AudioEngine::resetFx(uint8_t voice) {
  if (voice >= 4) return;
  filterLen[voice] = 0;
  driveLen[voice] = 0;
  crushLen[voice] = 0;
  filterIndex[voice] = 0;
  driveIndex[voice] = 0;
  crushIndex[voice] = 0;
  filterCurrent[voice] = 1.0f;
  driveCurrent[voice] = 1.0f;
  crushCurrentHold[voice] = 0;
  crushCountdown[voice] = 0;
  crushLatchedSample[voice] = 0;
}

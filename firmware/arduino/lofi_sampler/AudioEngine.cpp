#include "AudioEngine.h"
#include "Storage.h"
#include <Adafruit_ZeroTimer.h>
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

namespace {
static constexpr uint16_t DEFAULT_FADE_FRAMES = 96;
static constexpr uint16_t STOP_FADE_FRAMES    = 128;
static constexpr uint32_t STREAM_CHUNK = (AudioEngine::BUF_SAMPLES > 256u)
                                           ? 256u
                                           : ((AudioEngine::BUF_SAMPLES > 64u)
                                                  ? (AudioEngine::BUF_SAMPLES / 2u)
                                                  : AudioEngine::BUF_SAMPLES);
}

bool AudioEngine::begin() {
  analogWriteResolution(12);
  pinMode(DAC_PIN_L, OUTPUT);
  pinMode(DAC_PIN_R, OUTPUT);
  s_self = this;

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
  }

  // Configure ZeroTimer to fire at SAMPLE_RATE_HZ
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

  // Voices that drained out get recycled back to a clean slate.
  for (uint8_t v = 0; v < 4; ++v) {
    cleanupVoice(v);
  }
}

void AudioEngine::onTimerISR(AudioEngine* self) {
  if (self) self->isr();
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
    mix += (int32_t)(sample * vgainCurrent[v]);
    readIdx++;
    if (readIdx >= BUF_SAMPLES) readIdx = 0;
    vpos[v] = readIdx;
    vavailable[v] = avail - 1;
    if ((avail - 1u) == 0u && !voiceStreaming[v]) {
      voiceActive[v] = false;
    }
  }

  int32_t out = mix >> 1; // soft gain
  if (out < -2047) out = -2047;
  if (out >  2047) out =  2047;
  uint16_t dac = (uint16_t)(out + 2048); // 0..4095
  analogWrite(DAC_PIN_L, dac);
  analogWrite(DAC_PIN_R, dac);
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

    noInterrupts();
    vpos[voice] = 0;
    interrupts();

    if (!voiceDiagPending[voice]) {
      voiceDiagPending[voice] = true;
      Job job;
      job.type = JobType::Diagnostics;
      job.voice = voice;
      if (!enqueueJob(job)) {
        voiceDiagPending[voice] = false;
      }
    }
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

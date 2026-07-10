#include "ClockTransport.h"

ClockTransport::ClockTransport(uint8_t stepsPerBarRef, uint8_t clocksPerStepRef, float swingAmountRef)
  : stepsPerBar(stepsPerBarRef),
    clocksPerStep(clocksPerStepRef),
    swingAmount(swingAmountRef) {}

bool ClockTransport::handleRealtime(uint8_t statusByte) {
  switch (statusByte) {
    case 0xF8: { // Timing Clock (24 PPQN)
      if (!playing) {
        // Some hosts/modules emit MIDI clock without an explicit Start.
        // Treat the first clock as a transport start so the sequencer
        // still advances from clock-only sources.
        playing = true;
        midiClockCount = 0;
        stepIndex = 0;
        return true;
      }
      ++midiClockCount;
      uint8_t nextStep = (stepIndex + 1) % stepsPerBar;
      uint8_t clocksNeeded = (uint8_t)(clocksPerStep + swingTicksForStep(nextStep));
      if (midiClockCount >= clocksNeeded) {
        midiClockCount = 0;
        stepIndex = nextStep;
        return true;
      }
      return false;
    }
    case 0xFA: // Start
      playing = true;
      midiClockCount = 0;
      stepIndex = 0;
      return true;
    case 0xFB: // Continue (keep current step)
      playing = true;
      return false;
    case 0xFC: // Stop (freeze UI animation but keep last gates)
      stop();
      return false;
    default:
      return false;
  }
}

void ClockTransport::stop() {
  playing = false;
}

int8_t ClockTransport::swingTicksForStep(uint8_t nextStep) const {
  float swing = clocksPerStep * swingAmount;
  if (swing < 0.0f) swing = 0.0f;
  uint8_t ticks = (uint8_t)(swing + 0.5f);
  if (ticks > clocksPerStep / 2) ticks = clocksPerStep / 2;
  if (ticks == 0) return 0;

  return (nextStep & 0x01) ? (int8_t)ticks : -(int8_t)ticks;
}

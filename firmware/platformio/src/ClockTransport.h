#pragma once

#include <stdint.h>

class ClockTransport {
public:
  ClockTransport(uint8_t stepsPerBar, uint8_t clocksPerStep, float swingAmount);

  // Feed one realtime MIDI status byte (0xF8..0xFC).
  // Returns true only when a new sequencer step is reached.
  bool handleRealtime(uint8_t statusByte);

  void stop();
  bool isPlaying() const { return playing; }
  uint8_t currentStep() const { return stepIndex; }

private:
  uint8_t swingTicksForStep(uint8_t nextStep) const;

  const uint8_t stepsPerBar;
  const uint8_t clocksPerStep;
  const float swingAmount;

  bool playing = false;
  uint8_t stepIndex = 0;
  uint16_t midiClockCount = 0;
};

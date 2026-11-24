#pragma once

#include <stdint.h>

class ClockTransport {
public:
  enum class State : uint8_t {
    Startup,
    Playing,
    ExplicitlyStopped,
    ClockLost,
  };

  ClockTransport(uint8_t stepsPerBar, uint8_t clocksPerStep, float swingAmount);

  // Feed one realtime MIDI status byte (0xF8..0xFC).
  // Returns true only when a new sequencer step is reached.
  bool handleRealtime(uint8_t statusByte);

  void stop();
  void clockLost();
  bool isPlaying() const { return transportState == State::Playing; }
  State state() const { return transportState; }
  uint8_t currentStep() const { return stepIndex; }

private:
  int8_t swingTicksForStep(uint8_t nextStep) const;

  const uint8_t stepsPerBar;
  const uint8_t clocksPerStep;
  const float swingAmount;

  State transportState = State::Startup;
  uint8_t stepIndex = 0;
  uint16_t midiClockCount = 0;
};

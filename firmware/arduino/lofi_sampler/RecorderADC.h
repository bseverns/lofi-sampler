
#pragma once
#include <Arduino.h>
#include "Config.h"

class RecorderADC {
public:
  bool begin();
  void setInputPin(int pin) { analogPin = pin; }
  void start();
  uint32_t stop(); // returns samples recorded
  bool isRecording() const { return rec; }
  // call frequently during recording; returns samples currently in buffer
  uint32_t service();
  const int16_t* data() const { return buf; }
  // Expose a writable view so reslice routines can reuse the capture buffer as
  // scratch RAM once recording is idle (no extra heap grab on SAMD51).
  int16_t* mutableData() { return buf; }

private:
  volatile bool rec = false;
  int analogPin = ANALOG_IN_PIN;
  static const uint32_t CAP = MAX_RECORD_SAMPLES;
  int16_t* buf = nullptr;
  uint32_t idx = 0;
  uint32_t nextMicros = 0;
};

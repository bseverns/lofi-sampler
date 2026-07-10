#pragma once

#include <cstdint>
#include <cstdlib>

static const uint8_t A0 = 29;
static const uint8_t A1 = 30;
static const uint8_t A5 = 34;

enum eAnalogReference {
  AR_DEFAULT = 0,
};

void analogReadResolution(int bits);
void analogReference(eAnalogReference mode);
uint32_t micros();
int analogRead(int pin);
void noInterrupts();
void interrupts();

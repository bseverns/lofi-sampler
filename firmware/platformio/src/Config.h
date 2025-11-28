
#pragma once

// ---------- Core timing ----------
// USB Clock is 24 PPQN so every 12 clocks = one of our 8 steps per bar.
static const uint32_t SAMPLE_RATE_HZ   = 22050;
static const uint8_t  STEPS_PER_BAR    = 8;
static const uint8_t  BEATS_PER_BAR    = 4;
static const uint8_t  MIDI_PPQN        = 24;         // USB MIDI Clock
static const uint8_t  CLOCKS_PER_STEP  = (MIDI_PPQN * BEATS_PER_BAR) / STEPS_PER_BAR; // 12

// ---------- Recording ----------
// 2.6 s ≈ 114.7 KB capture + 57.3 KB of voice buffers ≈ 172.0 KB audio RAM
//5.2 s = 229.4 KB capture + 114.6 KB of voice buffers = 344.0 KB audio RAM
static const float    MAX_RECORD_SECONDS = 5.2f;
static const uint32_t MAX_RECORD_SAMPLES = (uint32_t)(SAMPLE_RATE_HZ * MAX_RECORD_SECONDS);

// ---------- Pins ----------
// Feed your Audio Input Circuit into ANALOG_IN_PIN (default A5) and plug phones into DAC A0/A1.
#define DAC_PIN_L      A0
#define DAC_PIN_R      A1
#define ANALOG_IN_PIN  A5   // set to TRRS-mic ADC channel if you prefer headset input

// ---------- UI columns ----------
static const uint8_t COL_ALT   = 6;  // column index 0..7 (7th column as Alt)
static const uint8_t COL_SHIFT = 7;  // 8th column as Shift

// ---------- Colors (RGB 0-255) ----------
struct RGB { uint8_t r,g,b; };
static const RGB ROW_COLOR[4] = {
  {180, 30, 30},   // A
  {20, 180, 40},   // B
  {20, 110, 200},  // C
  {200, 120, 20}   // D
};
static const float BRIGHT_OFF = 0.06f;
static const float BRIGHT_ON  = 0.35f;
static const float BRIGHT_STEP= 0.7f;

// ---------- Storage ----------
#define FS_LABEL       "NTM4"
#define PATH_A         "/A"
#define PATH_B         "/B"
#define PATH_C         "/C"
#define PATH_D         "/D"

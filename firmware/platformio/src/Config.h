
#pragma once

// ---------- Core timing ----------
// USB Clock is 24 PPQN so every 12 clocks = one of our 8 steps per bar.
static const uint32_t SAMPLE_RATE_HZ   = 22050;
static const uint8_t  STEPS_PER_BAR    = 8;
static const uint8_t  BEATS_PER_BAR    = 4;
static const uint8_t  MIDI_PPQN        = 24;         // USB MIDI Clock
static const uint8_t  CLOCKS_PER_STEP  = (MIDI_PPQN * BEATS_PER_BAR) / STEPS_PER_BAR; // 12
// Swing shifts the even-numbered steps (2/4/6) later by this fraction of a 24 PPQN tick bucket.
static const float    GLOBAL_SWING_AMOUNT = 0.15f;   // 0.0f = straight, 0.25f ≈ "classic" MPC push

// Per-step expressive defaults
static const uint8_t  STEP_DEFAULT_VELOCITY    = 108; // generous but leaves headroom for accents
static const uint8_t  STEP_DEFAULT_PROBABILITY = 100; // percent chance a lit gate will fire

// ---------- FX (live performance macros) ----------
// Filter sweeps ride a gentle sine LFO between 1.0 and (1.0 - depth) over the table length.
static const float    FILTER_SWEEP_DEPTH      = 0.35f;
static const uint16_t FILTER_SWEEP_TABLE_SIZE = 96;   // how many service() ticks in one cycle

// Bitcrush keeps a simple S+H gate and a bit-mask precomputed so the ISR only has to mask + hold.
static const uint8_t  BITCRUSH_DEPTH_BITS     = 8;    // 16-bit PCM target depth (8 = classic crunchy)
static const uint16_t BITCRUSH_RATE_TABLE     = 48;   // how many hold durations to pre-bake per trigger

// Drive cheats a soft clip by precomputing a multiplier ramp; we keep it mild so it stays musical.
static const float    DRIVE_DEPTH_MULT        = 1.4f; // >1.0 boosts into a soft knee
static const uint16_t DRIVE_SWELL_TABLE       = 64;   // service() ticks before the swell loops

// ---------- Recording ----------
// Keep test captures short enough to preserve SRAM headroom while still
// allowing musically useful one-bar phrases at faster tempos.
static const float    MAX_RECORD_SECONDS = 2.0f;
static const uint32_t MAX_RECORD_SAMPLES = (uint32_t)(SAMPLE_RATE_HZ * MAX_RECORD_SECONDS);

// ---------- Pins ----------
// Feed your Audio Input Circuit into ANALOG_IN_PIN (default A5) and plug phones into DAC A0/A1.
#define DAC_PIN_L      A0
#define DAC_PIN_R      A1
#define ANALOG_IN_PIN  A5   // set to TRRS-mic ADC channel if you prefer headset input

// ---------- UI columns ----------
// Modifier pads live on the row below the track they control (wrapping D→A)
// so the main grid keeps all eight columns free for steps.
static const uint8_t MOD_ROW_OFFSET = 1;  // how far below the owning row the modifier latch lives
static const uint8_t COL_ALT        = 6;  // column index 0..7 (7th column as Alt)
static const uint8_t COL_SHIFT      = 7;  // 8th column as Shift

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

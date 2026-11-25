#pragma once
#include <stdint.h>

namespace Ntm4PerformanceSet {
struct Step {
  bool gate;
  uint8_t velocity;
  uint8_t probability;
};
static constexpr char kSetId[] = "bundled-demo";
static constexpr char kSetVersion[] = "0";
static constexpr Step kInitialPattern[4][8] = {};
}  // namespace Ntm4PerformanceSet

#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/lofi-sampler-host-tests"
mkdir -p "${BUILD_DIR}"

if [[ -n "${CXX:-}" ]]; then
  read -r -a CXX_CMD <<< "${CXX}"
elif [[ "$(uname -s)" == "Darwin" ]] && arch -arm64 /usr/bin/true 2>/dev/null; then
  CXX_CMD=(arch -arm64 c++)
else
  CXX_CMD=(c++)
fi

echo "Compiling AudioEngine queue host tests..."
"${CXX_CMD[@]}" -std=c++17 -Wall -Wextra -pedantic \
  "${ROOT_DIR}/tests/audio_engine_queue_test.cpp" \
  -DAUDIO_ENGINE_HEADER=\"${ROOT_DIR}/firmware/platformio/src/AudioEngine.h\" \
  -o "${BUILD_DIR}/audio_engine_queue_test"

echo "Running AudioEngine queue host tests..."
"${BUILD_DIR}/audio_engine_queue_test"

echo "Compiling ClockTransport host tests..."
"${CXX_CMD[@]}" -std=c++17 -Wall -Wextra -pedantic \
  "${ROOT_DIR}/tests/clock_transport_test.cpp" \
  "${ROOT_DIR}/firmware/platformio/src/ClockTransport.cpp" \
  -I"${ROOT_DIR}/firmware/platformio/src" \
  -o "${BUILD_DIR}/clock_transport_test"

echo "Running ClockTransport host tests..."
"${BUILD_DIR}/clock_transport_test"

echo "Compiling PadActionRouter host tests..."
"${CXX_CMD[@]}" -std=c++17 -Wall -Wextra -pedantic \
  -DPAD_ROUTER_HOST_TEST \
  "${ROOT_DIR}/tests/pad_action_router_test.cpp" \
  "${ROOT_DIR}/firmware/platformio/src/PadActionRouter.cpp" \
  -I"${ROOT_DIR}/tests/fakes" \
  -I"${ROOT_DIR}/firmware/platformio/src" \
  -o "${BUILD_DIR}/pad_action_router_test"

echo "Running PadActionRouter host tests..."
"${BUILD_DIR}/pad_action_router_test"

echo "Compiling RecorderADC host tests..."
"${CXX_CMD[@]}" -std=c++17 -Wall -Wextra -pedantic \
  -DRECORDER_ADC_HOST_TEST \
  "${ROOT_DIR}/tests/recorder_adc_test.cpp" \
  "${ROOT_DIR}/firmware/platformio/src/RecorderADC.cpp" \
  -I"${ROOT_DIR}/tests/fakes" \
  -I"${ROOT_DIR}/firmware/platformio/src" \
  -o "${BUILD_DIR}/recorder_adc_test"

echo "Running RecorderADC host tests..."
"${BUILD_DIR}/recorder_adc_test"

echo "Validating filesystem contract..."
python3 "${ROOT_DIR}/scripts/validate_filesystem_contract.py"

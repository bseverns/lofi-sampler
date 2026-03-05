#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/lofi-sampler-host-tests"
mkdir -p "${BUILD_DIR}"

echo "Compiling ClockTransport host tests..."
c++ -std=c++17 -Wall -Wextra -pedantic \
  "${ROOT_DIR}/tests/clock_transport_test.cpp" \
  "${ROOT_DIR}/firmware/platformio/src/ClockTransport.cpp" \
  -I"${ROOT_DIR}/firmware/platformio/src" \
  -o "${BUILD_DIR}/clock_transport_test"

echo "Running ClockTransport host tests..."
"${BUILD_DIR}/clock_transport_test"

echo "Compiling PadActionRouter host tests..."
c++ -std=c++17 -Wall -Wextra -pedantic \
  -DPAD_ROUTER_HOST_TEST \
  "${ROOT_DIR}/tests/pad_action_router_test.cpp" \
  "${ROOT_DIR}/firmware/platformio/src/PadActionRouter.cpp" \
  -I"${ROOT_DIR}/tests/fakes" \
  -I"${ROOT_DIR}/firmware/platformio/src" \
  -o "${BUILD_DIR}/pad_action_router_test"

echo "Running PadActionRouter host tests..."
"${BUILD_DIR}/pad_action_router_test"

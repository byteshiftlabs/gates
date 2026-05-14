#!/usr/bin/env bash
# Helper to configure (if needed), build, and run all tests with verbose output.
set -euo pipefail
BUILD_DIR="build"

cmake -S . -B "$BUILD_DIR" -DENABLE_TESTING=ON
cmake --build "$BUILD_DIR" --target gates_tests -j "${JOBS:-4}"
# Run discovered tests (gtest_discover_tests creates them at build time)
ctest --test-dir "$BUILD_DIR" --output-on-failure "$@"

#!/usr/bin/env bash

set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
JOBS="${JOBS:-4}"
SMOKE_DIR="${BUILD_DIR}/smoke"
SMOKE_INPUTS=(
  examples/ci_validate.c
  examples/example.c
  examples/function_calls.c
  examples/struct_example.c
)

cmake -S . -B "$BUILD_DIR" -DENABLE_TESTING=ON
cmake --build "$BUILD_DIR" --target gates_tests -j "$JOBS"
ctest --test-dir "$BUILD_DIR" --output-on-failure
cmake --build "$BUILD_DIR" --target gates -j "$JOBS"

rm -rf "$SMOKE_DIR"
mkdir -p "$SMOKE_DIR"

for input_file in "${SMOKE_INPUTS[@]}"; do
  output_file="${SMOKE_DIR}/$(basename "${input_file%.c}").vhdl"
  "./${BUILD_DIR}/gates" "$input_file" "$output_file"
  test -s "$output_file"
done

./build_docs.sh
cmake --build "$BUILD_DIR" --target cppcheck -j "$JOBS"
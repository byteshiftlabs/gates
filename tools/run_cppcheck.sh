#!/usr/bin/env bash
# Run cppcheck static analysis on compi source code
# Exit code 0 = clean, 1 = issues found
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v cppcheck &>/dev/null; then
    echo "Error: cppcheck not found. Install with: sudo apt install cppcheck"
    exit 1
fi

cppcheck \
    --enable=all \
    --std=c11 \
    --inconclusive \
    --force \
    --error-exitcode=1 \
    --suppressions-list=tools/cppcheck_suppressions.txt \
    -I include \
    -I src \
    src/ 2>&1

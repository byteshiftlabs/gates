#!/usr/bin/env bash
# run.sh - Run the already-built 'gates' executable
#
# Does not configure or build; use build.sh first. Locates the executable in
# the given (or default) build directory and passes through all arguments.
#
# Usage:
#   ./run.sh [--build-dir DIR] [-- <program arguments>]
# Options:
#       --build-dir DIR  Build directory to look in (default: ./build)
#   -h, --help           Show help
#
# Examples:
#   ./run.sh -- examples/example.c output.vhdl
#   ./run.sh --build-dir build-debug -- examples/example.c output.vhdl
#
set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
PROGRAM_ARGS=()

color() { local code="$1"; shift; if [[ -t 1 ]]; then printf "\e[%sm%s\e[0m" "$code" "$*"; else printf "%s" "$*"; fi; }
info()  { echo "$(color 36 [INFO]) $*"; }
err()   { echo "$(color 31 [ERR ]) $*" >&2; }

show_help() { sed -n '1,/^set -euo/p' "$0" | sed '/^set -euo/q'; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) shift; BUILD_DIR="$(realpath -m "${1:-}")";;
    --build-dir=*) BUILD_DIR="$(realpath -m "${1#*=}")";;
    -h|--help) show_help; exit 0;;
    --) shift; PROGRAM_ARGS+=("$@"); break;;
    *) PROGRAM_ARGS+=("$1");;
  esac
  shift || true
done

EXEC_PATH="$BUILD_DIR/gates"
if [[ ! -x "$EXEC_PATH" ]]; then
  err "Executable not found: $EXEC_PATH"
  err "Run ./build.sh first (or pass --build-dir to match where you built)."
  exit 1
fi

info "Running: $EXEC_PATH ${PROGRAM_ARGS[*]:-}"
"$EXEC_PATH" "${PROGRAM_ARGS[@]}"
EXIT_CODE=$?
info "Program exited with code $EXIT_CODE"
exit $EXIT_CODE

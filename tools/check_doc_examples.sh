#!/usr/bin/env bash
# Regenerate VHDL from every examples/doc/*.c file with the current gates
# build and fail if the output no longer matches the checked-in .vhdl file
# used by docs/source/examples.rst (via literalinclude), or no longer
# analyzes cleanly with GHDL (when available).
#
# Usage: check_doc_examples.sh <path-to-gates-binary> [path-to-ghdl]
set -euo pipefail

cd "$(dirname "$0")/.."

GATES_BIN="${1:?usage: check_doc_examples.sh <path-to-gates-binary> [path-to-ghdl]}"
GHDL_BIN="${2:-}"

if [[ ! -x "$GATES_BIN" ]]; then
    echo "Error: gates binary not found or not executable: $GATES_BIN"
    exit 1
fi

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

status=0

for c_file in examples/doc/*.c; do
    name="$(basename "$c_file" .c)"
    golden="examples/doc/${name}.vhdl"
    regenerated="${WORKDIR}/${name}.vhdl"

    if [[ ! -f "$golden" ]]; then
        echo "Error: no golden file for $c_file (expected $golden)"
        status=1
        continue
    fi

    if ! "$GATES_BIN" "$c_file" "$regenerated" >/dev/null 2>&1; then
        echo "Error: gates failed to compile $c_file"
        status=1
        continue
    fi

    if ! diff -u "$golden" "$regenerated"; then
        echo "Error: $golden no longer matches gates' output for $c_file"
        echo "       (docs/source/examples.rst includes $golden verbatim;"
        echo "       regenerate it from the current build and update the doc"
        echo "       prose too if the output changed on purpose)"
        status=1
        continue
    fi

    if [[ -n "$GHDL_BIN" ]]; then
        analysis_dir="${WORKDIR}/${name}_analysis"
        mkdir -p "$analysis_dir"
        if ! "$GHDL_BIN" -a --std=93 --work=work --workdir="$analysis_dir" \
                "$golden" >/dev/null 2>&1; then
            echo "Error: $golden no longer analyzes cleanly with GHDL"
            status=1
            continue
        fi
    fi

    echo "OK: $c_file -> $golden"
done

exit $status

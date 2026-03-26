# Gates Audit Findings

## Executive Summary
- Total open blockers: 0
- Total open serious issues: 0
- Total open minor issues: 0
- Overall recommendation: Release now

This ledger reflects the current `main` branch after all prior fixes plus the
VHDL type-correctness overhaul, per-entity library preamble fix, structural
validation test suite, and Node 20 deprecation fix (actions/checkout@v5).

## Findings Table
| ID | Severity | Confidence | Status | File | Issue | Verification |
|----|----------|------------|--------|------|-------|--------------|
| B1 | Blocker | confirmed | fixed | src/codegen/codegen_vhdl_statements.c, tests/integration/integration_tests.cpp | Comparison-valued returns now lower to numeric 0/1 assignments instead of a raw boolean result-port assignment | targeted regression test, full `ctest`, generated example output |
| S1 | Serious | confirmed | fixed | src/codegen/codegen_vhdl_types.c | Array initializer bit emission no longer shifts out of range for widths above the host integer width | `cmake --build <build-dir> --target cppcheck` |
| B2 | Blocker | confirmed | fixed | src/codegen/codegen_vhdl_types.c, tests/integration/integration_tests.cpp, examples/example.c | Nested loop-local variables are now hoisted and reset correctly, so shipped examples no longer emit undeclared inner loop signals | generated example output, targeted regression test, full `ctest` |
| M1 | Minor | confirmed | fixed | README.md, docs/source/index.rst, docs/source/examples.rst, examples/function_calls.c | Public-facing function-call wording now matches the documented limitation that inter-entity wiring is not yet synthesized | targeted doc review, `./build_docs.sh` |
| B3 | Blocker | confirmed | fixed | src/codegen/codegen_vhdl_expressions.c, src/codegen/codegen_vhdl_main.c | Numeric literals emitted as bare `c_N` identifiers instead of `to_unsigned(N,32)`; arithmetic/bitwise ops returned bare `unsigned` instead of `std_logic_vector`; library/use clauses only emitted once instead of per design unit | 5 structural validation tests, full `ctest` (81/81), CI run 23533019794 |
| M2 | Minor | confirmed | fixed | .github/workflows/ci.yml | actions/checkout@v4 triggers Node 20 deprecation warning | CI run 23533019794 (v5, no warning) |
| M3 | Minor | confirmed | fixed | docs/source/known_issues.rst | Known issues claimed no automated VHDL validation despite structural tests now existing | `./build_docs.sh` |

## Detailed Findings

### Blockers
#### B1
- Location: src/codegen/codegen_vhdl_expressions.c, src/codegen/codegen_vhdl_statements.c, tests/integration/integration_tests.cpp
- Confidence: confirmed
- Status: fixed
- Problem: Comparison expressions were generated as boolean predicates, but return statements assigned them directly to the `result` port, which is a `std_logic_vector`.
- Fix: Boolean-valued returns now emit an `if/else` that assigns `std_logic_vector(to_unsigned(1, 32))` or `std_logic_vector(to_unsigned(0, 32))`.
- Verification: Regression coverage exists in `EndToEndTest.ComparisonReturnProducesNumericResultAssignment`, and the generated example output was manually rechecked.

#### B2
- Location: src/codegen/codegen_vhdl_types.c, tests/integration/integration_tests.cpp, examples/example.c
- Confidence: confirmed
- Status: fixed
- Problem: Local-signal discovery and reset generation only scanned one level of the function body. Variables declared inside nested control-flow blocks were used in generated VHDL without a corresponding signal declaration.
- Fix: Replaced the shallow scan with a recursive subtree walk. Added regression coverage in `EndToEndTest.NestedLoopVariablesAreDeclaredAndReset`.
- Verification: Full `ctest` passes (81/81).

#### B3
- Location: src/codegen/codegen_vhdl_expressions.c, src/codegen/codegen_vhdl_main.c
- Confidence: confirmed
- Status: fixed
- Problem: Three related type-correctness issues in generated VHDL:
  1. Numeric literals (e.g. `int x = 42;`) emitted as `c_42` — a sanitized identifier with no declaration — instead of `std_logic_vector(to_unsigned(42, 32))`.
  2. Arithmetic and bitwise expression results were `unsigned` type but assigned to `std_logic_vector` signals without wrapping.
  3. Library/use clauses were emitted once at the top of the file instead of before each design unit, which is required by the VHDL standard.
- Fix: Added `is_numeric_literal()` / `is_negative_numeric_literal()` checks in `generate_expression()` to emit properly wrapped literals. Wrapped all bitwise and arithmetic fallback results in `std_logic_vector()`. Moved library preamble emission into `emit_entity_declaration()`.
- Verification: 5 new structural validation tests verify no bare `c_N` identifiers, balanced constructs, per-entity library preamble, declared signals, and proper type wrapping. Full `ctest` passes (81/81). CI run 23533019794 green.

### Serious
#### S1
- Location: src/codegen/codegen_vhdl_types.c
- Confidence: confirmed
- Status: fixed
- Problem: `cppcheck` failed with `shiftTooManyBits` on the array initializer bit emission logic.
- Fix: High-bit extraction is now guarded.
- Verification: `cppcheck` target passes.

### Minor
#### M1
- Location: README.md, docs/source/index.rst, docs/source/examples.rst, examples/function_calls.c
- Confidence: confirmed
- Status: fixed
- Problem: Function-call support described too broadly.
- Fix: Narrowed to self-contained functions, added limitation notes.
- Verification: Doc review and rebuild.

#### M2
- Location: .github/workflows/ci.yml
- Confidence: confirmed
- Status: fixed
- Problem: `actions/checkout@v4` uses Node 20, which is deprecated and will become a hard failure.
- Fix: Upgraded to `actions/checkout@v5`.
- Verification: CI run 23533019794 completes without deprecation warning.

#### M3
- Location: docs/source/known_issues.rst
- Confidence: confirmed
- Status: fixed
- Problem: Known issues still claimed "automated simulator verification is not yet part of the regular test flow" despite structural validation tests now existing.
- Fix: Updated wording to "Structural well-formedness is verified by self-contained validation tests; external simulator verification is planned for Phase 5."
- Verification: `./build_docs.sh` passes.

## Coverage Matrix
| Surface | Files / Modules | Status | Notes |
|---------|------------------|--------|-------|
| Build and packaging | CMakeLists.txt, run_tests.sh, build_docs.sh, build_and_run.sh | reviewed | Build, tests, docs, version smoke, and cppcheck exercised locally and in CI |
| CLI entrypoint | src/app/gates.c | reviewed | Manual CLI runs for example translation and version output |
| Parser core | src/parser/*.c, token.c | reviewed | Control-flow lowering rechecked against nested loop cases |
| Codegen core | src/codegen/*.c | reviewed | Type-correctness overhaul verified by 5 structural tests |
| Error handling | src/error_handler.c | reviewed | No material issues |
| Core and symbol utilities | src/core/*.c, src/symbols/*.c | reviewed | No material issues |
| Tests | tests/integration/integration_tests.cpp, tests/unit/*.cpp | reviewed | Full suite passes (81/81); structural validation tests added |
| Documentation | README.md, docs/source/**, CONTRIBUTING.md, ROADMAP.md | reviewed | All public claims verified against actual behavior |
| Static analysis | cppcheck target | reviewed | Passes |
| Generated VHDL validity | examples/ci_validate.c, examples/example.c | reviewed | Structural validation tests cover type wrapping, balanced constructs, signal declarations, library preambles |
| CI / automation | .github/workflows/ci.yml | reviewed | checkout@v5, 81 tests, cppcheck, docs all pass |

## Unreviewed Or Uncertain Areas
None.

## Release Hardening Follow-Ups
None required for this release.

## Merge / Release Recommendation
- Merge now / Do not merge: Merge now
- Release now / Do not release: Release now
- Preconditions:
  1. None.

## Ordered Fix Plan
All items from the previous audit pass have been addressed. No remaining action items.
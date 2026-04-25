# Gates Audit Findings

## Executive Summary
- Total open blockers: 0
- Total open serious issues: 0
- Total open minor issues: 0
- Overall recommendation: Release now

This ledger reflects the current `main` branch after the GoogleTest FetchContent fix,
the cppcheck follow-up, the nested-loop local-signal fix, and the public-doc scope
alignment for function-call limitations.

## Findings Table
| ID | Severity | Confidence | Status | File | Issue | Verification |
|----|----------|------------|--------|------|-------|--------------|
| B1 | Blocker | confirmed | fixed | src/codegen/codegen_vhdl_statements.c, tests/integration/integration_tests.cpp | Comparison-valued returns now lower to numeric 0/1 assignments instead of a raw boolean result-port assignment | targeted regression test, full `ctest`, generated example output |
| S1 | Serious | confirmed | fixed | src/codegen/codegen_vhdl_types.c | Array initializer bit emission no longer shifts out of range for widths above the host integer width | `cmake --build <build-dir> --target cppcheck` |
| B2 | Blocker | confirmed | fixed | src/codegen/codegen_vhdl_types.c, tests/integration/integration_tests.cpp, examples/example.c | Nested loop-local variables are now hoisted and reset correctly, so shipped examples no longer emit undeclared inner loop signals | generated example output, targeted regression test, full `ctest` |
| M1 | Minor | confirmed | fixed | README.md, docs/source/index.rst, docs/source/examples.rst, examples/function_calls.c | Public-facing function-call wording now matches the documented limitation that inter-entity wiring is not yet synthesized | targeted doc review, `./build_docs.sh` |

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
- Problem: Local-signal discovery and reset generation only scanned one level of the function body. Variables declared inside nested control-flow blocks, such as the inner `j` in nested `while` and `for` loops, were used in generated VHDL without a corresponding signal declaration.
- Fix: Replaced the shallow scan with a recursive subtree walk for local-signal declarations and reset emission. Added regression coverage in `EndToEndTest.NestedLoopVariablesAreDeclaredAndReset`.
- Verification: The shipped example now generates VHDL containing `signal j : std_logic_vector(31 downto 0);` and `j <= (others => '0');` for both `while_nested_loop` and `for_loop_sum`. Full `ctest` passes (`76/76`).

### Serious
#### S1
- Location: src/codegen/codegen_vhdl_types.c
- Confidence: confirmed
- Status: fixed
- Problem: `cppcheck` failed with `shiftTooManyBits` on the array initializer bit emission logic when `bit_position` exceeded the width of `unsigned int`.
- Fix: High-bit extraction is now guarded; positions above the host integer width no longer shift out of range and negative values are sign-extended instead.
- Verification: The `cppcheck` build target passes locally and in CI.

### Minor
#### M1
- Location: README.md, docs/source/index.rst, docs/source/examples.rst, examples/function_calls.c
- Confidence: confirmed
- Status: fixed
- Problem: Public-facing docs and examples described function-call support too broadly even though cross-function hardware wiring is still a documented limitation.
- Fix: Narrowed the feature wording to self-contained functions, added an explicit limitation note in the examples docs, and marked the shipped `function_calls.c` sample as parser/codegen coverage rather than a supported hardware-composition example.
- Verification: Reviewed the updated public docs against `docs/source/known_issues.rst` and rebuilt the docs.

## Coverage Matrix
| Surface | Files / Modules | Status | Notes |
|---------|------------------|--------|-------|
| Build and packaging | CMakeLists.txt, run_tests.sh, build_docs.sh, build_and_run.sh | reviewed | Build, tests, docs, version smoke, and cppcheck exercised locally and in CI |
| CLI entrypoint | src/app/gates.c | reviewed | Manual CLI runs performed for example translation and version output |
| Parser core | src/parser/parse.c, parse_expression.c, parse_struct.c, parse_function.c, parse_statement.c, parse_control_flow.c, parse_for.c, token.c | reviewed | Control-flow lowering rechecked against nested loop cases |
| Codegen core | src/codegen/codegen_vhdl_main.c, codegen_vhdl_expressions.c, codegen_vhdl_statements.c, codegen_vhdl_types.c | reviewed | Blockers and static-analysis issues fixed and revalidated |
| Error handling | src/error_handler.c | reviewed | No new material issues found in the audited release pass |
| Core and symbol utilities | src/core/utils.c, src/core/astnode.c, src/symbols/symbol_structs.c, src/symbols/symbol_arrays.c | reviewed | No new material issues found in the audited release pass |
| Tests | tests/integration/integration_tests.cpp, tests/unit/basic_tests.cpp, tests/unit/edge_case_tests.cpp, tests/unit/test_error_handler.cpp | reviewed | Full suite passes; nested-loop regression added |
| Documentation | README.md, docs/source/**, CONTRIBUTING.md, ROADMAP.md | partially reviewed | Docs now scope function support to self-contained cases; simulator validation remains documented as a limitation |
| Static analysis | cppcheck target | reviewed | Passes |
| Generated VHDL validity | CLI output from examples/example.c | reviewed | Shipped example manually rechecked after nested-loop fix |
| CI / automation | .github/workflows/ci.yml | reviewed | Latest CI run covers configure, tests, docs, version smoke, and cppcheck |

## Unreviewed Or Uncertain Areas
- Surface: Automated VHDL simulator validation
- Why unreviewed or uncertain: The project still does not run an external VHDL parser or simulator as part of the regular test or CI flow.
- Required follow-up: Optional for this release because the limitation is documented, but adding a syntax-level validation step would materially strengthen the release gate.

## Release Hardening Follow-Ups
- Add a lightweight VHDL syntax-validation step for shipped examples in CI.
- Refresh GitHub Actions dependencies before the Node 20 deprecation window becomes a hard failure.

## Merge / Release Recommendation
- Merge now / Do not merge: Merge now
- Release now / Do not release: Release now
- Preconditions:
  1. None from this audit pass.

## Ordered Fix Plan
1. Add syntax-only VHDL validation for shipped examples in CI.
2. Refresh GitHub Actions dependencies affected by the Node 20 deprecation notice.
3. Expand automated coverage for additional documented language limitations as the supported subset grows.
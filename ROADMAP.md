# Gates Roadmap

Planned features and milestones for Gates, in the order they should be tackled.

Phases are ordered by dependency, not preference. Correctness work precedes new
language features, because every new construct multiplies the surface area of an
unfixed codegen bug.

---

## Phase 0: Release Blockers ✅ (one item remaining)

Found by a pre-publication audit. Fixed across PRs #18–#36, verified against
the current codebase during a second publication audit.

### Memory safety and crashes — fixed

- Stack buffer overflow copying array names and indices (PR #19)
- NULL dereference in codegen on empty conditions — `if ()` and `while ()` crashed (PR #22)
- Recursion depth limits added for the parser; bounds the AST that codegen and
  `free_node` walk (PR #23)
- The lexer's per-comment recursion in `get_next_token()` converted to a loop (PR #23)
- `capacity` restored after a failed `realloc` in `add_child()` (PR #28)

### Correct VHDL output — fixed

The structural test suite asserted on emitted text and therefore missed all of
these; each produced output a VHDL analyzer rejects.

- Duplicate `signal` declaration for every initialized array (PR #24)
- Struct record types emitted at file scope, outside any design unit (PR #30)
- Struct field writes emitted an undeclared flat signal — `p_x` vs `p.x` (PR #31)
- Operands bypassed signal mapping — reads of the `result` out port, undeclared
  `p__x` (PR #25, #31)
- A parameter named `result` produced a duplicate port name (PR #25)
- VHDL reserved words used as C identifiers were emitted unquoted (PR #25)
- `return f(x);` was dropped, leaving `result` undriven (PR #33)
- Conditions bypassed expression generation — `arr[0]` and `unsigned(1)` leaked
  through (PR #32)

### Diagnostics and contract — fixed

- Exit status now reflects code generation errors (PR #18)
- Parenthesized low-precedence operators no longer rejected by the parser (PR #20)
- Output file is no longer truncated until compilation succeeds (PR #26)
- A directory or unreadable file as input is now rejected instead of reporting
  success (PR #26)
- Identifiers over 127 characters are now rejected instead of silently
  truncated (PR #34)
- Trailing garbage after an initializer is now rejected instead of discarded (PR #27)
- Multi-declarator declarations (`int x = 1, y = 2;`) are now rejected instead
  of silently dropping every declarator after the first (PR #27)
- `for`-init backtracking desync fixed (PR #35)
- `GATES_MAX_ARRAYS` / `GATES_MAX_STRUCTS` wired up to the symbol table (PR #29)
- Partially built nodes are now freed on parser error paths (PR #36)

### Remaining

- [ ] Array names over 63 characters are still silently truncated in the
      symbol table, which registers by truncated name but is looked up by
      exact match elsewhere — bounds checking silently no-ops instead of
      becoming fatal. See `docs/source/known_issues.rst`.

---

## Phase 1: Core Language Support ✅

- Tokenizer and recursive-descent parser with full expression precedence
- Control flow: `if/else`, `while`, `for`, `break`, `continue`
- Function calls and return value propagation
- Structs with field access; array declarations and indexing
- VHDL entity/architecture generation with clock/reset, signals, and synchronous processes
- Multi-level error diagnostics with source locations and colored output
- GoogleTest unit, integration, structural, edge case, and CLI coverage
- Sphinx documentation (architecture, internals, usage, testing)

---

## Phase 2: Language Completeness

### Control flow
- [ ] `switch/case` — maps directly to VHDL `case`
- [ ] `do-while` — VHDL loop with the exit condition at the bottom

### Pointers
- [ ] Address-of (`&`) and dereference (`*`) in expressions
- [ ] Pointer arithmetic for array traversal

### Data types
- [ ] `TOKEN_CHAR_LITERAL` for single-quoted characters
- [ ] `%` (modulo) — currently fails with a misleading syntax error
- [ ] Reject string literals explicitly; they have no VHDL equivalent

### Global variables
- [ ] Parse declarations outside functions
- [ ] Distinguish global and local scope in the symbol table
- [ ] Emit globals as architecture-level signals
- [ ] Handle cross-function reads and writes

### Advanced structs
- [ ] Nested struct definitions
- [ ] Arrays of structs
- [ ] Whole-struct assignment with `=`

---

## Phase 3: Verification Infrastructure

Behavioral verification is the missing half of the proof bar. The current suite
checks emitted text, not whether that text analyzes.

- [ ] **Behavioural verification of generated VHDL** — route the emitted output through
      the in-house simulator once that path is ready. This is the single highest-value
      addition; it would have caught every Phase 0 codegen defect.
- [ ] Code coverage reporting (gcov/lcov) to find untested paths
- [ ] Fuzz testing for parser and codegen robustness (AFL or libFuzzer)
- [ ] Benchmark suite for larger, realistic inputs

---

## Phase 4: Complete VHDL Generation

- [ ] Unique, hierarchical signal naming across scopes
- [ ] Temporary signal reduction for simple expressions
- [ ] Function-to-entity mapping strategy (entities, components, or processes)
- [ ] Inter-function signal wiring for call chains
- [ ] Parser error recovery (panic mode) so multiple errors are reported per run
- [ ] Testbench generation alongside each entity

---

## Phase 5: VHDL Optimization

Only once Phase 4 output is correct and complete.

- [ ] Resource sharing for repeated operations
- [ ] Pipeline stage insertion
- [ ] Constant folding and propagation
- [ ] Dead code elimination

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) and the Sphinx docs for architecture details.

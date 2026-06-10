# Gates Roadmap

Planned features and milestones for Gates, in the order they should be tackled.

Phases are ordered by dependency, not preference. Correctness work precedes new
language features, because every new construct multiplies the surface area of an
unfixed codegen bug.

---

## Phase 0: Release Blockers (Current)

Found by a pre-publication audit. These block a public release and come before
any Phase 2 feature work.

### Memory safety and crashes

- [ ] Stack buffer overflow copying array names and indices — fix open in PR #19
- [ ] NULL dereference in codegen on empty conditions — `if ()` and `while ()` crash
- [ ] Recursion depth limits for the parser, code generator, and `free_node`
- [ ] Convert the lexer's per-comment recursion in `get_next_token()` to a loop
- [ ] Restore `capacity` after a failed `realloc` in `add_child()`

### Correct VHDL output

The structural test suite asserts on emitted text and therefore missed all of
these. Each produces output a VHDL analyzer rejects.

- [ ] Duplicate `signal` declaration for every initialized array
- [ ] Struct record types emitted at file scope, outside any design unit
- [ ] Struct field writes emit an undeclared flat signal (`p_x` vs `p.x`)
- [ ] Operands bypass signal mapping — reads of the `result` out port, undeclared `p__x`
- [ ] A parameter named `result` produces a duplicate port name
- [ ] VHDL reserved words used as C identifiers are emitted unquoted
- [ ] `return f(x);` is dropped, leaving `result` undriven
- [ ] Conditions bypass expression generation — `arr[0]` and `unsigned(1)` leak through

### Diagnostics and contract

- [ ] Exit status must reflect code generation errors — fix open in PR #18
- [ ] Parenthesized low-precedence operators rejected by the parser — fix open in PR #20
- [ ] Do not truncate the output file until compilation succeeds
- [ ] Reject a directory or unreadable file as input instead of reporting success
- [ ] Make silent truncations fatal: identifiers over 127 chars, array names over 63
- [ ] Reject, rather than discard, trailing garbage after an initializer
- [ ] Parse multi-declarator declarations (`int x = 1, y = 2;`) or reject them
- [ ] Fix `for`-init backtracking leaving the parser desynchronized
- [ ] Wire up `GATES_MAX_ARRAYS` / `GATES_MAX_STRUCTS`, or remove them from the documented knobs
- [ ] Free partially built nodes on parser error paths

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

- [ ] **VHDL analysis in CI** — run generated output through GHDL. This is the
      single highest-value addition; it would have caught every Phase 0 codegen defect.
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

# Gates Roadmap

This document outlines the planned features, improvements, and milestones for Gates.

---

## Phase 1: Core Language Support (Current)

### ✅ Completed
- Tokenizer and recursive-descent parser
- Full expression precedence handling
- Control flow: `if/else`, `while`, `for`, `break`, `continue`
- Function calls in all contexts
- Return value propagation (expression returns, struct field-by-field copy, negative literals)
- Basic struct support with field access
- Array declarations and indexing
- VHDL entity/architecture generation with clock/reset, signals, and synchronous processes
- Multi-level error diagnostics (error/warning/note × 5 categories)
- Source location tracking with line/column reporting
- Colored terminal output (ANSI: red errors, yellow warnings, blue notes)
- Unit, integration, and edge case test suite (62 tests, GoogleTest)
- Signal naming collision handling (`emit_mapped_signal_name()` with `_local` suffix, PR #12)

### 📋 Planned
- Sphinx documentation content (infrastructure exists, content needed)

---

## Phase 2: Language Completeness

### Control Flow
- [ ] **`switch/case` statement**
  Maps directly to VHDL `case` — one of the most common C constructs, conspicuously missing.
- [ ] **`do-while` loop**
  Trivial extension of existing `while` support. Maps to VHDL loop with exit condition at bottom.

### Pointer Support
- [ ] **Address-of operator (`&`)**
  Tokenizer and parser support for unary `&`. Codegen maps to signal references.
- [ ] **Pointer dereference (`*`)**
  Parser support for unary `*` in expressions. Extend `parse_identifier()` dispatcher (already structured for this).
- [ ] **Pointer arithmetic for array traversal**
  Support `ptr + offset` patterns that map to array indexing in VHDL.

### Data Types
- [ ] **Character literal tokenization**
  Add `TOKEN_CHAR_LITERAL` to the tokenizer. Currently single-quoted characters have no dedicated token type.
- [ ] **String literal exclusion (explicit)**
  Strings have no VHDL equivalent. Document this as an intentional non-goal and emit a clear error if encountered.

### Global Variables
- [ ] **Parser support for global declarations**
  Recognize variable definitions outside functions and store them in the AST.
- [ ] **Symbol table scoping updates**
  Distinguish global vs local scope so codegen knows where to place signals.
- [ ] **VHDL signal generation at architecture level**
  Emit globals as architecture-level signals visible to all processes.
- [ ] **Cross-function access patterns**
  Handle reads/writes to globals from multiple functions without conflicts.

### Advanced Structs
- [ ] **Nested struct definitions**
  Allow structs containing other structs as fields (e.g., `struct A { struct B inner; }`).
- [ ] **Arrays of structs**
  Support declaring and indexing arrays where each element is a struct.
- [ ] **Struct assignment operations**
  Enable copying entire structs with `=` instead of field-by-field assignment.

---

## Phase 3: Testing Infrastructure

Code coverage and fuzz testing must come *before* adding complex features — you need coverage data to know where the gaps are, and fuzz testing catches parser bugs that haunt you later.

- [ ] **Code coverage reporting**
  Track which lines/branches are exercised by tests to find gaps. Use gcov/lcov with CMake.
- [ ] **Fuzz testing for parser robustness**
  Feed random/malformed inputs to catch crashes and edge cases. AFL or libFuzzer.
- [ ] **VHDL simulation verification**
  Run generated VHDL through a simulator (GHDL/ModelSim) to validate behavior.
- [ ] **Benchmark suite for complex inputs**
  Measure compile time and output quality on realistic, larger programs.

---

## Phase 4: Correct & Complete VHDL Generation

Before optimizing anything, the generated VHDL must be correct and complete for all supported C constructs.

### Signal Management
- [ ] **Unique naming scheme for all contexts**
  Prevent collisions when the same variable name appears in different scopes.
- [ ] **Hierarchical signal prefixing**
  Prefix signals with function/block names for clarity in generated VHDL.
- [ ] **Temporary signal reduction**
  Minimize intermediate signals by inlining simple expressions.

### Multi-Function Support
- [ ] **Function-to-entity mapping strategy**
  Define how multiple C functions map to VHDL: separate entities, component instantiation, or processes.
- [ ] **Inter-function signal wiring**
  Connect output ports of one entity to input ports of another for function call chains.

### Parser Robustness
- [ ] **Error recovery (panic mode)**
  Synchronize to the next statement boundary after a syntax error instead of aborting. Makes the tool usable on real code with typos.

### Testbench Generation
- [ ] **Basic testbench output**
  Generate a VHDL testbench alongside each entity for quick simulation verification.

---

## Phase 5: VHDL Optimization

Only after Phase 4 produces correct, complete output.

- [ ] **Resource sharing for repeated operations**
  Reuse adders/multipliers across different expressions to reduce hardware.
- [ ] **Pipeline stage insertion**
  Automatically add registers between combinational stages to meet timing.
- [ ] **Constant folding and propagation**
  Evaluate compile-time constants and replace variables with known values.
- [ ] **Dead code elimination**
  Remove signals and logic that have no effect on outputs.

---

## Contributing

See the full documentation for contribution guidelines and architecture details.

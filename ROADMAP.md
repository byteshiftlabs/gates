# Compi Roadmap

This document outlines the planned features, improvements, and milestones for Compi.

> **Note:** This roadmap was initially generated with AI assistance and will be reviewed and refined by the development team as priorities evolve.

---

## Phase 1: Core Language Support (Current)

### ✅ Completed
- Tokenizer and recursive-descent parser
- Full expression precedence handling
- Control flow: `if/else`, `while`, `for`, `break`, `continue`
- Function calls in all contexts
- Basic struct support with field access
- Array declarations and indexing
- VHDL entity/architecture generation
- Multi-level error diagnostics
- Unit test suite (35 tests)

### 🔄 In Progress
- Signal naming collision handling
- Documentation improvements

---

## Phase 2: Data Structures

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
- [ ] **Struct return types from functions**
  Allow functions to return structs, generating appropriate VHDL signal bundles.

---

## Phase 3: Code Generation Improvements

### VHDL Optimization
- [ ] **Resource sharing for repeated operations**
  Reuse adders/multipliers across different expressions to reduce hardware.
- [ ] **Pipeline stage insertion**
  Automatically add registers between combinational stages to meet timing.
- [ ] **Constant folding and propagation**
  Evaluate compile-time constants and replace variables with known values.
- [ ] **Dead code elimination**
  Remove signals and logic that have no effect on outputs.

### Signal Management
- [ ] **Unique naming scheme for all contexts**
  Prevent collisions when the same variable name appears in different scopes.
- [ ] **Hierarchical signal prefixing**
  Prefix signals with function/block names for clarity in generated VHDL.
- [ ] **Temporary signal reduction**
  Minimize intermediate signals by inlining simple expressions.

---

## Phase 4: Diagnostics & Developer Experience

- [ ] **Source location tracking improvements**
  Report exact line/column for errors, pointing to the problematic token.
- [ ] **Colored terminal output**
  Use ANSI colors to highlight errors (red), warnings (yellow), and notes (blue).
- [ ] **Warning levels (error, warning, note)**
  Categorize messages by severity so users can filter or treat warnings as errors.

---

## Phase 5: Testing & Validation

- [ ] **Integration test suite**
  End-to-end tests that compile C files and verify the generated VHDL.
- [ ] **Code coverage reporting**
  Track which lines/branches are exercised by tests to find gaps.
- [ ] **VHDL simulation verification**
  Run generated VHDL through a simulator (GHDL/ModelSim) to validate behavior.
- [ ] **Benchmark suite for complex inputs**
  Measure compile time and output quality on realistic, larger programs.
- [ ] **Fuzz testing for parser robustness**
  Feed random/malformed inputs to catch crashes and edge cases.


---

## Contributing

See the full documentation for contribution guidelines and architecture details.

# Compi: C-to-VHDL Compiler

Minimal C subset → VHDL

## 🚧 Known Limitations & Issues
* Global variables not yet implemented
* No C short-circuit evaluation for logical operators
* Basic VHDL optimization (no resource tuning)
* Limited struct support: no nested structs or arrays of structs yet

## 🗺️ Roadmap
Global variables, nested structs, arrays of structs, enhanced diagnostics, VHDL optimizations. See full docs for details.

## Key Features
* **Parser**: Tokenizer, recursive-descent parser, AST builder
* **Expressions**: Full precedence (arithmetic, shifts, bitwise, comparison, logical, unary)
* **Control Flow**: `if/else`, `while`, `for`, `break`, `continue`
* **Functions**: Calls in expressions, statements, conditions, and nested
* **Data Types**: Structs (declarations, field access, initializers) and arrays
* **Code Generation**: VHDL entity/architecture with signal mapping
* **Error Handling**: Multi-level diagnostics with source context, hints, and suggestions
* **Testing**: 35 GoogleTest unit tests via CTest
* **Documentation**: Comprehensive Sphinx docs

## 🛠️ Quick Start

```bash
git clone https://github.com/cmelnu/compi.git
cd compi && mkdir build && cd build
cmake .. && make
./compi input.c output.vhdl
```

**Debug Mode** (verbose output):
```bash
cmake -DDEBUG=ON .. && make
```

## 🧪 Testing

```bash
./run_tests.sh              # Quick: build + run all tests
./build_docs.sh             # Build Sphinx documentation
```

**Advanced**:
```bash
cmake -S . -B build -DENABLE_TESTING=ON
cmake --build build --target test_all -j 4
ctest --test-dir build --output-on-failure
./build/compi_tests --gtest_filter=ErrorHandlerTest.*  # Run specific tests
```

## 🗂️ Project Structure

```
src/        — Source code (parser, lexer, codegen, error handler)
examples/   — Example C files and demos
docs/       — Sphinx documentation
tests/      — GoogleTest unit tests
build/      — Build output (generated)
```

## 🚧 Known Issues
* Global variables not yet implemented
* Logical ops don’t model C short‑circuit timing
* No optimization / resource tuning in VHDL output
* Potential signal naming collisions
* Structs: no support for nested structs or arrays of structs yet

## ✅ Operator Coverage (Summary)
Arithmetic, shifts, bitwise, comparisons, logical (no short‑circuit semantics), unary minus / logical not, control flow + arrays.

## 🗺️ Roadmap
Next focus areas: global vars, nested structs, arrays of structs, better diagnostics, naming & optimization improvements, integration & coverage tests. Full details: see docs (architecture / roadmap sections).
# Compi

Minimal C subset → VHDL translator

## Quick Start

```bash
git clone https://github.com/cmelnulabs/compi.git
cd compi && mkdir build && cd build
cmake .. && make
./compi input.c output.vhdl
```

## Features

- Recursive-descent parser with full operator precedence
- Control flow: `if/else`, `while`, `for`, `break`, `continue`
- Functions, structs, and arrays
- VHDL entity/architecture generation
- Multi-level error diagnostics
- 35 unit tests (GoogleTest)

## Testing

```bash
./run_tests.sh    # Build and run all tests
./build_docs.sh   # Build documentation
```

## Project Structure

```
src/       — Parser, lexer, codegen, error handler
tests/     — GoogleTest unit tests
docs/      — Sphinx documentation
examples/  — Sample C files
```

## Known Limitations

- No global variables
- No nested structs or arrays of structs
- Basic VHDL output (no optimization)

## Documentation

See [ROADMAP.md](ROADMAP.md) for planned features and [docs/](docs/) for full documentation.

# Contributing to Compi

We welcome contributions to the C-to-VHDL compiler project.

## How to Contribute

1. Fork the repository and create a new branch for your feature or bugfix
2. Write clear, modular code and document your changes
3. Add tests or example files to demonstrate new features
4. Submit a pull request with a description of your changes

## Branch Naming

Branch names follow the pattern `[label]/[brief-description]`:

- `feature/global-variables`
- `bugfix/parser-null-pointer`
- `code-refactor/remove-magic-numbers`
- `docs-refactor/api-reference`
- `tests/codegen-coverage`

## Commit Messages

Use past participle, be brief and clear:

```
Added user authentication module
Fixed null pointer exception in parser
Renamed ambiguous variables, removed magic numbers
```

## Coding Guidelines

- Code compiles with `-Wall -Wextra -Wpedantic` and zero warnings
- All tests pass before submitting (`./run_tests.sh`)
- Follow the established patterns and conventions in the codebase
- No file exceeds 500 lines
- No magic numbers — use `#define` or named constants
- No code duplication — extract shared logic into reusable functions

See [docs/source/code_quality.rst](docs/source/code_quality.rst) for detailed coding standards.

## Building and Testing

```bash
mkdir build && cd build
cmake .. -DENABLE_TESTING=ON
make -j$(nproc)
ctest --output-on-failure
```

## Reporting Issues

- Report bugs or feature requests via GitHub Issues
- Include steps to reproduce, expected behavior, and relevant code snippets

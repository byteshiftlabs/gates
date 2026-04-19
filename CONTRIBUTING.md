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

## Code Quality

All contributions must meet these standards before merging:

- GNU coding standards compliance (https://www.gnu.org/prep/standards/)
- Includes organized: stdlib → third-party → local
- Clear, descriptive naming for structs, functions, and variables
- No magic numbers — use `#define` or `const`
- No hardcoded values in function calls
- Prefer string functions over regex where possible
- Consistent formatting and structure throughout
- No dead code or unused imports
- No code duplication — extract shared logic into reusable functions (3+ occurrences)
- No file exceeds 500 lines
- Complex logic has comments explaining "why", not "what"
- Code compiles with `-Wall -Wextra -Wpedantic` and zero warnings
- All tests pass before submitting (`./run_tests.sh`)

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

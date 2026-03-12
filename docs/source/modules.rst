Modules
=======

This project is organized into modular source files, each responsible for a key part of the C-to-VHDL compilation process:

.. contents:: Source Code Overview
    :depth: 2
    :local:

src/app/gates.c
---------------
The main entry point for the compiler. Handles command-line arguments, file I/O, and orchestrates the parsing and code generation process. It calls the parser to build the AST and then invokes the VHDL generator.

src/core/astnode.c / include/astnode.h
--------------------------------------
Defines and implements the AST node structure and management functions.

- Provides the data structures for AST nodes used throughout the compiler.
- Functions for creating, freeing, and manipulating AST nodes.
- Used by the parser and code generator to represent the program structure.

Parser Module (src/parser/)
---------------------------
Implements the recursive-descent parser and AST construction, split into focused sub-modules:

- **parse.c / parse.h** — Top-level parser entry point (``parse_program``). Dispatches to specialized sub-parsers.
- **parse_expression.c / parse_expression.h** — Expression parsing with full operator precedence and associativity.
- **parse_statement.c / parse_statement.h** — Variable declarations, assignments, return statements, and expression statements.
- **parse_control_flow.c / parse_control_flow.h** — Control flow: ``if/else``, ``while``, ``for``, ``break``, ``continue``.
- **parse_function.c / parse_function.h** — Function declarations and parameter lists.
- **parse_struct.c / parse_struct.h** — Struct definitions and field registration.
- **token.c / token.h** — Lexical analyzer (tokenizer). Breaks input into tokens, tracks line numbers, handles identifier truncation.

Codegen Module (src/codegen/)
-----------------------------
Generates VHDL code from the AST, organized into six files:

- **codegen_vhdl_main.c** — Top-level VHDL generation entry point (``generate_vhdl``). Orchestrates entity/architecture output.
- **codegen_vhdl_constants.c/h** — Centralized string and buffer size constants for the codegen module.
- **codegen_vhdl_helpers.c/h** — Utility functions: signal name mapping, type checking, numeric literal detection, array parsing.
- **codegen_vhdl_types.c/h** — Struct type declarations, signal declarations, array helpers.
- **codegen_vhdl_expressions.c/h** — Expression generation (binary, unary, function calls).
- **codegen_vhdl_statements.c/h** — Statement generation (assignments, loops, if/else, return).

src/core/utils.c / include/utils.h
-----------------------------------
Shared utility functions: safe string operations (``safe_append``, ``safe_copy``, ``safe_strdup``), AST printing, operator precedence, and numeric detection helpers.

src/error_handler.c / include/error_handler.h
----------------------------------------------
Multi-level error diagnostics with categories, error codes, and location tracking. Provides ``log_info``, ``log_warning``, and ``log_error`` with configurable output.

Symbol Tables (src/symbols/)
----------------------------
- **symbol_arrays.c / symbol_arrays.h** — Array registration and lookup table.
- **symbol_structs.c / symbol_structs.h** — Struct registration and field lookup table.

examples/
---------
Contains example C files for testing parsing and code generation, including while loop and nested loop examples.

CMakeLists.txt
--------------
Build configuration for compiling all source files. Sources are listed explicitly. Supports GoogleTest via ``find_package`` or ``FetchContent``, developer debug output via ``-DDEBUG=ON``, and a Sphinx docs target.

tests/
------
Contains GoogleTest-based C++ test files organized into subdirectories:

**Unit Tests** (``tests/unit/``):

- **basic_tests.cpp** — Core component tests: AST nodes, utilities, tokens, symbol tables
- **edge_case_tests.cpp** — Boundary conditions and robustness tests
- **test_error_handler.cpp** — Error handler module tests

**Integration Tests** (``tests/integration/``):

- **integration_tests.cpp** — Component boundary tests, end-to-end smoke tests, negative tests, VHDL validation

The build creates a reusable ``gates_gtest`` static library (all C sources except the CLI front-end) so tests link cleanly without invoking the command-line interface. Tests are auto-discovered with ``gtest_discover_tests`` enabling each test case to appear individually in CTest output.

run_tests.sh
------------
Helper script to configure (if needed), build, and run the full test suite with a single command.



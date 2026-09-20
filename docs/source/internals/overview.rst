Architecture Overview
=====================

System Architecture
-------------------

The Gates compiler is structured as a multi-stage pipeline:

.. code-block:: text

   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌──────────────┐
   │   C Source  │ ──> │    Lexer    │ ──> │   Parser    │ ──> │  Symbol      │
   │   (.c file) │     │ (Tokenizer) │     │  (AST)      │     │  Tables      │
   └─────────────┘     └─────────────┘     └─────────────┘     └──────────────┘
                                                                        │
                                                                        ▼
   ┌─────────────┐     ┌─────────────┐                          ┌──────────────┐
   │VHDL Output  │ <── │    VHDL     │ <────────────────────── │  Code Gen    │
   │ (.vhd file) │     │  Generator  │                          │              │
   └─────────────┘     └─────────────┘                          └──────────────┘

Pipeline Stages
---------------

1. **Lexical Analysis (Lexer)**

   - Input: Raw C source code (``FILE*`` stream)
   - Output: Stream of tokens via ``ParserContext``
   - Location: ``src/parser/token.c``, ``include/token.h``, ``src/parser/tokenizer.h``
   - Responsibilities:

     * Scan character by character
     * Recognize keywords, identifiers, literals, operators
     * Skip whitespace and comments
     * Track line numbers for error reporting
     * Provide token stream to parser via ``advance()``, ``match()``, ``consume()``

2. **Syntax Analysis (Parser)**

   - Input: Token stream from lexer
   - Output: Abstract Syntax Tree (AST)
   - Location: ``src/parser/``
   - Components:

     * ``parse.c`` — Top-level driver (program, top-level struct/function dispatch)
     * ``parse_function.c`` — Function parameter and body parsing
     * ``parse_statement.c`` — Statement parsing (variable declarations, assignments, returns)
     * ``parse_expression.c`` — Expression parsing with precedence climbing
     * ``parse_control_flow.c`` — if/else and while, plus break/continue
     * ``parse_for.c`` — for-loop parsing (init/condition/increment; separated from the other control-flow constructs since its header has three clauses instead of one)
     * ``parse_struct.c`` — Struct definition parsing

   - Responsibilities:

     * Validate syntax according to C grammar subset
     * Build AST nodes representing program structure
     * Handle operator precedence and associativity
     * Register structs and arrays in symbol tables during parsing
     * Error reporting through the centralized error handler

3. **Symbol Table Construction**

   - Input: Registration calls during parsing
   - Output: Symbol tables for structs and arrays
   - Location: ``src/symbols/``
   - Components:

     * ``symbol_arrays.c`` — Array symbol tracking (name, size)
     * ``symbol_structs.c`` — Struct type tracking (name, fields)

   - Responsibilities:

     * Track array declarations and sizes
     * Track struct definitions and field types
     * Provide lookup by name for code generation

4. **Code Generation (VHDL)**

   - Input: AST + symbol tables
   - Output: VHDL source code written to ``FILE*``
   - Location: ``src/codegen/`` (split across 7 files)
   - Components:

     * ``codegen_vhdl_main.c`` — Top-level VHDL generation (entity, architecture), node dispatch
     * ``codegen_vhdl_emit.c`` — Module-global output-buffer primitives (``emit_init()``, ``emit_raw()``, ``emit_line()``, ``emit_indent()``) everything else writes through
     * ``codegen_vhdl_expressions.c`` — Expression translation
     * ``codegen_vhdl_statements.c`` — Statement translation (assignments, loops, if/else)
     * ``codegen_vhdl_helpers.c`` — Name mapping, identifier safety utilities
     * ``codegen_vhdl_types.c`` — Type/signal declarations, struct-as-record handling
     * ``codegen_vhdl_constants.c`` — Centralized string constants

   - Responsibilities:

     * Generate VHDL entity declarations
     * Generate VHDL architecture bodies
     * Map C types to VHDL types
     * Translate C expressions and control flow to VHDL
     * Handle arrays, structs, and complex types

Directory Structure
-------------------

.. code-block:: text

   gates/
   ├── CMakeLists.txt          # CMake build configuration
   ├── README.md               # Project overview
   ├── LICENSE                 # GPL v3
   ├── Dockerfile              # Authoritative CI/validation image
   ├── build.sh                # Configure + build
   ├── build_docs.sh           # Documentation build script
   ├── run.sh                  # Build + run gates on a file
   ├── run_tests.sh            # Test runner script
   │
   ├── ci/
   │   └── run_validation.sh   # Full validation: build, ctest, smoke-translate, docs, cppcheck
   │
   ├── include/                # Public header files
   │   ├── astnode.h           # AST node types and structure
   │   ├── codegen_vhdl.h      # VHDL code generator API
   │   ├── config.h            # GATES_MAX_* build-time limits
   │   ├── error_handler.h     # Centralized error/warning reporting
   │   ├── parse.h             # Parser main API
   │   ├── symbol_arrays.h     # Array symbol table API
   │   ├── symbol_structs.h    # Struct symbol table API
   │   ├── token.h             # Token types, Token struct, ParserContext
   │   └── utils.h             # Precedence, string-safety utilities
   │
   ├── src/                    # Source code
   │   ├── error_handler.c     # Error handler implementation
   │   ├── app/                # Application entry point
   │   │   └── gates.c         # Main function
   │   ├── core/               # Core data structures
   │   │   ├── astnode.c       # AST node create/add_child/free
   │   │   └── utils.c         # safe_strdup, precedence table, print_ast, ...
   │   ├── parser/              # Parser components (see :doc:`parser`, :doc:`lexer`)
   │   │   ├── parse.c
   │   │   ├── parse_control_flow.c
   │   │   ├── parse_expression.c
   │   │   ├── parse_for.c
   │   │   ├── parse_function.c
   │   │   ├── parse_statement.c
   │   │   ├── parse_struct.c
   │   │   ├── token.c
   │   │   ├── tokenizer.h            # Internal lexer API
   │   │   └── parse_*.h              # Internal headers, one per .c above
   │   ├── codegen/             # Code generation, 7 .c files (see :doc:`codegen`)
   │   │   ├── codegen_vhdl_main.c
   │   │   ├── codegen_vhdl_emit.c
   │   │   ├── codegen_vhdl_expressions.c
   │   │   ├── codegen_vhdl_statements.c
   │   │   ├── codegen_vhdl_helpers.c
   │   │   ├── codegen_vhdl_types.c
   │   │   ├── codegen_vhdl_constants.c
   │   │   └── (matching internal .h headers)
   │   └── symbols/             # Symbol tables (see :doc:`symbols`)
   │       ├── symbol_arrays.c
   │       └── symbol_structs.c
   │
   ├── examples/                # Example C files
   │   ├── ci_validate.c        # Smoke-translated by ci/run_validation.sh
   │   ├── example.c
   │   ├── function_calls.c
   │   ├── struct_example.c
   │   ├── test_error_handler.c
   │   └── doc/                 # Golden C/VHDL pairs literalinclude'd into examples.rst
   │
   ├── tests/                   # GoogleTest + CTest (see :doc:`../testing`)
   │   ├── unit/
   │   │   ├── basic_tests.cpp
   │   │   ├── edge_case_tests.cpp
   │   │   └── test_error_handler.cpp
   │   ├── integration/
   │   │   └── integration_tests.cpp
   │   └── cli/                 # Fixtures for CTest cases that invoke the gates binary directly
   │       └── too_many_params.c
   │
   ├── tools/                   # Development tools
   │   ├── run_cppcheck.sh
   │   ├── check_doc_examples.sh
   │   └── cppcheck_suppressions.txt
   │
   └── docs/                    # Sphinx documentation
       ├── Makefile
       └── source/
           ├── conf.py
           ├── index.rst
           └── ...

Data Flow
---------

Main Compilation Flow
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

   // Actual flow in gates.c (simplified)
   int main(int argc, char *argv[]) {
       FILE *fin  = fopen(argv[1], "r");
       FILE *fout = fopen(argv[2], "w");

       // Parse: tokenize + build AST in one pass
       ASTNode *program = parse_program(fin);

       if (!program || has_errors()) {
           // Log error count, cleanup, exit
       }

       // Code generation: walk AST, emit VHDL
       generate_vhdl(program, fout);

       free_node(program);
       fclose(fin);
       fclose(fout);
       return 0;
   }

``parse_program()`` internally creates a ``ParserContext``, tokenizes on demand
via ``advance()``/``consume()``, and builds the AST in a single pass.

Parser Data Flow
^^^^^^^^^^^^^^^^

The parser uses a **recursive descent** strategy:

1. ``parse_program()`` initializes a ``ParserContext`` and enters the parsing loop
2. Dispatches to sub-parsers based on the current token:

   - ``parse_struct()`` for struct definitions (and functions returning a struct)
   - ``parse_function()`` for function declarations

   Global variables are **not supported**: a type/identifier pair at top
   level not followed by ``(`` is a parse error, not silently accepted (see
   :doc:`parser`).

3. Each sub-parser builds AST nodes and registers symbols:

   - ``register_struct()`` for struct definitions
   - ``register_array()`` for array declarations

4. Expression parsing uses a precedence climbing algorithm in ``parse_expression_prec()``
5. Errors are reported via ``log_error()`` and propagated via ``return NULL`` + ``has_errors()`` guards

AST Structure
^^^^^^^^^^^^^

AST nodes use a flat, uniform structure:

.. code-block:: c

   typedef struct ASTNode {
       NodeType type;           // NODE_PROGRAM, NODE_FUNCTION_DECL, NODE_IF_STATEMENT, etc.
       Token token;             // Associated token (for type info, source location)
       char *value;             // Dynamically allocated string value (name, operator, etc.)
       struct ASTNode *parent;  // Parent node
       struct ASTNode **children;  // Dynamic array of child nodes
       int num_children;        // Current child count
       int capacity;            // Allocated child array capacity
   } ASTNode;

Nodes are created with ``create_node()``, linked with ``add_child()``, and freed
recursively with ``free_node()``.

Error Handling
--------------

The compiler uses a centralized error reporting system in ``error_handler.h/c``:

.. code-block:: c

   // Error reporting with severity, category, and source location
   void log_error(ErrorCategory category, int line, const char *format, ...);
   void log_warning(ErrorCategory category, int line, const char *format, ...);

   // Error counting for flow control
   int has_errors(void);
   int get_error_count(void);

Parser functions return ``NULL`` on error (after logging via ``log_error``).
Parsing loops check ``has_errors()`` to stop iteration after the first error.
The application (``gates.c``) checks ``has_errors()`` before proceeding to
code generation.

Debug Mode
----------

Debug mode enabled with ``cmake -DDEBUG=ON`` provides:

- AST visualization via ``print_ast()`` before code generation
- Token stream tracing during parsing

Build System
------------

CMake Configuration
^^^^^^^^^^^^^^^^^^^

The project uses CMake and targets Ubuntu 24.04 x86_64:

- Minimum CMake version: 3.14
- C standard: C11
- C++ standard: C++17 (for GoogleTest)
- Compiler flags: ``-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion``
- Debug mode: Add ``-DDEBUG`` definition (when ``-DDEBUG=ON``)
- No optimization flags specified (uses compiler defaults)

Build Targets
^^^^^^^^^^^^^

- ``gates`` — Main compiler executable
- ``gates_tests`` — GoogleTest test executable
- ``cppcheck`` — Run cppcheck static analysis
- ``docs`` — Build Sphinx HTML documentation

Dependencies
------------

External Dependencies
^^^^^^^^^^^^^^^^^^^^^

- **Platform**: Ubuntu 24.04 x86_64
- **Standard C Library**: ``stdio.h``, ``stdlib.h``, ``string.h``, etc.
- **GoogleTest**: Unit testing framework (fetched via CMake FetchContent, v1.14.0, SHA256-pinned)
- **Sphinx**: Documentation generation (Python)
- **cppcheck**: Static analysis
- **CMake**: Build system (>= 3.14)
- **GCC**: C compiler supporting C11 (tested on GCC 13.3.0)

Internal Dependencies
^^^^^^^^^^^^^^^^^^^^^

Module dependency graph (``→`` means "depends on"):

.. code-block:: text

   src/app/gates.c
     → parse.h, codegen_vhdl.h, error_handler.h, utils.h

   src/parser/parse.c
     → tokenizer.h, parse_struct.h, parse_function.h, error_handler.h

   src/parser/parse_*.c
     → tokenizer.h, astnode.h, error_handler.h, utils.h, config.h (for GATES_MAX_PARSE_DEPTH),
       symbol_arrays.h / symbol_structs.h (as needed)

   src/codegen/codegen_vhdl_main.c
     → codegen_vhdl_*.h (internal headers),
       symbol_structs.h, astnode.h

   src/symbols/symbol_*.c
     → symbol_*.h (public), error_handler.h

Testing Strategy
----------------

See :doc:`../testing` for detailed testing documentation.

- Test coverage spans unit, integration, structural validation, and end-to-end scenarios
- GoogleTest framework
- Tests cover: lexer, parser, code generator, error handler, symbol tables
- The suite is intended to be run in both release and debug configurations

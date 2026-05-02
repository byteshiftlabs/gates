Architecture
============

The compiler is structured as a pipeline:

1. **Lexical Analysis (token.c / token.h)**
    - Reads the input C file and produces a stream of tokens.
    - Handles keywords, identifiers, literals, operators, and comments.

2. **Parsing and AST Representation (parse.c / parse.h, astnode.c / astnode.h)**
    - Consumes the token stream and builds an Abstract Syntax Tree (AST).
    - Supports function declarations, variable declarations, assignments, return statements, and control flow (`if`, `else if`, `else`, `while`, `for`, `break`, `continue`).
    - Handles binary and unary expressions, including negative literals and identifiers.
    - Defines node types for all supported C constructs.
    - Provides data structures and functions for creating, freeing, and manipulating AST nodes.
    - Supports nested while loops, nested for loops, and correct handling of break/continue at any loop depth.
    - Supports nested expressions and statement blocks, including nested for and while loops.

3. **VHDL Code Generation (src/codegen/)**
    - Six modular files: ``codegen_vhdl_main.c``, ``codegen_vhdl_constants.c``,
      ``codegen_vhdl_helpers.c``, ``codegen_vhdl_types.c``,
      ``codegen_vhdl_expressions.c``, ``codegen_vhdl_statements.c``.
    - Traverses the AST and emits VHDL code.
    - Maps C types to VHDL types, handles signal declarations, assignments, and control flow.
    - Handles negative values and binary expressions correctly in VHDL.
    - Generates VHDL for while loops and for loops, including nested loops, break, and continue statements.

4. **Utilities (utils.c / utils.h)**
    - Provides string manipulation, memory management, AST printing, and numeric conversion helpers.

The modular design allows for easy extension to new C constructs and VHDL features.

Overview
--------
The compiler is organized into modular components for tokenization, parsing, AST construction, and VHDL code generation.

Main Components
---------------
- **Tokenizer**: Scans C source code and produces tokens for keywords, identifiers, numbers, operators, and punctuation.
- **Parser**: Uses recursive descent to build an AST representing the structure of the C code, including support for nested while loops and control flow.
- **AST**: Abstract Syntax Tree nodes represent functions, variables, assignments, return statements, and control flow constructs.
- **Code Generator**: Traverses the AST to produce VHDL code, mapping C types to VHDL types and handling signal/port declarations, while loops, break, and continue.

Data Flow
---------
1. Input C file is tokenized.
2. Tokens are parsed into an AST.
3. VHDL code is generated from the AST.

Extensibility
-------------
- New C constructs can be supported by adding AST node types and parser logic.
- VHDL codegen can be expanded for more complex hardware descriptions.

Developer Debug Output
----------------------
- Enable verbose debug output for developers by configuring the build with the `-DDEBUG=ON` argument in CMake. This provides additional debug prints and diagnostics during parsing and code generation.

Global State and Threading
--------------------------

The compiler uses global state in several modules to keep the current single-file pipeline simple:

**Error Handler** (``error_handler.c``):

- ``error_count``, ``warning_count``: Track diagnostic counts per compilation.
- **Rationale**: Avoids passing context through every function call in the parser and codegen. The compiler processes one file at a time in a single-threaded pipeline.
- **Reset**: Call ``reset_error_state()`` between compilations if embedding in a tool.

**Symbol Tables** (``symbol_structs.c``, ``symbol_arrays.c``):

- Global tables for struct definitions and array size tracking.
- **Rationale**: Struct types and array sizes are needed during code generation but are discovered during parsing. A global table keeps that information available without threading additional context through the AST.
- **Limitation**: Not thread-safe. For parallel compilation, each thread would need isolated symbol tables.
- **Reset**: Call ``reset_struct_table()`` and ``reset_array_table()`` between compilations.

**Future Consideration**: If multi-threaded compilation is needed, these modules should be refactored to use a context struct passed through the call chain.


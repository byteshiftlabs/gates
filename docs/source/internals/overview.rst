Architecture Overview
=====================

System Architecture
-------------------

The Compi compiler is structured as a multi-stage pipeline:

.. code-block:: text

   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌──────────────┐
   │   C Source  │ ──> │    Lexer    │ ──> │   Parser    │ ──> │  Symbol      │
   │   (.c file) │     │ (Tokenizer) │     │  (AST)      │     │  Tables      │
   └─────────────┘     └─────────────┘     └─────────────┘     └──────────────┘
                                                                        │
                                                                        ▼
   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌──────────────┐
   │VHDL Output  │ <── │    VHDL     │ <── │   Type      │ <── │  Semantic    │
   │ (.vhd file) │     │  Generator  │     │  Checking   │     │  Analysis    │
   └─────────────┘     └─────────────┘     └─────────────┘     └──────────────┘

Pipeline Stages
---------------

1. **Lexical Analysis (Lexer)**
   
   - Input: Raw C source code string
   - Output: Stream of tokens
   - Location: ``src/core/lexer.c``
   - Responsibilities:
     
     * Scan character by character
     * Recognize keywords, identifiers, literals, operators
     * Skip whitespace and comments
     * Track line/column numbers for error reporting
     * Build token stream for parser

2. **Syntax Analysis (Parser)**

   - Input: Token stream from lexer
   - Output: Abstract Syntax Tree (AST)
   - Location: ``src/parser/``
   - Components:
     
     * ``parse.c`` - Main parser driver
     * ``parse_function.c`` - Function declaration parsing
     * ``parse_statement.c`` - Statement parsing (if/while/for/return/etc)
     * ``parse_expression.c`` - Expression parsing with precedence climbing
     * ``parse_struct.c`` - Struct definition parsing
   
   - Responsibilities:
     
     * Validate syntax according to C grammar subset
     * Build AST nodes representing program structure
     * Handle operator precedence and associativity
     * Parse declarations, statements, expressions
     * Error recovery and reporting

3. **Symbol Table Construction**

   - Input: AST nodes
   - Output: Symbol tables with scope information
   - Location: ``src/symbols/``
   - Components:
     
     * ``symbol_table.c`` - Main symbol table implementation
     * ``symbol_arrays.c`` - Array symbol tracking
     * ``symbol_structs.c`` - Struct type tracking
   
   - Responsibilities:
     
     * Track variables, functions, types across scopes
     * Resolve symbol references
     * Detect redeclarations and undefined symbols
     * Maintain type information for checking

4. **Semantic Analysis & Type Checking**

   - Input: AST + Symbol tables
   - Output: Validated, type-annotated AST
   - Location: Integrated with parser
   - Responsibilities:
     
     * Verify type compatibility in expressions
     * Check function call signatures
     * Validate array access bounds (where possible)
     * Ensure control flow statements are valid
     * Type coercion rules (C promotion rules)

5. **Code Generation (VHDL)**

   - Input: Validated AST
   - Output: VHDL source code
   - Location: ``src/codegen/codegen_vhdl.c``
   - Responsibilities:
     
     * Generate VHDL entity declarations
     * Generate VHDL architecture bodies
     * Map C types to VHDL types
     * Translate C expressions to VHDL
     * Translate C control flow to VHDL processes
     * Handle arrays, structs, and complex types

Directory Structure
-------------------

.. code-block:: text

   compi/
   ├── CMakeLists.txt          # CMake build configuration
   ├── README.md               # Project overview
   ├── LICENSE                 # License file
   ├── build_and_run.sh        # Build and run script
   ├── build_docs.sh           # Documentation build script
   ├── run_tests.sh            # Test runner script
   │
   ├── include/                # Public header files
   │   ├── astnode.h           # AST node structures
   │   ├── codegen_vhdl.h      # VHDL code generator API
   │   ├── parse.h             # Parser main API
   │   ├── parse_expression.h  # Expression parser API
   │   ├── parse_function.h    # Function parser API
   │   ├── parse_statement.h   # Statement parser API
   │   ├── parse_struct.h      # Struct parser API
   │   ├── symbol_arrays.h     # Array symbol table API
   │   ├── symbol_structs.h    # Struct symbol table API
   │   ├── token.h             # Token definitions
   │   └── utils.h             # Utility functions
   │
   ├── src/                    # Source code
   │   ├── app/                # Application entry point
   │   │   └── main.c          # Main function
   │   ├── core/               # Core compiler components
   │   │   └── lexer.c         # Lexical analyzer
   │   ├── parser/             # Parser components
   │   │   ├── parse.c         # Main parser driver
   │   │   ├── parse_expression.c
   │   │   ├── parse_function.c
   │   │   ├── parse_statement.c
   │   │   └── parse_struct.c
   │   ├── codegen/            # Code generation
   │   │   └── codegen_vhdl.c
   │   └── symbols/            # Symbol tables
   │       ├── symbol_arrays.c
   │       └── symbol_structs.c
   │
   ├── examples/               # Example C files for testing
   │   ├── example.c
   │   └── struct_example.c
   │
   ├── tests/                  # Unit tests
   │   └── ...
   │
   └── docs/                   # Sphinx documentation
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

   // Pseudo-code representation of main compilation flow
   
   int main(int argc, char **argv) {
       // 1. Read input C source file
       char *source_code = read_file(argv[1]);
       
       // 2. Lexical analysis - tokenize
       Token *tokens = tokenize(source_code);
       
       // 3. Parse - build AST
       ASTNode *ast_root = parse(tokens);
       
       // 4. Semantic analysis (integrated with parsing)
       // - Symbol tables built during parsing
       // - Type checking performed during AST construction
       
       // 5. Code generation - emit VHDL
       char *vhdl_output = codegen_vhdl(ast_root);
       
       // 6. Write output VHDL file
       write_file(output_file, vhdl_output);
       
       return 0;
   }

Parser Data Flow
^^^^^^^^^^^^^^^^

The parser uses a **recursive descent** strategy:

1. Start with ``parse_program()``
2. Parse global declarations and function definitions
3. For each function:
   
   - Parse function signature (return type, name, parameters)
   - Parse function body (compound statement)
   - Recursively parse statements inside body

4. For each statement:
   
   - Detect statement type (if/while/for/return/assignment/etc)
   - Call appropriate statement parser
   - Build AST node for statement
   - Attach to parent AST node

5. For each expression:
   
   - Use precedence climbing algorithm
   - Parse operators with proper precedence and associativity
   - Build expression AST subtree
   - Return expression node to statement parser

AST Construction
^^^^^^^^^^^^^^^^

AST nodes are allocated dynamically and linked in a tree structure:

.. code-block:: c

   typedef struct ASTNode {
       ASTNodeType type;        // NODE_FUNCTION, NODE_IF, NODE_BINOP, etc.
       
       union {
           // Different data for different node types
           struct {
               char *name;
               ASTNode *params;
               ASTNode *body;
           } function;
           
           struct {
               ASTNode *condition;
               ASTNode *then_branch;
               ASTNode *else_branch;
           } if_stmt;
           
           struct {
               TokenType operator;
               ASTNode *left;
               ASTNode *right;
           } binop;
           
           // ... more node type variants
       } data;
       
       // Source location for error reporting
       int line;
       int column;
   } ASTNode;


Error Handling
--------------

Error Reporting Mechanism
^^^^^^^^^^^^^^^^^^^^^^^^^

The compiler uses a centralized error reporting system:

.. code-block:: c

   void parser_error(const char *format, ...) {
       fprintf(stderr, "Parse error at line %d: ", current_line);
       va_list args;
       va_start(args, format);
       vfprintf(stderr, format, args);
       va_end(args);
       fprintf(stderr, "\n");
       exit(1);  // Abort compilation
   }

Error Recovery
^^^^^^^^^^^^^^

- Currently: Fail-fast approach (abort on first error)
- Future: Implement error recovery to report multiple errors

Debug Mode
----------

Debug mode enabled with ``cmake -DDEBUG=ON`` provides:

- Verbose token stream dumps
- AST visualization before code generation
- Parser state transitions
- Symbol table contents
- Code generation intermediate representations

Build System
------------

CMake Configuration
^^^^^^^^^^^^^^^^^^^

The project uses CMake for cross-platform builds:

- Minimum CMake version: 3.10
- C standard: C11
- Compiler flags: ``-Wall -Wextra -std=c11``
- Debug flags: ``-g -O0`` (when ``-DDEBUG=ON``)
- Release flags: ``-O3`` (default)

Build Targets
^^^^^^^^^^^^^

- ``compi`` - Main compiler executable
- ``test`` - Run unit tests (if TESTING=ON)
- ``docs`` - Build Sphinx HTML documentation

Dependencies
------------

External Dependencies
^^^^^^^^^^^^^^^^^^^^^

- **Standard C Library**: ``stdio.h``, ``stdlib.h``, ``string.h``, etc.
- **Sphinx**: Documentation generation (Python package)
- **CMake**: Build system (>= 3.10)
- **GCC/Clang**: C compiler for building compi itself

Internal Dependencies
^^^^^^^^^^^^^^^^^^^^^

Module dependency graph:

.. code-block:: text

   main.c
     └─> parse.h
           ├─> parse_function.h
           │     ├─> parse_statement.h
           │     │     ├─> parse_expression.h
           │     │     │     └─> token.h
           │     │     └─> token.h
           │     ├─> parse_expression.h
           │     └─> symbol_arrays.h
           ├─> parse_struct.h
           ├─> codegen_vhdl.h
           │     ├─> astnode.h
           │     └─> symbol_arrays.h
           └─> astnode.h


Testing Strategy
----------------

See :doc:`../testing` for detailed testing documentation.

**Unit Tests**:

- Lexer tests: Token recognition
- Parser tests: AST construction
- Code generator tests: VHDL output validation

**Integration Tests**:

- End-to-end: C file → VHDL file
- Compare expected vs actual VHDL output

**Test Coverage**:

- Goal: >80% code coverage
- Use ``gcov`` for coverage reporting

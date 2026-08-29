Code Quality & Refactoring
===========================

Overview
--------

This document describes the coding standards contributions must meet (mirroring ``CONTRIBUTING.md``) and the organizational patterns the current codebase follows, verified against the source.

Coding Standards
----------------

All code contributions must adhere to the following standards:

**Mandatory Rules:**

1. **Descriptive Variable Names**
   
   Use full, descriptive names that clearly indicate the variable's purpose. Avoid abbreviations and single-letter names except for loop counters in trivial loops.
   
   .. code-block:: c
   
      // Good
      Token function_name;
      ASTNode *condition_expression;
      int array_element_count;
      
      // Bad
      Token fn;
      ASTNode *cond;
      int cnt;

2. **No Magic Numbers - Always Use Constants**
   
   Replace all numeric literals with named constants that explain their meaning.
   
   .. code-block:: c
   
      // Good
      #define MAX_PARAMETERS 128
      #define VHDL_BIT_WIDTH 32
      #define FIRST_CHILD_INDEX 0
      
      if (parameter_count < MAX_PARAMETERS) { ... }
      
      // Bad
      if (parameter_count < 128) { ... }

3. **Static Functions for Internal Helpers**
   
   All helper functions that are not part of the public API must be declared ``static`` to limit their scope to the current translation unit.
   
   .. code-block:: c
   
      // Good
      static void emit_mapped_signal_name(const char *variable_name, FILE *output_file);
      static int parse_array_dimensions(const char *var_name, char *array_name);
      
      // Bad (unless intended for public API)
      void emit_mapped_signal_name(const char *variable_name, FILE *output_file);

4. **Modular Functions - Divide Work into Focused Functions**
   
   Break down complex logic into small, focused helper functions. Each function should have a single, clear responsibility. Follow the same modular approach used in the parser source files.
   
   .. code-block:: c
   
      // Good - Clean dispatcher with focused helpers
      static void generate_node(ASTNode *node, FILE *output_file) {
          switch (node->type) {
              case NODE_WHILE_STATEMENT:
                  generate_while_loop(node, output_file);
                  break;
              case NODE_FOR_STATEMENT:
                  generate_for_loop(node, output_file);
                  break;
              // ...
          }
      }
      
      // Bad - Monolithic function handling everything inline

5. **Statement Bodies on Separate Lines from Conditions**
   
   Control flow statement bodies (if, while, for) must always be on separate lines from their conditions. This improves readability and makes debugging easier.
   
   .. code-block:: c
   
      // Good
      if (condition_expression != NULL)
      {
          process_condition(condition_expression);
      }
      
      while (index < array_size)
      {
          process_element(array[index]);
          index++;
      }
      
      // Bad
      if (condition_expression != NULL) { process_condition(condition_expression); }
      while (index < array_size) { process_element(array[index]); index++; }

6. **All Variables Must Be Initialized**
   
   Never leave variables uninitialized. Always provide explicit initial values, even if they will be immediately reassigned.
   
   .. code-block:: c
   
      // Good
      int child_index = 0;
      ASTNode *condition = NULL;
      char buffer[MAX_BUFFER_SIZE] = {0};
      const char *operator = NULL;
      
      // Bad
      int child_index;
      ASTNode *condition;
      char buffer[MAX_BUFFER_SIZE];

**Rationale:**

These standards ensure that code is:

* **Readable** - Anyone can understand the code's intent without extensive comments
* **Maintainable** - Changes can be made safely without unintended side effects
* **Debuggable** - Issues are easy to identify and fix
* **Consistent** - All code follows the same patterns and style

Refactoring Principles
----------------------

The refactoring effort followed these core principles:

**1. Single Responsibility Principle**

Each function should have one clear, well-defined purpose. Large monolithic functions were broken down into focused helper functions.

**2. Self-Documenting Code**

Code should be readable without extensive comments through:

* Descriptive variable names (``identifier_name`` instead of ``id``)
* Descriptive function names (``parse_logical_not()`` instead of generic parsing code)
* Named constants instead of magic numbers (``MAX_STRUCT_FIELDS`` instead of ``32``)

**3. Consistent Organization**

All parser modules follow a consistent structure:

* Constants defined at the top
* Forward declarations for helper functions
* Helper functions before main functions
* Clear separation of concerns

**4. Reduced Complexity**

* Minimize nesting depth
* Extract complex logic into named helper functions
* Use early returns to reduce indentation
* Clear dispatcher patterns for multi-case logic

Parser Organization
--------------------

Each parser module is split into small, single-purpose helper functions
behind a short dispatcher, verified against the current source:

* ``parse_expression.c`` — ``parse_primary()`` dispatches to
  ``parse_logical_not()``, ``parse_bitwise_not()``, ``parse_unary_minus()``,
  ``parse_parenthesized_expr()``, ``parse_field_access()``,
  ``parse_array_index()``, ``validate_array_bounds()``, ``parse_identifier()``,
  and ``parse_number()``. Buffer sizes are named constants:
  ``IDENTIFIER_BUFFER_SIZE``, ``INDEX_EXPRESSION_BUFFER_SIZE``,
  ``FULL_EXPRESSION_BUFFER_SIZE``, ``OPERATOR_COPY_BUFFER_SIZE``. Parenthesized
  expressions reuse the same top-level minimum precedence
  (``PREC_TOP_LEVEL_MIN``, defined in ``utils.h``) as any other expression —
  parentheses reset precedence, they do not introduce a separate one.
* ``parse_statement.c`` — ``parse_statement()`` dispatches to
  ``parse_variable_declaration()``, ``parse_assignment_or_expression()``,
  ``parse_return_statement()``, ``parse_if_statement()``,
  ``parse_while_statement()``, ``parse_for_statement()``,
  ``parse_break_statement()``, and ``parse_continue_statement()``, plus the
  smaller ``parse_initializer_list()``, ``parse_lhs_expression()``,
  ``parse_else_blocks()``, ``parse_for_init()``, and
  ``parse_for_increment()`` behind them.
* ``parse_function.c`` — ``parse_function()`` calls
  ``parse_function_parameters()`` (with ``parse_single_parameter()`` per
  parameter) and ``parse_function_body()``.
* ``parse_struct.c`` — ``parse_struct()`` calls ``parse_struct_field()`` per
  field, which registers into the struct symbol table via
  ``register_struct()``/``register_struct_field()`` (``symbol_structs.c``).
* ``parse.c`` — ``parse_program()`` dispatches to
  ``parse_struct_declaration()`` and ``parse_function_declaration()``, with
  ``skip_to_semicolon()``/``skip_to_sync_point()`` for error recovery.

Buffer-size and table-limit constants throughout the parser and codegen are
named rather than left as bare numbers — for example ``MAX_STRUCT_FIELDS``
(``symbol_structs.h``, tied to the ``-DGATES_MAX_STRUCT_FIELDS`` override) and
the buffer constants listed above.

Variable Naming Conventions
----------------------------

The refactoring established clear naming conventions:

**Tokens:**

* Full descriptive names: ``return_type``, ``function_name``, ``field_type``
* Suffix ``_token`` when needed for clarity: ``struct_name_token``
* No abbreviations: ``fname`` → ``field_name``

**AST Nodes:**

* Descriptive purpose: ``struct_node``, ``function_node``, ``parameter_node``
* No generic names like ``n``, ``node``, ``tmp``
* Suffix ``_node`` for clarity when needed

**Buffer Sizes and Indices:**

* Descriptive: ``available_space``, ``copy_length``, ``array_size``
* No single-letter names: ``n`` → ``copy_length``, ``i`` → ``field_index``

**Temporary Variables:**

* Describe what they hold: ``condition_expression``, ``increment_expression``
* Not generic: ``temp``, ``tmp``, ``val``

Constants and Magic Numbers
----------------------------

All magic numbers were replaced with named constants:

**Buffer Sizes:**

.. code-block:: c

   // Instead of:
   char buf[128];
   char expr[512];
   
   // Use:
   #define IDENTIFIER_BUFFER_SIZE 128
   #define INDEX_EXPRESSION_BUFFER_SIZE 512
   char identifier_buffer[IDENTIFIER_BUFFER_SIZE];
   char index_expression[INDEX_EXPRESSION_BUFFER_SIZE];

**Limits and Thresholds:**

.. code-block:: c

   // Instead of:
   if (count < 32) { ... }

   // Use (symbol_structs.h, tied to the -DGATES_MAX_STRUCT_FIELDS override):
   #define MAX_STRUCT_FIELDS GATES_MAX_STRUCT_FIELDS
   if (field_count < MAX_STRUCT_FIELDS) { ... }

**Precedence Values:**

.. code-block:: c

   // Instead of:
   parse_expression_prec(ctx, -2);

   // Use (utils.h):
   #define PREC_TOP_LEVEL_MIN -2
   parse_expression_prec(ctx, PREC_TOP_LEVEL_MIN);

Helper Function Patterns
-------------------------

The refactoring established consistent patterns for helper functions:

**Single Responsibility:**

Each helper does one thing:

.. code-block:: c

   // Good: Clear single purpose
   static ASTNode* parse_logical_not(ParserContext *ctx);
   static ASTNode* parse_return_statement(ParserContext *ctx);
   static void validate_array_bounds(const char *array_name, const char *index_str);
   
   // Bad: Multiple responsibilities
   static ASTNode* parse_unary_and_validate(ParserContext *ctx);

**Clear Naming:**

Function names describe exactly what they do:

.. code-block:: c

   // Good: Clear purpose from name
   int register_struct(const char *name);
   static ASTNode* parse_else_blocks(ParserContext *ctx, ASTNode *if_node);
   
   // Bad: Generic/unclear
   static void handle_struct(int idx, Token t);
   static ASTNode* process_blocks(ParserContext *ctx, ASTNode *n);

**Forward Declarations:**

All helper functions declared at top of file:

.. code-block:: c

   // Forward declarations
   static ASTNode* parse_logical_not(ParserContext *ctx);
   static ASTNode* parse_bitwise_not(ParserContext *ctx);
   static ASTNode* parse_unary_minus(ParserContext *ctx);
   // ...
   
   // Implementations
   static ASTNode* parse_logical_not(ParserContext *ctx) {
       // ...
   }

Code Organization
-----------------

Most parser and codegen files follow the same general shape — includes,
then any file-local constants, then static helper functions, then the
public entry point the header declares. It is a loose convention rather
than a strict rule enforced anywhere: not every file has a constants block
(``parse_function.c`` has none), and forward declarations are used only
where a helper needs to call one defined later in the same file (for
example ``parse_statement.c`` forward-declares ``parse_statement_inner()``).

Observed outcomes
------------------

* Helper functions are short and single-purpose (see `Parser Organization`_ above)
* Named constants replace magic numbers for buffer sizes and table limits
* Repeated parsing patterns (skip-to-semicolon, comma-separated argument
  lists, field-access decoding) are extracted into shared helpers used from
  more than one call site

Future Refactoring Opportunities
---------------------------------

Areas for potential future improvement:

**Error Handling:**

* Extract error reporting into helper functions
* Consistent error message formatting
* Better error recovery strategies

**Symbol Tables:**

* Refactor global symbol table access into dedicated module
* Consistent API for registration and lookup
* Better encapsulation of symbol table data structures

**Testing:**

* Unit tests for individual helper functions
* Integration tests for complete parsing scenarios
* Validation of refactoring (no behavior changes)

**Documentation:**

* Document helper function contracts
* Add examples for each statement type
* Inline documentation for complex algorithms

Conclusion
----------

The parser refactoring replaced large monolithic functions with smaller helpers and clearer module boundaries. The resulting structure provides a concrete model for future changes and makes the compiler easier to review, modify, and extend.

The refactoring demonstrates that **code quality is not just about correctness** — it's about creating code that is:

* Easy to understand
* Easy to modify
* Easy to test
* Easy to extend
* Consistent with the surrounding codebase

These improvements lay a solid foundation for future compiler enhancements and serve as a model for maintaining high code quality throughout the project.

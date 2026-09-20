Parser Implementation
=====================

Overview
--------

The parser is a **recursive descent parser** implemented across multiple modules in ``src/parser/``. It consumes tokens one at a time from the tokenizer and constructs an Abstract Syntax Tree (AST) representing the structure of the C source code.

**Key responsibilities:**

* Parse the supported C subset (functions, statements, expressions, structs)
* Build AST nodes and establish parent-child relationships
* Perform syntax validation and error reporting via ``log_error()``
* Handle operator precedence using precedence climbing
* Validate array bounds and loop control flow (``break``/``continue`` outside a loop)
* Flatten struct field access and array indexing into string representations
* Guard against unbounded recursion on deeply nested input

.. note::
   The parser uses a **modular design**, with separate files for different syntactic categories: the top-level driver, functions, structs, statements, control flow, for-loops, and expressions.

Location
--------

The parser is split across multiple source files:

- Top-level driver: ``src/parser/parse.c`` / ``include/parse.h``
- Tokenizer: ``src/parser/token.c`` / ``src/parser/tokenizer.h`` (internal) / ``include/token.h`` (public ``Token``/``ParserContext``/``TokenType``)
- Expressions: ``src/parser/parse_expression.c`` / ``src/parser/parse_expression.h``
- Statements: ``src/parser/parse_statement.c`` / ``src/parser/parse_statement.h``
- Functions: ``src/parser/parse_function.c`` / ``src/parser/parse_function.h``
- Structs: ``src/parser/parse_struct.c`` / ``src/parser/parse_struct.h``
- If/while/break/continue: ``src/parser/parse_control_flow.c`` / ``src/parser/parse_control_flow.h``
- For loops: ``src/parser/parse_for.c`` / ``src/parser/parse_for.h``
- Shared utilities (precedence, safe string helpers): ``src/core/utils.c`` / ``include/utils.h``
- Struct symbol table: ``src/symbols/symbol_structs.c`` / ``include/symbol_structs.h``
- Array symbol table: ``src/symbols/symbol_arrays.c`` / ``include/symbol_arrays.h``

Parser Architecture
-------------------

The parser follows a **top-down recursive descent** design:

.. code-block:: text

   parse_program()
     ├─ parse_struct_declaration()
     │    ├─ parse_struct()              // struct Name { ... };
     │    │    └─ parse_struct_field()
     │    └─ parse_function()            // struct Name f(...) { ... }
     └─ parse_function_declaration()
          └─ parse_function()
               ├─ parse_function_parameters()
               │    └─ parse_single_parameter()
               └─ parse_function_body()
                    └─ parse_statement()          [depth-guarded]
                         ├─ parse_variable_declaration()
                         ├─ parse_assignment_or_expression()
                         │    └─ parse_expression()
                         ├─ parse_if_statement() ─┐
                         ├─ parse_while_statement()├─ parse_statement() (recursive)
                         ├─ parse_for_statement() ─┘
                         ├─ parse_break_statement()
                         ├─ parse_continue_statement()
                         └─ parse_expression()
                              └─ parse_expression_prec()   [depth-guarded]
                                   └─ parse_primary()

**Design principles:**

* **One function per grammar rule**: each parsing function corresponds to a syntactic category
* **Mutual recursion**: statements contain expressions; expressions contain parenthesized sub-expressions; control-flow bodies contain more statements
* **Lookahead**: ``match(ctx, type)`` checks the current token's type without consuming it
* **Error recovery**: errors are reported via ``log_error(category, line, fmt, ...)``; parsing functions return ``NULL`` (or ``return`` for ``void`` helpers). Nothing here uses ``printf()`` for diagnostics — see :ref:`parser-error-handling` below for exactly how far parsing continues after an error
* **Recursion depth guard**: ``parse_statement()`` and ``parse_expression_prec()`` are thin wrappers that check ``ctx->depth`` against ``GATES_MAX_PARSE_DEPTH`` (default 512, configurable via ``-DGATES_MAX_PARSE_DEPTH``) before calling into the real recursive body, so pathologically nested input is rejected with an error instead of overflowing the C call stack
* **Symbol tracking**: registers arrays and struct fields during parsing for later bounds validation and code generation

Grammar Overview
----------------

The parser recognizes a subset of C with the following grammar (simplified):

.. code-block:: text

   program         ::= (struct_decl | function_decl)*

   struct_decl     ::= 'struct' IDENTIFIER '{' field_decl* '}' ';'
   field_decl      ::= type IDENTIFIER ';'

   function_decl   ::= type IDENTIFIER '(' param_list ')' '{' statement* '}'
   param_list      ::= 'void' | (param (',' param)*)?
   param           ::= type IDENTIFIER

   statement       ::= var_decl | assignment | func_call_stmt | return_stmt |
                       if_stmt | while_stmt | for_stmt | break_stmt | continue_stmt

   var_decl        ::= type IDENTIFIER ('[' NUMBER ']')? ('=' initializer)? ';'
   initializer     ::= expr | '{' (NUMBER | IDENTIFIER)* '}'
   assignment      ::= IDENTIFIER ('.' IDENTIFIER)* ('[' expr ']')? '=' expr ';'
   func_call_stmt  ::= IDENTIFIER '(' (expr (',' expr)*)? ')' ';'
   return_stmt     ::= 'return' expr ';'
   if_stmt         ::= 'if' '(' expr ')' '{' statement* '}'
                       ('else' 'if' '(' expr ')' '{' statement* '}')*
                       ('else' '{' statement* '}')?
   while_stmt      ::= 'while' '(' expr ')' '{' statement* '}'
   for_stmt        ::= 'for' '(' for_init ';' expr? ';' for_incr ')' '{' statement* '}'
   for_init        ::= var_decl_no_semi | assignment_no_semi | expr_no_semi | (empty)
   for_incr        ::= IDENTIFIER ('++' | '--' | '=' expr) | (empty)
   break_stmt      ::= 'break' ';'
   continue_stmt   ::= 'continue' ';'

   expr            ::= primary (OPERATOR primary)*
   primary         ::= IDENTIFIER | IDENTIFIER '(' args ')' | NUMBER |
                       '(' expr ')' | unary_op primary
   unary_op        ::= '!' | '~' | '-'

.. note::
   ``i++`` / ``i--`` are only accepted in the **increment clause of a for-loop**
   header (``for (...; ...; i++)``), where the parser desugars them into
   ``i = i + 1`` / ``i = i - 1``. As a standalone statement, ``i++;`` is a
   parse error — increment/decrement and every compound-assignment operator
   (``+=``, ``-=``, ...) are rejected outside that one context. See
   :doc:`../known_issues`.

Program Parsing
---------------

parse_program()
~~~~~~~~~~~~~~~

The entry point for parsing, in ``src/parser/parse.c``. Iterates over top-level declarations:

.. code-block:: c

   ASTNode* parse_program(FILE *input)
   {
       ParserContext ctx;
       parser_context_init(&ctx, input);

       ASTNode *program_node = create_node(NODE_PROGRAM);

       advance(&ctx); // prime tokenizer

       while (!match(&ctx, TOKEN_EOF) && !has_errors()) {
           if (match(&ctx, TOKEN_KEYWORD)) {
               if (strcmp(ctx.current_token.value, "struct") == 0) {
                   parse_struct_declaration(&ctx, program_node);
                   continue;
               }

               // Primitive or known type function
               Token return_type = ctx.current_token;
               advance(&ctx);
               parse_function_declaration(&ctx, return_type, program_node);
           } else {
               log_error(ERROR_CATEGORY_PARSER, ctx.current_token.line,
                         "Unexpected token '%s' at top level",
                         ctx.current_token.value);
               advance(&ctx);
           }
       }

       return program_node;
   }

**Process:**

1. Create root ``NODE_PROGRAM`` node
2. Prime the tokenizer with ``advance(&ctx)``
3. Loop until ``TOKEN_EOF`` or ``has_errors()`` — the ``!has_errors()`` condition means the very first top-level error stops the whole loop, so at most one function/struct's worth of errors is ever reported per run
4. Dispatch to struct or function parsing based on the leading keyword; any other token at top level is a hard error, not a silent skip

**Recognized patterns:**

* ``struct Name { ... };`` → ``parse_struct_declaration()``
* ``int function(...) { ... }`` → ``parse_function_declaration()``
* Functions returning structs: ``struct Name func(...) { ... }`` (also dispatched from ``parse_struct_declaration()``, which disambiguates on whether an identifier or ``{`` follows the struct name)

parse_function_declaration()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Parses functions with primitive return types, also in ``parse.c``:

.. code-block:: c

   static ASTNode* parse_function_declaration(ParserContext *ctx, Token return_type, ASTNode *program_node)
   {
       if (!match(ctx, TOKEN_IDENTIFIER)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                       "Expected identifier after type '%s'", return_type.value);
           skip_to_sync_point(ctx);
           return NULL;
       }

       Token function_name = ctx->current_token;
       advance(ctx);

       if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
           ASTNode *function_node = parse_function(ctx, return_type, function_name);
           if (function_node) {
               add_child(program_node, function_node);
           }
           return function_node;
       } else {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                       "Global variable declarations not supported (found '%s %s' without '(')",
                       return_type.value, function_name.value);
           skip_to_semicolon(ctx);
           return NULL;
       }
   }

.. note::
   Global variable declarations are **not supported**. A type/identifier pair
   not followed by ``(`` is reported as an error, not silently accepted.

Struct Parsing
--------------

parse_struct()
~~~~~~~~~~~~~~

In ``src/parser/parse_struct.c``. Parses struct definitions with field declarations:

.. code-block:: c

   ASTNode* parse_struct(ParserContext *ctx, Token struct_name_token)
   {
       if (!consume(ctx, TOKEN_BRACE_OPEN)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected '{' after struct name");
           return NULL;
       }

       ASTNode *struct_node = create_node(NODE_STRUCT_DECL);
       struct_node->value = safe_strdup(struct_name_token.value);

       int struct_index = register_struct(struct_name_token.value);
       if (struct_index < 0) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Struct table full or invalid name");
           free_node(struct_node);
           return NULL;
       }

       while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
           ASTNode *field_node = parse_struct_field(ctx, struct_index);
           if (field_node) {
               add_child(struct_node, field_node);
           } else {
               advance(ctx);
           }
       }

       if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected '}' after struct body");
       }
       if (!consume(ctx, TOKEN_SEMICOLON)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected ';' after struct declaration");
       }

       return struct_node;
   }

**Process:**

1. Consume the opening brace
2. Create ``NODE_STRUCT_DECL`` with the struct name
3. Register the struct via ``register_struct()`` (declared in ``include/symbol_structs.h``, implemented in ``src/symbols/symbol_structs.c``) — this returns the struct's index into the internal table, or ``-1`` if the table (``GATES_MAX_STRUCTS``, default 64) is full
4. Parse fields until the closing brace, each one registered with ``register_struct_field(struct_index, field_type, field_name)``
5. Consume the closing brace and semicolon

**Side effects:** registers the struct and each field in the symbol table owned by ``symbol_structs.c``, used later by codegen to resolve field types (see :doc:`codegen`). That table is ``static`` to ``symbol_structs.c`` — the parser only ever touches it through ``register_struct()``, ``register_struct_field()``, and ``find_struct_index()``; there is no directly-accessible global struct array.

parse_struct_field()
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_struct_field(ParserContext *ctx, int struct_index)
   {
       if (!match(ctx, TOKEN_KEYWORD)) {
           return NULL;
       }

       Token field_type = ctx->current_token;
       advance(ctx);

       if (!match(ctx, TOKEN_IDENTIFIER)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected field name in struct");
           return NULL;
       }

       Token field_name = ctx->current_token;
       advance(ctx);

       ASTNode *field_node = create_node(NODE_VAR_DECL);
       field_node->token = field_type;
       field_node->value = safe_strdup(field_name.value);

       register_struct_field(struct_index, field_type.value, field_name.value);

       if (!consume(ctx, TOKEN_SEMICOLON)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected ';' after struct field");
           free_node(field_node);
           return NULL;
       }

       return field_node;
   }

**Limitations:**

* No nested structs as field types
* No array fields
* No pointer fields
* All field types must be primitive keyword types

**Example struct:**

.. code-block:: c

   struct Point {
       int x;
       int y;
   };

**AST structure** (each field is a child of the struct node, per the code above):

.. code-block:: text

   NODE_STRUCT_DECL (value = "Point")
     ├─ NODE_VAR_DECL (token.value = "int", value = "x")
     └─ NODE_VAR_DECL (token.value = "int", value = "y")

Struct Symbol Table
~~~~~~~~~~~~~~~~~~~~

The real storage, in ``src/symbols/symbol_structs.c``:

.. code-block:: c

   #define MAX_STRUCTS          GATES_MAX_STRUCTS        // default 64
   #define MAX_STRUCT_FIELDS    GATES_MAX_STRUCT_FIELDS  // default 32
   #define STRUCT_NAME_LENGTH   64

   typedef struct {
       char field_name[STRUCT_NAME_LENGTH];
       char field_type[32];
   } FieldInfo;

   typedef struct {
       char name[STRUCT_NAME_LENGTH];
       FieldInfo fields[MAX_STRUCT_FIELDS];
       int field_count;
   } StructInfo;

   static StructInfo g_structs[MAX_STRUCTS];
   static int g_struct_count = 0;

``g_structs``/``g_struct_count`` are ``static`` — nothing outside ``symbol_structs.c`` reads or writes them directly. The public API is:

.. code-block:: c

   int register_struct(const char *name);
   int register_struct_field(int struct_index, const char *field_type, const char *field_name);
   int find_struct_index(const char *name);
   int get_struct_count(void);
   const StructInfo* get_struct_info(int struct_index);
   void reset_struct_table(void);

``find_struct_index()`` and ``get_struct_info()`` are what codegen uses to resolve field types and layout (see :doc:`codegen`).

Function Parsing
----------------

parse_function()
~~~~~~~~~~~~~~~~

In ``src/parser/parse_function.c``. Entry point for parsing a function's parameters and body:

.. code-block:: c

   ASTNode* parse_function(ParserContext *ctx, Token return_type, Token function_name)
   {
       ASTNode *function_node = create_node(NODE_FUNCTION_DECL);
       function_node->token = return_type;
       function_node->value = safe_strdup(function_name.value);

       // Reset the array table for this function scope
       reset_array_table();

       parse_function_parameters(ctx, function_node);
       if (has_errors()) {
           free_node(function_node);
           return NULL;
       }

       parse_function_body(ctx, function_node);
       if (has_errors()) {
           free_node(function_node);
           return NULL;
       }

       return function_node;
   }

**Important:** ``reset_array_table()`` means arrays are **function-scoped**. An array declared in one function does not persist to another (see :doc:`symbols`).

parse_function_parameters()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void parse_function_parameters(ParserContext *ctx, ASTNode *function_node)
   {
       if (!consume(ctx, TOKEN_PARENTHESIS_OPEN)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected '(' after function name");
           return;
       }

       while (!match(ctx, TOKEN_PARENTHESIS_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
           if (match(ctx, TOKEN_KEYWORD)) {
               // `void` as a parameter type only appears in C to spell an empty
               // parameter list, e.g. `int f(void)`. It is never a real
               // parameter, so consume it without expecting a following name.
               if (strcmp(ctx->current_token.value, "void") == 0) {
                   advance(ctx);
                   break;
               }

               Token parameter_type = (Token){0};
               ASTNode *parameter_node = parse_single_parameter(ctx, &parameter_type);
               if (!parameter_node) {
                   break;
               }

               add_child(function_node, parameter_node);

               if (match(ctx, TOKEN_COMMA)) {
                   advance(ctx);
               }
           } else {
               advance(ctx);
           }
       }

       if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected ')' after parameter list");
       }
   }

**Two parameter type patterns**, handled by ``parse_single_parameter()``:

1. **Primitive types:** ``int x`` → type = ``"int"``, name = ``"x"``
2. **Struct types:** ``struct Point p`` → type = ``"Point"`` (the ``struct`` keyword is consumed and only the struct name kept), name = ``"p"``

parse_function_body()
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void parse_function_body(ParserContext *ctx, ASTNode *function_node)
   {
       if (!consume(ctx, TOKEN_BRACE_OPEN)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected '{' to start function body");
           return;
       }

       while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
           ASTNode *statement_node = parse_statement(ctx);
           if (statement_node) {
               add_child(function_node, statement_node);
           }
       }

       if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected '}' after function body");
       }
   }

There is no manual brace-depth counter here: ``if``/``while``/``for`` bodies each consume their own ``{``/``}`` inside ``parse_statement()``'s dispatch, so this loop only ever sees the function's own closing brace.

**Example function:**

.. code-block:: c

   int add(int a, int b) {
       int result = a + b;
       return result;
   }

**AST structure:**

.. code-block:: text

   NODE_FUNCTION_DECL (token.value = "int", value = "add")
     ├─ NODE_VAR_DECL (token.value = "int", value = "a")  [parameter]
     ├─ NODE_VAR_DECL (token.value = "int", value = "b")  [parameter]
     ├─ NODE_STATEMENT
     │    └─ NODE_VAR_DECL (token.value = "int", value = "result")
     │         └─ NODE_BINARY_EXPR (value = "+")
     │              ├─ NODE_EXPRESSION (value = "a")
     │              └─ NODE_EXPRESSION (value = "b")
     └─ NODE_STATEMENT (token.value = "return")
          └─ NODE_EXPRESSION (value = "result")

Expression Parsing
-------------------

The expression parser uses **precedence climbing** to handle operator precedence, in ``src/parser/parse_expression.c``.

parse_expression() / parse_expression_prec()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_expression(ParserContext *ctx)
   {
       return parse_expression_prec(ctx, PREC_TOP_LEVEL_MIN);
   }

   // Depth-guarded entry point; the real body is parse_expression_prec_inner().
   ASTNode* parse_expression_prec(ParserContext *ctx, int min_prec)
   {
       if (ctx->depth >= GATES_MAX_PARSE_DEPTH) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expression nesting exceeds the maximum depth of %d",
                     GATES_MAX_PARSE_DEPTH);
           return NULL;
       }

       ctx->depth++;
       ASTNode *expr_node = parse_expression_prec_inner(ctx, min_prec);
       ctx->depth--;
       return expr_node;
   }

   static ASTNode* parse_expression_prec_inner(ParserContext *ctx, int min_prec)
   {
       ASTNode *left_operand = parse_primary(ctx);
       if (!left_operand) {
           return NULL;
       }
       while (match(ctx, TOKEN_OPERATOR)) {
           const char *op = ctx->current_token.value;
           int operator_precedence = get_precedence(op);
           if (operator_precedence < min_prec) {
               break;
           }
           char operator_copy[8] = {0};
           safe_copy(operator_copy, sizeof(operator_copy), op, sizeof(operator_copy) - 1);
           advance(ctx);
           ASTNode *right_operand = parse_expression_prec(ctx, operator_precedence + 1);
           if (!right_operand) {
               log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                         "Expected right operand after operator '%s'", operator_copy);
               free_node(left_operand);
               return NULL;
           }
           ASTNode *binary_expr = create_node(NODE_BINARY_EXPR);
           binary_expr->value = safe_strdup(operator_copy);
           add_child(binary_expr, left_operand);
           add_child(binary_expr, right_operand);
           left_operand = binary_expr;
       }
       return left_operand;
   }

**Algorithm:**

1. Parse the left operand with ``parse_primary()``
2. While the current token is an operator with precedence ≥ ``min_prec``:

   a. Save the operator
   b. Recursively parse the right operand with precedence = ``operator_precedence + 1`` (left-associativity)
   c. Combine into a ``NODE_BINARY_EXPR``
   d. The new expression becomes the left operand for the next iteration

3. Return the final expression tree

**Precedence levels** (``get_precedence()`` in ``src/core/utils.c``, constants in ``include/utils.h``):

.. code-block:: c

   PREC_MULTIPLICATIVE   7   // * /
   PREC_ADDITIVE         6   // + -
   PREC_SHIFT            5   // << >>
   PREC_RELATIONAL       4   // < <= > >=
   PREC_EQUALITY         3   // == !=
   PREC_BITWISE_AND      2   // &
   PREC_BITWISE_XOR      1   // ^
   PREC_BITWISE_OR       0   // |
   PREC_LOGICAL_AND     -1   // &&
   PREC_LOGICAL_OR      -2   // || (lowest)
   PREC_TOP_LEVEL_MIN    -2  // = PREC_LOGICAL_OR; the minimum precedence
                             // passed at every expression entry point,
                             // including inside parentheses

There is no separate "parenthesized expression" precedence constant — parenthesized sub-expressions restart at ``PREC_TOP_LEVEL_MIN``, the same minimum used everywhere else, since parentheses reset precedence completely.

**Example:** ``3 + 4 * 5``

.. code-block:: text

   parse_expression_prec(ctx, -2)
     left = parse_primary() → "3"
     operator = "+", precedence = 6
     right = parse_expression_prec(ctx, 7)
       left = parse_primary() → "4"
       operator = "*", precedence = 7
       right = parse_expression_prec(ctx, 8)
         left = parse_primary() → "5"
         no more operators
         return "5"
       return BINARY("*", "4", "5")
     return BINARY("+", "3", BINARY("*", "4", "5"))

parse_primary()
~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_primary(ParserContext *ctx)
   {
       if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "!") == 0) {
           return parse_logical_not(ctx);
       }
       if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "~") == 0) {
           return parse_bitwise_not(ctx);
       }
       if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "-") == 0) {
           return parse_unary_minus(ctx);
       }
       if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
           return parse_parenthesized_expr(ctx);
       }
       if (match(ctx, TOKEN_IDENTIFIER)) {
           return parse_identifier(ctx);
       }
       if (match(ctx, TOKEN_NUMBER)) {
           return parse_number(ctx);
       }
       return NULL;
   }

**Recognized patterns:**

* ``!expr`` → logical NOT (``NODE_BINARY_OP``, value ``"!"``)
* ``~expr`` → bitwise NOT (``NODE_BINARY_OP``, value ``"~"``)
* ``-expr`` → unary minus (see below)
* ``(expr)`` → parenthesized expression
* ``identifier`` / ``identifier(...)`` / ``identifier.field`` / ``identifier[index]`` → ``parse_identifier()``
* ``123`` → number literal

Unary minus (``-``) is optimized: if the operand is a plain ``NODE_EXPRESSION`` (a literal or bare identifier), the parser folds the ``-`` directly into its value (``"-x"``) instead of emitting a subtraction. Otherwise it builds ``0 - operand``:

.. code-block:: c

   static ASTNode* parse_unary_minus(ParserContext *ctx)
   {
       advance(ctx);
       ASTNode *operand = parse_primary(ctx);
       if (!operand) return NULL;

       if (operand->type == NODE_EXPRESSION && operand->value) {
           char negated_value[128];
           snprintf(negated_value, sizeof(negated_value), "-%s", operand->value);
           ASTNode *result_node = create_node(NODE_EXPRESSION);
           result_node->value = safe_strdup(negated_value);
           free_node(operand);
           return result_node;
       }

       ASTNode *zero_node = create_node(NODE_EXPRESSION);
       zero_node->value = safe_strdup("0");
       ASTNode *binary_expr = create_node(NODE_BINARY_EXPR);
       binary_expr->value = safe_strdup("-");
       add_child(binary_expr, zero_node);
       add_child(binary_expr, operand);
       return binary_expr;
   }

Parenthesized Expressions
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_parenthesized_expr(ParserContext *ctx)
   {
       advance(ctx);
       ASTNode *expr_node = parse_expression_prec(ctx, PREC_TOP_LEVEL_MIN);

       if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected ')' after expression");
           if (expr_node) free_node(expr_node);
           return NULL;
       }

       return expr_node;
   }

Identifier Parsing
~~~~~~~~~~~~~~~~~~

``parse_identifier()`` dispatches to a function call, or handles field access and array indexing on a plain identifier:

.. code-block:: c

   static ASTNode* parse_identifier(ParserContext *ctx)
   {
       char identifier_name[128] = {0};

       if (strlen(ctx->current_token.value) >= sizeof(identifier_name)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Identifier '%.32s...' exceeds the maximum length of %zu characters",
                     ctx->current_token.value, sizeof(identifier_name) - 1);
           advance(ctx);
           return NULL;
       }

       safe_copy(identifier_name, sizeof(identifier_name), ctx->current_token.value,
                 sizeof(identifier_name) - 1);
       advance(ctx);

       if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
           return parse_function_call(ctx, identifier_name);
       }

       parse_field_access(ctx, identifier_name, sizeof(identifier_name));
       if (has_errors()) return NULL;

       if (match(ctx, TOKEN_BRACKET_OPEN)) {
           return parse_array_access(ctx, identifier_name);
       }

       ASTNode *identifier_node = create_node(NODE_EXPRESSION);
       identifier_node->value = safe_strdup(identifier_name);
       return identifier_node;
   }

.. note::
   An identifier at or past the 128-character buffer limit is now a **parse
   error**, not a silent truncation — two identifiers differing only past
   that length used to collapse into the same signal name. See
   :doc:`../known_issues`.

**Field access flattening:** ``point.x.y`` → stored as ``"point__x__y"`` (double-underscore separator), built by ``parse_field_access()`` looping on ``.`` operators.

**Array indexing:** ``arr[i + 1]`` → stored as ``"arr[i+1]"`` (index expression captured verbatim as a string by ``parse_array_index()``, tracking parenthesis depth so a ``)`` inside the index doesn't get mistaken for the closing ``]``). ``parse_array_access()`` then calls ``validate_array_bounds()``, which only checks bounds when the index is a compile-time numeric constant (a dynamic index like ``arr[i]`` cannot be validated at parse time) — an out-of-bounds constant index is reported as ``ERROR_CATEGORY_SEMANTIC``, not ``ERROR_CATEGORY_PARSER``.

Function Call Parsing
----------------------

Function calls are represented by ``NODE_FUNC_CALL`` nodes with the function name as ``value`` and each argument expression as a child. Both expression-context and standalone-statement calls share one argument-parsing helper, ``parse_function_call_args()`` in ``parse_expression.c``:

.. code-block:: c

   ASTNode* parse_function_call_args(ParserContext *ctx, const char *function_name)
   {
       ASTNode *call_node = create_node(NODE_FUNC_CALL);
       call_node->value = safe_strdup(function_name);

       advance(ctx); // consume '('

       while (!match(ctx, TOKEN_PARENTHESIS_CLOSE) && !match(ctx, TOKEN_EOF)) {
           ASTNode *arg_node = parse_expression_prec(ctx, PREC_TOP_LEVEL_MIN);
           if (arg_node) {
               add_child(call_node, arg_node);
           }
           if (match(ctx, TOKEN_COMMA)) {
               advance(ctx);
               continue;
           }
           break;
       }

       if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected ')' after function call arguments for '%s'", function_name);
           free_node(call_node);
           return NULL;
       }

       return call_node;
   }

**Two thin wrappers around it:**

* ``parse_function_call()`` (in ``parse_expression.c``) — called from ``parse_identifier()`` when an identifier is followed by ``(``; used for calls inside expressions (``z = add(x, y) + 3;``, ``if (max(a, b) > 10)``, nested calls, ...).
* ``parse_standalone_function_call()`` (in ``parse_statement.c``) — called from ``parse_assignment_or_expression()`` when a statement starts with ``identifier(``; reuses ``parse_function_call_args()`` and then additionally requires a trailing semicolon (``print_debug(x);``).

**Supported contexts:** expression context, standalone statement, return statement, conditionals, loop conditions, and nested calls.

**AST examples**, all directly following from the node construction shown above:

.. code-block:: text

   z = add(x, y);
   →
   NODE_ASSIGNMENT
     ├─ NODE_EXPRESSION (value = "z")
     └─ NODE_FUNC_CALL (value = "add")
          ├─ NODE_EXPRESSION (value = "x")
          └─ NODE_EXPRESSION (value = "y")

   print_debug(x);
   →
   NODE_FUNC_CALL (value = "print_debug")
     └─ NODE_EXPRESSION (value = "x")

Limitations
~~~~~~~~~~~

* VHDL generation emits calls to entities that are never instantiated or wired together — see the cross-function-call limitation in :doc:`../examples` and :doc:`codegen`
* No type checking on arguments
* No validation that the called function actually exists
* No recursion support in VHDL

Statement Parsing
-----------------

parse_statement()
~~~~~~~~~~~~~~~~~

Depth-guarded the same way as ``parse_expression_prec()``; the real dispatcher is ``parse_statement_inner()``, both in ``parse_statement.c``:

.. code-block:: c

   ASTNode* parse_statement(ParserContext *ctx)
   {
       if (ctx->depth >= GATES_MAX_PARSE_DEPTH) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Statement nesting exceeds the maximum depth of %d",
                     GATES_MAX_PARSE_DEPTH);
           return NULL;
       }
       ctx->depth++;
       ASTNode *stmt = parse_statement_inner(ctx);
       ctx->depth--;
       return stmt;
   }

``parse_statement_inner()`` dispatches on the current token to: variable declaration (leading type keyword or ``struct``), assignment/expression/standalone-call (leading identifier), ``return``, ``if``, ``while``, ``for``, ``break``, ``continue``. Anything else is skipped up to the next ``;``, ``}``, or EOF.

Return statements get their own small helper, ``parse_return_statement()``: it stores the ``return`` keyword itself in the resulting ``NODE_STATEMENT``'s ``token`` field (so codegen can tell a return apart from other statement wrappers), parses the following expression, and requires a trailing semicolon.

Variable Declaration
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_variable_declaration(ParserContext *ctx, Token type_token)
   {
       int is_struct = 0, is_array = 0;

       if (strcmp(type_token.value, "struct") == 0) {
           if (!match(ctx, TOKEN_IDENTIFIER)) {
               log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                         "Expected struct name after 'struct'");
               return NULL;
           }
           type_token = ctx->current_token;
           advance(ctx);
           is_struct = 1;
       }

       if (!match(ctx, TOKEN_IDENTIFIER)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected variable name after type");
           return NULL;
       }
       Token name_token = ctx->current_token;
       advance(ctx);

       ASTNode *var_decl_node = create_node(NODE_VAR_DECL);
       var_decl_node->token = type_token;
       var_decl_node->value = safe_strdup(name_token.value);

       if (match(ctx, TOKEN_BRACKET_OPEN)) {
           is_array = 1;
           // ...parse '[' NUMBER ']', register_array(name, size), rename
           // var_decl_node->value to "name[size]"...
       }

       if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "=") == 0) {
           advance(ctx);
           if (is_array && match(ctx, TOKEN_BRACE_OPEN)) {
               add_child(var_decl_node, parse_initializer_list(ctx, 1));
           } else if (is_struct && match(ctx, TOKEN_BRACE_OPEN)) {
               add_child(var_decl_node, parse_initializer_list(ctx, 0));
           } else {
               ASTNode *init_expr = parse_expression(ctx);
               if (init_expr) add_child(var_decl_node, init_expr);

               // Anything between the initializer and ';' is now a hard error.
               if (!match(ctx, TOKEN_SEMICOLON) && !match(ctx, TOKEN_EOF)) {
                   log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                             "Unexpected '%s' after initializer; only one declarator "
                             "per declaration is supported", ctx->current_token.value);
                   while (!match(ctx, TOKEN_SEMICOLON) && !match(ctx, TOKEN_EOF)) advance(ctx);
               }
           }
       }

       if (!consume(ctx, TOKEN_SEMICOLON)) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected ';' after variable declaration");
           free_node(var_decl_node);
           return NULL;
       }

       return var_decl_node;
   }

.. note::
   ``int x = 1, y = 2;`` is a **parse error**: only one declarator per
   declaration is supported, and the second one is reported rather than
   silently discarded (which used to leave ``y`` referenced but never
   declared). Write each declarator as its own statement.

**Features:** primitive types (``int``, ``float``, ``char``, ``double``); struct types (``struct Point p;``); arrays (``int arr[10];``); scalar initialization (``int x = 42;``); array/struct brace initializers (``int arr[3] = {1, 2, 3};``, ``struct Point p = {10, 20};``).

**Side effect:** array declarations call ``register_array()`` (see :doc:`symbols`) to track sizes for bounds checking.

Assignment / Expression Statement
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_assignment_or_expression(ParserContext *ctx)
   {
       Token lhs_token = ctx->current_token;
       advance(ctx);

       if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
           return parse_standalone_function_call(ctx, lhs_token.value);
       }

       char lhs_buf[1024] = {0}, idx_buf[512] = {0}, base_name[128] = {0};
       parse_lhs_expression(ctx, lhs_token, lhs_buf, sizeof(lhs_buf),
                             idx_buf, sizeof(idx_buf), base_name, sizeof(base_name));
       if (has_errors()) return NULL;

       ASTNode *lhs_expr = create_node(NODE_EXPRESSION);
       lhs_expr->value = safe_strdup(lhs_buf);

       if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "=") == 0) {
           advance(ctx);
           ASTNode *assign_node = create_node(NODE_ASSIGNMENT);
           add_child(assign_node, lhs_expr);

           ASTNode *rhs_node = parse_expression(ctx);
           if (rhs_node) add_child(assign_node, rhs_node);

           if (!consume(ctx, TOKEN_SEMICOLON)) {
               log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                         "Expected ';' after assignment");
               free_node(assign_node);
               return NULL;
           }
           return assign_node;
       }

       // Not a plain "name = expr;" assignment.
       int is_compound_op = /* "++", "--", or any of + - * / % & | ^ << >> */ 0;
       if (is_compound_op) {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "'%s' is not supported as a statement; only 'name = expr;' "
                     "assignments are (increment/decrement and compound-assignment "
                     "operators are not yet implemented, see ROADMAP.md)",
                     ctx->current_token.value);
       } else {
           log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                     "Expected '=' after '%s'", lhs_buf);
       }
       // ...skip to next ';', free_node(lhs_expr), return NULL...
   }

**Left-hand side patterns** (built by ``parse_lhs_expression()``, the assignment-side twin of the identifier flattening described above):

* Simple variable: ``x = 10;``
* Struct field: ``point.x = 5;`` → ``"point__x"``
* Array element: ``arr[i] = 42;`` → ``"arr[i]"``
* Combined: ``points[i].x = 3;`` → ``"points[i]__x"``

.. note::
   ``i++;``, ``i--;``, and every compound-assignment operator (``+=``, ``-=``,
   ``*=``, ...) are **rejected with a specific error message** when used as a
   standalone statement — not silently dropped. This is a real, current gap
   (see the for-loop increment clause below for the one place ``++``/``--``
   *is* accepted), tracked in ``ROADMAP.md``.

Control Flow Statements
-----------------------

If / Else-If / Else
~~~~~~~~~~~~~~~~~~~~

In ``src/parser/parse_control_flow.c``. ``parse_if_statement()`` parses the ``if`` head and body, then hands off to ``parse_else_blocks()``, which loops on ``else`` to build zero or more ``NODE_ELSE_IF_STATEMENT`` children followed by an optional final ``NODE_ELSE_STATEMENT``:

.. code-block:: text

   NODE_IF_STATEMENT
     ├─ condition
     ├─ body statement(s)...
     ├─ NODE_ELSE_IF_STATEMENT
     │    ├─ condition
     │    └─ body statement(s)...
     └─ NODE_ELSE_STATEMENT
          └─ body statement(s)...

An empty condition (``if ()``) is rejected: ``parse_expression()`` returns ``NULL`` when there's nothing to parse, and the caller logs ``"Expected expression in if condition"`` rather than building a node with a missing child.

While Loop
~~~~~~~~~~

Structurally the same shape as ``if``, with a ``NODE_WHILE_STATEMENT`` node. Loop nesting is tracked with a file-static counter, incremented before parsing the body and decremented after:

.. code-block:: c

   static int s_loop_depth = 0;
   void loop_depth_inc(void) { s_loop_depth++; }
   void loop_depth_dec(void) { s_loop_depth--; }

This is what lets ``parse_break_statement()`` and ``parse_continue_statement()`` (also in ``parse_control_flow.c``) reject a ``break``/``continue`` outside any loop, reported as ``ERROR_CATEGORY_SEMANTIC`` rather than ``ERROR_CATEGORY_PARSER``.

For Loop
~~~~~~~~

In ``src/parser/parse_for.c``, the most involved control-flow parser: it has a separate init clause, an optional condition, and an increment clause with its own desugaring.

**Init** (``parse_for_init()``) accepts, in order: nothing (bare ``;``); a type-keyword-led variable declaration (``for (int i = 0; ...)``); or an identifier. For an identifier, if it's immediately followed by ``=`` it's parsed as an assignment; otherwise the parser backtracks (via ``ftell()``/``fseek()`` on the input stream) and re-parses it as a discarded expression — a bare ``for (i; ...)`` init has no hardware effect but still has to be consumed correctly so the following clauses don't shift.

**Condition:** parsed as a normal expression if present; an empty condition (``for (...;;...)``) becomes a literal ``"1"`` node (i.e. true), not an error — unlike ``if``/``while``, an omitted for-condition is valid C.

**Increment** (``parse_for_increment()``) is the one place the parser accepts and desugars ``++``/``--``:

.. code-block:: c

   if (match(ctx, TOKEN_OPERATOR) && (strcmp(op, "++") == 0 || strcmp(op, "--") == 0)) {
       // i++  →  NODE_ASSIGNMENT(i, NODE_BINARY_EXPR("+", i, 1))
       // i--  →  NODE_ASSIGNMENT(i, NODE_BINARY_EXPR("-", i, 1))
   } else if (match(ctx, TOKEN_OPERATOR) && strcmp(op, "=") == 0) {
       // i = expr  →  NODE_ASSIGNMENT(i, expr)
   }

The increment is added as the **last child** of the ``NODE_FOR_STATEMENT``, after the body statements — codegen emits it at the end of the loop body (see :doc:`codegen`).

**AST shape:**

.. code-block:: text

   NODE_FOR_STATEMENT
     ├─ initialization (NODE_VAR_DECL or NODE_ASSIGNMENT, if present)
     ├─ condition (always present: parsed expression, or a literal "1")
     ├─ body statement 1
     ├─ body statement 2, ...
     └─ increment (NODE_ASSIGNMENT, last child, if present)

**Loop depth tracking:** ``for`` uses the same ``loop_depth_inc()``/``loop_depth_dec()`` pair as ``while``, so ``break``/``continue`` validation is shared across both loop kinds.

Limitations
~~~~~~~~~~~

* No ``do``/``while``, ``switch``, or goto/labels
* No pointers
* No multi-dimensional arrays
* See :doc:`../known_issues` for the full, current list — this page describes the parser as it exists, not a target state

.. _parser-error-handling:

Error Handling
--------------

All parser diagnostics go through ``log_error(category, line, fmt, ...)`` (``src/error_handler.c``), which prints the message and increments an internal error counter — it does not abort the process itself. What actually stops parsing is that every parsing loop in the code shown above is additionally guarded with ``!has_errors()``, so once *any* error has been logged, no loop adds further children and (crucially) ``parse_program()``'s top-level loop stops after the current declaration. In practice this means: the first error inside a function or struct is usually the only one reported for that run, since the top-level loop won't move on to parse anything after it.

.. code-block:: text

   $ ./build/gates bad.c out.vhdl
   error[Parser] line 3: Expected '=' after 'x'
   error[General] Parsing failed with 1 error(s)

Every parser error uses ``ERROR_CATEGORY_PARSER``, except the array-bounds and loop-nesting checks, which use ``ERROR_CATEGORY_SEMANTIC`` (see :doc:`../usage` for the categories shown in gates' actual CLI output).

Helper Functions
----------------

The tokenizer primitives every parsing function above is built on, in ``src/parser/token.c``:

.. code-block:: c

   void advance(ParserContext *ctx) { ctx->current_token = get_next_token(ctx); }

   int match(const ParserContext *ctx, TokenType type) {
       return ctx->current_token.type == type;
   }

   int consume(ParserContext *ctx, TokenType type) {
       if (match(ctx, type)) { advance(ctx); return 1; }
       return 0;
   }

``consume()`` itself never logs an error — it just reports whether the expected token was there, and every call site above logs its own message on failure.

The string-safety helpers used throughout (``src/core/utils.c``):

* ``safe_strdup()`` — bounds-checked ``strdup()`` used for every AST node value
* ``safe_copy(dst, dst_size, src, limit)`` — copies at most ``limit`` bytes, always NUL-terminates
* ``safe_append(dst, dst_size, src)`` — appends to an existing buffer without overflowing it, used to build up flattened field-access/array-index strings incrementally

Design Decisions
-----------------

**Recursive descent:** simple, intuitive mapping from grammar to code; easy to extend; cannot handle left-recursive grammars (not needed for this C subset). Stack depth is now bounded by the ``GATES_MAX_PARSE_DEPTH`` guard described above rather than growing unchecked with input nesting.

**Precedence climbing for expressions:** handles all binary operators with correct precedence in one function, using ``min_prec + 1`` on the recursive call for left-associativity; more compact than one function per precedence level.

**``ParserContext`` instead of global state:** every parsing function takes a ``ParserContext *ctx`` rather than reading module-global current-token/line variables. This is what makes the parser reentrant — nothing here prevents parsing multiple files in the same process.

**Flattened field access and array indexing:** ``point.x`` and ``arr[i]`` become plain strings (``"point__x"``, ``"arr[i]"``) rather than nested AST nodes. This simplifies codegen (no tree walk needed to resolve a reference) at the cost of losing structure — the index expression inside ``arr[...]`` is opaque text to everything downstream, and the ``__`` separator would collide with a real identifier that already contains a double underscore.

**Error-counter-driven stopping, not true fail-fast:** ``log_error()`` doesn't unwind the call stack — the ``!has_errors()`` guards on parsing loops are what stop further work after the first error, at both the statement and top-level-declaration granularity (see :ref:`parser-error-handling`). There's no error-recovery pass that resumes after a bad construct to report unrelated errors elsewhere in the same file.

**Loop depth as a file-static counter, not context state:** ``s_loop_depth`` in ``parse_control_flow.c`` is not part of ``ParserContext``, unlike everything else in this section. It's incremented/decremented correctly within a single parse, but — unlike the rest of the parser — two ``parse_program()`` calls in the same process would need it reset between them.

Summary
-------

The parser is a **modular recursive descent parser** with **precedence climbing** for expressions, split across eight source files by syntactic category. It:

* Consumes tokens through a ``ParserContext`` (no global mutable parsing state) and builds an AST
* Handles the supported C subset: functions, structs, statements, expressions, one level of struct field access, single-dimensional arrays
* Reports errors via ``log_error()`` with line numbers and specific messages — including for constructs that used to be silently dropped (compound assignment, ``++``/``--`` as a statement, a second declarator, an over-length identifier)
* Flattens field access and array indexing into strings for codegen to consume directly
* Guards recursion depth on both statements and expressions via ``GATES_MAX_PARSE_DEPTH``
* Tracks array sizes and struct layout through dedicated symbol-table modules, not ad hoc globals

See :doc:`../known_issues` for what the parser does *not* yet handle, and :doc:`codegen` for how the AST it produces becomes VHDL.

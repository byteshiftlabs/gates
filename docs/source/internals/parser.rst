Parser Implementation
=====================

Overview
--------

The parser implements a **recursive descent parser** that converts the token stream from the lexer into an Abstract Syntax Tree (AST). The parser is modular, splitting parsing logic across specialized files:

- ``src/parser/parse.c`` - Main parser driver and program-level parsing
- ``src/parser/parse_expression.c`` - Expression parsing with operator precedence
- ``src/parser/parse_statement.c`` - Statement parsing (if, while, for, declarations, assignments)
- ``src/parser/parse_function.c`` - Function declaration parsing
- ``src/parser/parse_struct.c`` - Struct declaration parsing

Location
--------

**Source files:**

* ``src/parser/parse.c``
* ``src/parser/parse_expression.c``
* ``src/parser/parse_statement.c``
* ``src/parser/parse_function.c``
* ``src/parser/parse_struct.c``

**Header files:**

* ``include/parse.h``
* ``include/parse_expression.h``
* ``include/parse_statement.h``
* ``include/parse_function.h``
* ``include/parse_struct.h``

Parsing Strategy
----------------

The parser uses **recursive descent** with these characteristics:

* **Top-down**: Starts from the program and descends into functions, statements, expressions
* **LL(1)**: Uses one token of lookahead via ``current_token``
* **Modular**: Separate files for different syntactic categories
* **Manual precedence handling**: Expression parsing uses precedence climbing algorithm
* **Error recovery**: Minimal - prints warnings and calls ``exit()`` on critical errors

Parser Entry Point
------------------

parse_program()
~~~~~~~~~~~~~~~

The main entry point for parsing is ``parse_program()`` in ``src/parser/parse.c``:

.. code-block:: c

   ASTNode* parse_program(FILE *input);

This function:

1. Creates a ``NODE_PROGRAM`` AST root node
2. Calls ``advance(input)`` to prime the tokenizer
3. Loops until ``TOKEN_EOF`` is encountered
4. Dispatches to specialized parsers based on the current token:
   
   - If keyword ``struct`` → ``parse_struct_declaration()``
   - If type keyword (``int``, ``float``, etc.) → ``parse_function_declaration()``
   - Otherwise → skip unknown token

5. Returns the complete AST

**Implementation:**

.. code-block:: c

   ASTNode* parse_program(FILE *input) {
       ASTNode *program_node = create_node(NODE_PROGRAM);
       
       advance(input); // Prime tokenizer

       while (!match(TOKEN_EOF)) {
           #ifdef DEBUG
           printf("Parsing token: type=%d, value='%s'\n", 
                  current_token.type, 
                  current_token.value ? current_token.value : "");
           #endif
           
           if (match(TOKEN_KEYWORD)) {
               if (strcmp(current_token.value, "struct") == 0) {
                   parse_struct_declaration(input, program_node);
                   continue;
               }
               
               // Primitive or known type function
               Token return_type = current_token;
               advance(input);
               parse_function_declaration(input, return_type, program_node);
           } else {
               advance(input); // Skip unknown token
           }
       }
       
       return program_node;
   }

**Helper Functions:**

.. code-block:: c

   // Parse struct declaration or function returning struct
   static ASTNode* parse_struct_declaration(FILE *input, ASTNode *program_node);
   
   // Parse function declaration with primitive return type
   static ASTNode* parse_function_declaration(FILE *input, Token return_type, 
                                              ASTNode *program_node);
   
   // Skip tokens until semicolon or EOF
   static void skip_to_semicolon(FILE *input);

Expression Parsing
------------------

Expression parsing is implemented in ``src/parser/parse_expression.c`` using a **precedence climbing algorithm**.

parse_expression()
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_expression(FILE *input);

Entry point for expression parsing. Delegates to ``parse_expression_prec()`` with minimum precedence:

.. code-block:: c

   ASTNode* parse_expression(FILE *input) { 
       return parse_expression_prec(input, TOP_LEVEL_EXPR_MIN_PRECEDENCE);
   }

The ``TOP_LEVEL_EXPR_MIN_PRECEDENCE`` constant is set to ``-2`` to include all operators (including logical OR at precedence -2).

parse_expression_prec()
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_expression_prec(FILE *input, int min_prec);

Implements **precedence climbing** to parse binary expressions with correct operator precedence:

.. code-block:: c

   ASTNode* parse_expression_prec(FILE *input, int min_prec) {
       ASTNode *left_operand = parse_primary(input);
       if (!left_operand) {
           return NULL;
       }
       
       while (match(TOKEN_OPERATOR)) {
           const char *operator = current_token.value;
           int operator_precedence = get_precedence(operator);
           
           if (operator_precedence < min_prec) {
               break;  // Precedence too low, stop
           }
           
           char operator_copy[8] = {0};
           safe_copy(operator_copy, sizeof(operator_copy), operator);
           advance(input);  // Consume operator
           
           ASTNode *right_operand = parse_expression_prec(input, operator_precedence + 1);
           if (!right_operand) {
               printf("Error: Expected right operand after operator '%s'\n", operator_copy);
               exit(EXIT_FAILURE);
           }
           
           // Build binary expression node
           ASTNode *binary_expr = create_node(NODE_BINARY_EXPR);
           binary_expr->value = strdup(operator_copy);
           add_child(binary_expr, left_operand);
           add_child(binary_expr, right_operand);
           left_operand = binary_expr;  // New left side for next iteration
       }
       
       return left_operand;
   }

**Algorithm:**

1. Parse left operand using ``parse_primary()``
2. While current token is an operator with precedence ≥ ``min_prec``:
   
   - Get operator and its precedence
   - Recursively parse right operand with precedence ``prec + 1``
   - Build binary expression node with left and right children
   - Set result as new left operand

3. Return the accumulated left expression

**Precedence handling:**

The ``get_precedence()`` function (in ``src/core/utils.c``) assigns numeric precedence values to operators:

.. code-block:: c

   int get_precedence(const char *op) {
       if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) return 7;
       if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 6;
       if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) return 5;
       if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
           strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) return 4;
       if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) return 3;
       if (strcmp(op, "&") == 0) return 2;
       if (strcmp(op, "^") == 0) return 1;
       if (strcmp(op, "|") == 0) return 0;
       if (strcmp(op, "&&") == 0) return -1;
       if (strcmp(op, "||") == 0) return -2;
       return -1000;  // Unknown operator
   }

**Precedence table** (higher number = higher precedence):

========= =================================== ===========
Prec      Operators                           Assoc
========= =================================== ===========
7         ``*``, ``/``                        Left
6         ``+``, ``-``                        Left
5         ``<<``, ``>>``                      Left
4         ``<``, ``<=``, ``>``, ``>=``        Left
3         ``==``, ``!=``                      Left
2         ``&``                               Left
1         ``^``                               Left
0         ``|``                               Left
-1        ``&&``                              Left
-2        ``||``                              Left
========= =================================== ===========

.. note::
   All operators are **left-associative**. The ``prec + 1`` in the recursive call ensures left associativity.

parse_primary()
~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_primary(FILE *input);

Parses primary expressions (terminals and unary operations).

**Helper Functions:**

.. code-block:: c

   static ASTNode* parse_logical_not(FILE *input);
   static ASTNode* parse_bitwise_not(FILE *input);
   static ASTNode* parse_unary_minus(FILE *input);
   static ASTNode* parse_parenthesized_expr(FILE *input);
   static void parse_field_access(FILE *input, char *identifier_buffer, size_t buffer_size);
   static void parse_array_index(FILE *input, char *index_buffer, size_t buffer_size);
   static void validate_array_bounds(const char *identifier_name, const char *index_expr);
   static ASTNode* parse_identifier(FILE *input);
   static ASTNode* parse_number(FILE *input);

**Main Function:**

.. code-block:: c

   ASTNode* parse_primary(FILE *input) {
       // Unary operators
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "!") == 0) {
           return parse_logical_not(input);
       }
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "~") == 0) {
           return parse_bitwise_not(input);
       }
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "-") == 0) {
           return parse_unary_minus(input);
       }
       
       // Parenthesized expressions
       if (match(TOKEN_PARENTHESIS_OPEN)) {
           return parse_parenthesized_expr(input);
       }
       
       // Identifiers (with field access and array indexing)
       if (match(TOKEN_IDENTIFIER)) {
           return parse_identifier(input);
       }
       
       // Number literals
       if (match(TOKEN_NUMBER)) {
           return parse_number(input);
       }
       
       return NULL;
   }

**Supported primary expressions:**

1. **Unary operators**: ``!``, ``~``, ``-``
2. **Parenthesized expressions**: ``(expr)``
3. **Identifiers**: ``foo``
4. **Struct field access**: ``foo.bar.baz`` (converted to ``foo__bar__baz``)
5. **Array subscripting**: ``arr[index]`` with bounds checking
6. **Numbers**: ``42``, ``3.14``

**Helper Function Examples:**

.. code-block:: c

   // Logical NOT: !expr
   static ASTNode* parse_logical_not(FILE *input) {
       advance(input);  // consume '!'
       ASTNode *operand = parse_primary(input);
       if (!operand) {
           return NULL;
       }
       
       ASTNode *not_node = create_node(NODE_BINARY_OP);
       not_node->value = strdup("!");
       add_child(not_node, operand);
       return not_node;
   }
   
   // Array bounds validation
   static void validate_array_bounds(const char *identifier_name, const char *index_expr) {
       if (!is_number_str(index_expr)) {
           return;  // Dynamic index, cannot validate at parse time
       }
       
       int index_value = atoi(index_expr);
       int array_size = find_array_size(identifier_name);
       
       if (array_size > 0 && (index_value < 0 || index_value >= array_size)) {
           printf("Error (line %d): Array index %d out of bounds for '%s' with size %d\n",
                  current_token.line, index_value, identifier_name, array_size);
           exit(EXIT_FAILURE);
       }
   }

**Field access flattening:**

Struct field access like ``foo.bar.baz`` is converted to the flat identifier ``foo__bar__baz`` at parse time. This simplification allows the code generator to treat all identifiers uniformly.

Statement Parsing
-----------------

Statement parsing is implemented in ``src/parser/parse_statement.c``.

parse_statement()
~~~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_statement(FILE *input);

Parses individual statements and returns a ``NODE_STATEMENT`` AST node containing the actual statement as a child.

**Helper Functions:**

.. code-block:: c

   static ASTNode* parse_initializer_list(FILE *input, int is_array);
   static ASTNode* parse_variable_declaration(FILE *input, Token type_token);
   static void parse_lhs_expression(FILE *input, Token lhs_token, 
                                     char *lhs_buf, size_t lhs_buf_size,
                                     char *idx_buf, size_t idx_buf_size, 
                                     char *base_name, size_t base_name_size);
   static ASTNode* parse_assignment_or_expression(FILE *input);
   static ASTNode* parse_return_statement(FILE *input);
   static void parse_else_blocks(FILE *input, ASTNode *if_node);
   static ASTNode* parse_if_statement(FILE *input);
   static ASTNode* parse_while_statement(FILE *input);
   static ASTNode* parse_for_init(FILE *input);
   static ASTNode* parse_for_increment(FILE *input);
   static ASTNode* parse_for_statement(FILE *input);
   static ASTNode* parse_break_statement(FILE *input);
   static ASTNode* parse_continue_statement(FILE *input);

**Main Function:**

.. code-block:: c

   ASTNode* parse_statement(FILE *input) {
       ASTNode *stmt_node = create_node(NODE_STATEMENT);
       ASTNode *sub_statement = NULL;
       
       // Variable declaration
       if (match(TOKEN_KEYWORD) && (strcmp(current_token.value, "int") == 0 ||
                                     strcmp(current_token.value, "float") == 0 ||
                                     strcmp(current_token.value, "char") == 0 ||
                                     strcmp(current_token.value, "double") == 0 ||
                                     strcmp(current_token.value, "struct") == 0)) {
           Token type_token = current_token;
           advance(input);
           sub_statement = parse_variable_declaration(input, type_token);
           if (sub_statement) {
               add_child(stmt_node, sub_statement);
           }
           return stmt_node;
       }
       
       // Assignment or expression
       if (match(TOKEN_IDENTIFIER)) {
           sub_statement = parse_assignment_or_expression(input);
           if (sub_statement) {
               add_child(stmt_node, sub_statement);
           }
           return stmt_node;
       }
       
       // Return statement
       if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "return") == 0) {
           return parse_return_statement(input);
       }
       
       // If statement
       if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "if") == 0) {
           sub_statement = parse_if_statement(input);
           if (sub_statement) {
               add_child(stmt_node, sub_statement);
           }
           return stmt_node;
       }
       
       // While/for/break/continue statements...
       // (similar dispatch pattern)
       
       return stmt_node;
   }

**Supported statement types:**

1. **Variable declarations**: ``int x;``, ``int arr[10];``, ``struct Foo f;``
2. **Assignments**: ``x = expr;``, ``arr[i] = expr;``, ``foo.bar = expr;``
3. **Control flow**: ``if``, ``else if``, ``else``, ``while``, ``for``
4. **Loop control**: ``break``, ``continue``
5. **Return**: ``return expr;``

Variable Declarations
~~~~~~~~~~~~~~~~~~~~~

Variable declarations are handled by the ``parse_variable_declaration()`` helper function:

.. code-block:: c

   static ASTNode* parse_variable_declaration(FILE *input, Token type_token) {
       Token name_token = {0};
       ASTNode *var_decl_node = NULL;
       ASTNode *init_expr = NULL;
       ASTNode *init_list = NULL;
       int is_struct = 0;
       int is_array = 0;
       char arr_size_buf[ARRAY_SIZE_BUFFER_SIZE] = {0};
       char buf[GENERAL_BUFFER_SIZE] = {0};
       
       // Check if it's a struct type
       if (strcmp(type_token.value, "struct") == 0) {
           if (!match(TOKEN_IDENTIFIER)) {
               printf("Error (line %d): Expected struct name after 'struct'\n", 
                      current_token.line);
               exit(EXIT_FAILURE);
           }
           type_token = current_token;
           advance(input);
           is_struct = 1;
       }
       
       // Get variable name
       if (!match(TOKEN_IDENTIFIER)) {
           printf("Error (line %d): Expected variable name after type\n", 
                  current_token.line);
           exit(EXIT_FAILURE);
       }
       
       name_token = current_token;
       advance(input);
       
       var_decl_node = create_node(NODE_VAR_DECL);
       var_decl_node->token = type_token;
       var_decl_node->value = strdup(name_token.value);
       
       // Handle array declaration
       if (match(TOKEN_BRACKET_OPEN)) {
           is_array = 1;
           advance(input);
           
           if (!match(TOKEN_NUMBER)) {
               printf("Error (line %d): Expected array size after '['\n", 
                      current_token.line);
               exit(EXIT_FAILURE);
           }
           
           snprintf(arr_size_buf, sizeof(arr_size_buf), "%s", current_token.value);
           snprintf(buf, sizeof(buf), "%s[%s]", name_token.value, current_token.value);
           free(var_decl_node->value);
           var_decl_node->value = strdup(buf);
           advance(input);
           
           register_array(name_token.value, atoi(arr_size_buf));
           
           if (!consume(input, TOKEN_BRACKET_CLOSE)) {
               printf("Error (line %d): Expected ']' after array size\n", 
                      current_token.line);
               exit(EXIT_FAILURE);
           }
       }
       
       // Handle initialization
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
           advance(input);
           
           if (is_array && match(TOKEN_BRACE_OPEN)) {
               init_list = parse_initializer_list(input, 1);
               add_child(var_decl_node, init_list);
           } else if (is_struct && match(TOKEN_BRACE_OPEN)) {
               init_list = parse_initializer_list(input, 0);
               add_child(var_decl_node, init_list);
           } else {
               init_expr = parse_expression(input);
               if (init_expr) {
                   add_child(var_decl_node, init_expr);
               }
               while (!match(TOKEN_SEMICOLON) && !match(TOKEN_EOF)) {
                   advance(input);
               }
           }
       }
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error (line %d): Expected ';' after variable declaration\n", 
                  current_token.line);
           exit(EXIT_FAILURE);
       }
       
       return var_decl_node;
   }

**Array registration:**

When an array declaration is parsed, ``register_array()`` is called to record the array name and size in a global symbol table. This enables bounds checking during parsing.

Assignments
~~~~~~~~~~~

Assignments are handled by the ``parse_assignment_or_expression()`` helper function, which delegates left-hand side parsing to ``parse_lhs_expression()``:

.. code-block:: c

   static ASTNode* parse_assignment_or_expression(FILE *input) {
       Token lhs_token = current_token;
       ASTNode *assign_node = NULL;
       ASTNode *lhs_expr = NULL;
       ASTNode *rhs_node = NULL;
       char lhs_buf[LHS_BUFFER_SIZE] = {0};
       char idx_buf[INDEX_BUFFER_SIZE] = {0};
       char base_name[BASE_NAME_BUFFER_SIZE] = {0};
       
       advance(input);
       
       parse_lhs_expression(input, lhs_token, lhs_buf, sizeof(lhs_buf),
                           idx_buf, sizeof(idx_buf), base_name, sizeof(base_name));
       
       lhs_expr = create_node(NODE_EXPRESSION);
       lhs_expr->value = strdup(lhs_buf);
       
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
           advance(input);
           assign_node = create_node(NODE_ASSIGNMENT);
           add_child(assign_node, lhs_expr);
           
           rhs_node = parse_expression(input);
           if (rhs_node) {
               add_child(assign_node, rhs_node);
           }
           
           if (!consume(input, TOKEN_SEMICOLON)) {
               printf("Error (line %d): Expected ';' after assignment\n", 
                      current_token.line);
               exit(EXIT_FAILURE);
           }
           
           return assign_node;
       } else {
           // Not an assignment, skip to semicolon
           while (!match(TOKEN_SEMICOLON) && !match(TOKEN_EOF)) {
               advance(input);
           }
           if (match(TOKEN_SEMICOLON)) {
               advance(input);
           }
           free_node(lhs_expr);
           return NULL;
       }
   }

The ``parse_lhs_expression()`` helper handles field access (``foo.bar`` → ``foo__bar``) and array indexing (``arr[i]``) with bounds validation for constant indices.

Control Flow Statements
~~~~~~~~~~~~~~~~~~~~~~~

The parser handles if, while, and for statements through dedicated helper functions.

**If Statements**

``parse_if_statement()`` handles if/else-if/else chains:

.. code-block:: c

   static ASTNode* parse_if_statement(FILE *input) {
       advance(input);
       
       if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
           printf("Error: Expected '(' after 'if'\n");
           exit(EXIT_FAILURE);
       }
       
       ASTNode *cond_expr = parse_expression(input);
       
       if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
           printf("Error: Expected ')' after if condition\n");
           exit(EXIT_FAILURE);
       }
       if (!consume(input, TOKEN_BRACE_OPEN)) {
           printf("Error: Expected '{' after if condition\n");
           exit(EXIT_FAILURE);
       }
       
       ASTNode *if_node = create_node(NODE_IF_STATEMENT);
       if (cond_expr) {
           add_child(if_node, cond_expr);
       }
       
       while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
           ASTNode *inner_stmt = parse_statement(input);
           if (inner_stmt) {
               add_child(if_node, inner_stmt);
           }
       }
       
       consume(input, TOKEN_BRACE_CLOSE);
       parse_else_blocks(input, if_node);  // Handles else-if and else
       
       return if_node;
   }

The ``parse_else_blocks()`` helper handles chained else-if and final else blocks.

**While Statements**

``parse_while_statement()`` handles loop syntax and tracks nesting depth:

.. code-block:: c

   static ASTNode* parse_while_statement(FILE *input) {
       advance(input);
       
       consume(input, TOKEN_PARENTHESIS_OPEN);
       ASTNode *cond_expr = parse_expression(input);
       consume(input, TOKEN_PARENTHESIS_CLOSE);
       consume(input, TOKEN_BRACE_OPEN);
       
       ASTNode *while_node = create_node(NODE_WHILE_STATEMENT);
       if (cond_expr) {
           add_child(while_node, cond_expr);
       }
       
       s_loop_depth++;  // Track loop nesting for break/continue validation
       while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
           ASTNode *inner_stmt = parse_statement(input);
           if (inner_stmt) {
               add_child(while_node, inner_stmt);
           }
       }
       s_loop_depth--;
       
       consume(input, TOKEN_BRACE_CLOSE);
       return while_node;
   }

**For Statements**

``parse_for_statement()`` handles initialization, condition, and increment:

.. code-block:: c

   static ASTNode* parse_for_statement(FILE *input) {
       advance(input);
       consume(input, TOKEN_PARENTHESIS_OPEN);
       
       // Parse initialization (var decl or assignment)
       ASTNode *init_node = parse_for_init(input);
       if (match(TOKEN_SEMICOLON)) {
           advance(input);
       }
       
       // Parse condition
       ASTNode *cond_expr = NULL;
       if (!match(TOKEN_SEMICOLON)) {
           cond_expr = parse_expression(input);
       }
       consume(input, TOKEN_SEMICOLON);
       
       // Parse increment (i++, i--, i = i + 1, etc.)
       ASTNode *incr_expr = parse_for_increment(input);
       consume(input, TOKEN_PARENTHESIS_CLOSE);
       consume(input, TOKEN_BRACE_OPEN);
       
       ASTNode *for_node = create_node(NODE_FOR_STATEMENT);
       if (init_node) add_child(for_node, init_node);
       if (cond_expr) {
           add_child(for_node, cond_expr);
       } else {
           // No condition means infinite loop: for(;;)
           ASTNode *true_expr = create_node(NODE_EXPRESSION);
           true_expr->value = strdup("1");
           add_child(for_node, true_expr);
       }
       if (incr_expr) add_child(for_node, incr_expr);
       
       s_loop_depth++;
       while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
           ASTNode *inner = parse_statement(input);
           if (inner) {
               add_child(for_node, inner);
           }
       }
       s_loop_depth--;
       
       consume(input, TOKEN_BRACE_CLOSE);
       return for_node;
   }

The ``parse_for_init()`` helper handles both variable declarations (``int i = 0``) and assignments (``i = 0``). The ``parse_for_increment()`` helper supports ``i++``, ``i--``, and ``i = expr`` forms.

**For loop AST structure:**

.. code-block:: text

   NODE_FOR_STATEMENT
     ├─ init_node (child 0) - NODE_VAR_DECL or NODE_ASSIGNMENT
     ├─ cond_expr (child 1) - condition expression
     ├─ body_stmt (child 2)
     ├─ body_stmt (child 3)
     ├─ ...
     └─ incr_expr (last child) - increment expression

Break and Continue
~~~~~~~~~~~~~~~~~~

Break and continue statements track loop nesting depth to validate they appear only within loops:

.. code-block:: c

   static int s_loop_depth = 0;  // File-static loop nesting counter
   
   // In while/for parsing:
   s_loop_depth++;
   // ... parse loop body
   s_loop_depth--;
   
   // Break:
   if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "break") == 0) {
       if (s_loop_depth <= 0) {
           printf("Error: 'break' not within a loop\n");
           exit(EXIT_FAILURE);
       }
       advance(input);
       consume(input, TOKEN_SEMICOLON);
       br = create_node(NODE_BREAK_STATEMENT);
       add_child(stmt_node, br);
       return stmt_node;
   }
   
   // Continue:
   if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "continue") == 0) {
       if (s_loop_depth <= 0) {
           printf("Error: 'continue' not within a loop\n");
           exit(EXIT_FAILURE);
       }
       advance(input);
       consume(input, TOKEN_SEMICOLON);
       cn = create_node(NODE_CONTINUE_STATEMENT);
       add_child(stmt_node, cn);
       return stmt_node;
   }

Return Statements
~~~~~~~~~~~~~~~~~

Handled by the ``parse_return_statement()`` helper function:

.. code-block:: c

   static ASTNode* parse_return_statement(FILE *input) {
       ASTNode *stmt_node = create_node(NODE_STATEMENT);
       stmt_node->token = current_token;
       advance(input);
       
       ASTNode *return_expr = parse_expression(input);
       if (return_expr) {
           add_child(stmt_node, return_expr);
       }
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error (line %d): Expected ';' after return statement\n", 
                  current_token.line);
           exit(EXIT_FAILURE);
       }
       
       return stmt_node;
   }

Function Parsing
----------------

Function parsing is implemented in ``src/parser/parse_function.c``.

parse_function()
~~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_function(FILE *input, Token return_type, Token function_name);

Parses a complete function declaration including parameters and body.

**Helper Functions:**

.. code-block:: c

   static ASTNode* parse_single_parameter(FILE *input, Token *parameter_type);
   static void parse_function_parameters(FILE *input, ASTNode *function_node);
   static void parse_function_body(FILE *input, ASTNode *function_node);

**Implementation:**

.. code-block:: c

   ASTNode* parse_function(FILE *input, Token return_type, Token function_name) {
       // Initialize function node
       ASTNode *function_node = create_node(NODE_FUNCTION_DECL);
       function_node->token = return_type;
       function_node->value = strdup(function_name.value);
       
       // Reset global array count for this function scope
       g_array_count = 0;
       
       // Parse function parameters
       parse_function_parameters(input, function_node);
       
       // Parse function body
       parse_function_body(input, function_node);
       
       return function_node;
   }

**Parameter Parsing:**

.. code-block:: c

   static void parse_function_parameters(FILE *input, ASTNode *function_node) {
       if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
           printf("Error: Expected '(' after function name\n");
           exit(EXIT_FAILURE);
       }
       
       while (!match(TOKEN_PARENTHESIS_CLOSE) && !match(TOKEN_EOF)) {
           if (match(TOKEN_KEYWORD)) {
               Token parameter_type;
               ASTNode *parameter_node = parse_single_parameter(input, &parameter_type);
               if (parameter_node) {
                   add_child(function_node, parameter_node);
               }
               
               if (match(TOKEN_COMMA)) {
                   advance(input);
               }
           } else {
               advance(input);
           }
       }
       
       if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
           printf("Error: Expected ')' after parameter list\n");
           exit(EXIT_FAILURE);
       }
   }

The ``parse_single_parameter()`` helper handles both primitive types (``int x``) and struct types (``struct Point p``).

Struct Parsing
--------------

Struct parsing is implemented in ``src/parser/parse_struct.c``.

parse_struct()
~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_struct(FILE *input, Token struct_name_token);

Parses a complete struct definition: ``struct Name { type field; ... };``

**Helper Functions:**

.. code-block:: c

   static void register_struct_in_table(int struct_index, Token struct_name_token);
   static void register_field_in_struct(int struct_index, Token field_type, Token field_name);
   static ASTNode* parse_struct_field(FILE *input, int struct_index);

**Implementation:**

.. code-block:: c

   ASTNode* parse_struct(FILE *input, Token struct_name_token) {
       if (!consume(input, TOKEN_BRACE_OPEN)) {
           printf("Error (line %d): Expected '{' after struct name\n", 
                  current_token.line);
           return NULL;
       }
       
       ASTNode *struct_node = create_node(NODE_STRUCT_DECL);
       struct_node->value = strdup(struct_name_token.value);
       
       int struct_index = g_struct_count;
       register_struct_in_table(struct_index, struct_name_token);
       
       // Parse all fields
       while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
           ASTNode *field_node = parse_struct_field(input, struct_index);
           if (field_node) {
               add_child(struct_node, field_node);
           } else {
               advance(input);  // Skip unknown tokens
           }
       }
       
       consume(input, TOKEN_BRACE_CLOSE);
       consume(input, TOKEN_SEMICOLON);
       
       if (struct_index == g_struct_count) {
           g_struct_count++;
       }
       
       return struct_node;
   }

**Field Parsing:**

.. code-block:: c

   static ASTNode* parse_struct_field(FILE *input, int struct_index) {
       if (!match(TOKEN_KEYWORD)) {
           return NULL;
       }
       
       Token field_type = current_token;
       advance(input);
       
       if (!match(TOKEN_IDENTIFIER)) {
           printf("Error (line %d): Expected field name in struct\n", 
                  current_token.line);
           exit(EXIT_FAILURE);
       }
       
       Token field_name = current_token;
       advance(input);
       
       ASTNode *field_node = create_node(NODE_VAR_DECL);
       field_node->token = field_type;
       field_node->value = strdup(field_name.value);
       
       register_field_in_struct(struct_index, field_type, field_name);
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error (line %d): Expected ';' after struct field\n", 
                  current_token.line);
           exit(EXIT_FAILURE);
       }
       
       return field_node;
   }

**Global Symbol Table:**

Structs are registered in a global symbol table during parsing:

.. code-block:: c

   extern StructInfo g_structs[MAX_STRUCTS];
   extern int g_struct_count;

This enables validation of struct field access and proper type information for code generation.

Error Handling
--------------

The parser has **aggressive error handling**:

* **Expected token errors**: Calls ``exit(EXIT_FAILURE)`` when required tokens are missing
* **Bounds checking**: Validates array indices at parse time if they are constants
* **Loop context validation**: Ensures ``break``/``continue`` appear only in loops
* **Warnings**: Prints warnings for unimplemented features (e.g., global variables)

**Example error messages:**

.. code-block:: text

   Error (line 42): Expected ')' after expression
   Error (line 15): Array index 10 out of bounds for 'arr' with size 5
   Error (line 8): 'break' not within a loop

Limitations
-----------

**No semantic analysis:**

* Type checking is deferred to later phases
* No symbol table maintenance during parsing (except for array bounds)
* No validation of struct field names

**Limited error recovery:**

* Parse errors cause immediate program termination
* No attempt to synchronize and continue parsing
* Single error reported per invocation

**Grammar restrictions:**

* Braces required for all control structures (no single-statement bodies)
* No switch statements
* No do-while loops
* No ternary operator (``? :``)
* No comma operator
* No function pointers
* No typedefs

Summary
-------

The parser is a **modular recursive descent implementation** that:

* Uses **precedence climbing** for expression parsing
* Provides **clear separation** between syntactic categories (expressions, statements, functions, structs)
* Implements **early validation** (array bounds, loop context)
* Flattens struct field access at parse time
* Uses global token stream state for simplicity
* Terminates aggressively on errors

The implementation prioritizes **correctness and clarity**, handling the C subset required by the compiler with clean, well-organized code.

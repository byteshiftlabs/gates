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
       return parse_expression_prec(input, -2); 
   }

parse_expression_prec()
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* parse_expression_prec(FILE *input, int min_prec);

Implements **precedence climbing** to parse binary expressions with correct operator precedence:

.. code-block:: c

   ASTNode* parse_expression_prec(FILE *input, int min_prec) {
       ASTNode *left = parse_primary(input);
       if (!left) return NULL;
       
       while (match(TOKEN_OPERATOR)) {
           const char *op = current_token.value;
           int prec = get_precedence(op);
           
           if (prec < min_prec) break;  // Precedence too low, stop
           
           advance(input);  // Consume operator
           
           // Right-associative: use prec+1 for left-associative operators
           ASTNode *right = parse_expression_prec(input, prec + 1);
           
           // Build binary expression node
           ASTNode *bin = create_node(NODE_BINARY_EXPR);
           bin->value = strdup(op);
           add_child(bin, left);
           add_child(bin, right);
           left = bin;  // New left side for next iteration
       }
       return left;
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
   static ASTNode* parse_field_access(FILE *input, const char *base_identifier);
   static ASTNode* parse_array_index(FILE *input, const char *base_name);
   static void validate_array_bounds(const char *array_name, const char *index_str);
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
       ASTNode *not_node = create_node(NODE_BINARY_OP);
       not_node->value = strdup("!");
       add_child(not_node, operand);
       return not_node;
   }
   
   // Array bounds validation
   static void validate_array_bounds(const char *array_name, const char *index_str) {
       if (!is_number_str(index_str)) {
           return;  // Dynamic index, cannot validate at parse time
       }
       
       int index_value = atoi(index_str);
       int array_size = find_array_size(array_name);
       
       if (array_size > 0) {
           if (index_value < 0 || index_value >= array_size) {
               printf("Error (line %d): Array index %d out of bounds for '%s' (size %d)\n",
                      current_token.line, index_value, array_name, array_size);
               exit(EXIT_FAILURE);
           }
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
   static ASTNode* parse_lhs_expression(FILE *input, Token lhs_token, 
                                        char *lhs_buffer, size_t buffer_size);
   static ASTNode* parse_assignment_or_expression(FILE *input);
   static ASTNode* parse_return_statement(FILE *input);
   static ASTNode* parse_else_blocks(FILE *input, ASTNode *if_node);
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

Variable declarations are recognized when the current token is a type keyword (``int``, ``float``, ``char``, ``double``, ``struct``):

.. code-block:: c

   if (match(TOKEN_KEYWORD) && (
       strcmp(current_token.value, "int") == 0 ||
       strcmp(current_token.value, "float") == 0 ||
       strcmp(current_token.value, "char") == 0 ||
       strcmp(current_token.value, "double") == 0 ||
       strcmp(current_token.value, "struct") == 0)) {
       
       type_token = current_token;
       advance(input);
       
       // Handle "struct Name" type
       if (strcmp(type_token.value, "struct") == 0) {
           type_token = current_token;  // Struct name becomes the type
           advance(input);
       }
       
       // Get variable name
       name_token = current_token;
       advance(input);
       
       // Create variable declaration node
       var_decl_node = create_node(NODE_VAR_DECL);
       var_decl_node->token = type_token;
       var_decl_node->value = strdup(name_token.value);
       
       // Handle array declaration: int arr[SIZE];
       if (match(TOKEN_BRACKET_OPEN)) {
           advance(input);
           if (match(TOKEN_NUMBER)) {
               char buf[256];
               snprintf(buf, sizeof(buf), "%s[%s]", name_token.value, current_token.value);
               free(var_decl_node->value);
               var_decl_node->value = strdup(buf);
               register_array(name_token.value, atoi(current_token.value));
               advance(input);
           }
           consume(input, TOKEN_BRACKET_CLOSE);
       }
       
       // Handle initialization: int x = 42;
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
           advance(input);
           
           if (is_array && match(TOKEN_BRACE_OPEN)) {
               // Array initializer: int arr[3] = {1, 2, 3};
               advance(input);
               init_list = create_node(NODE_EXPRESSION);
               init_list->value = strdup("array_init");
               while (!match(TOKEN_BRACE_CLOSE)) {
                   if (match(TOKEN_NUMBER) || match(TOKEN_IDENTIFIER)) {
                       elem = create_node(NODE_EXPRESSION);
                       elem->value = strdup(current_token.value);
                       add_child(init_list, elem);
                       advance(input);
                   } else if (match(TOKEN_COMMA)) {
                       advance(input);
                   }
               }
               consume(input, TOKEN_BRACE_CLOSE);
               add_child(var_decl_node, init_list);
           } else if (is_struct && match(TOKEN_BRACE_OPEN)) {
               // Struct initializer: struct Foo f = {1, 2};
               // (same logic as array initializer)
           } else {
               // Scalar initializer: int x = 42;
               init_expr = parse_expression(input);
               add_child(var_decl_node, init_expr);
           }
       }
       
       consume(input, TOKEN_SEMICOLON);
       add_child(stmt_node, var_decl_node);
       return stmt_node;
   }

**Array registration:**

When an array declaration is parsed, ``register_array()`` is called to record the array name and size in a global symbol table. This enables bounds checking during parsing.

Assignments
~~~~~~~~~~~

Assignments are recognized when an identifier is followed by ``=``:

.. code-block:: c

   if (match(TOKEN_IDENTIFIER)) {
       lhs_token = current_token;
       advance(input);
       
       char lhs_buf[1024] = {0};
       strcpy(lhs_buf, lhs_token.value);
       
       // Handle field access: foo.bar = expr;
       while (match(TOKEN_OPERATOR) && strcmp(current_token.value, ".") == 0) {
           advance(input);
           if (!match(TOKEN_IDENTIFIER)) {
               printf("Error: Expected field name after '.'\n");
               exit(EXIT_FAILURE);
           }
           strcat(lhs_buf, "__");
           strcat(lhs_buf, current_token.value);
           advance(input);
       }
       
       // Handle array subscript: arr[i] = expr;
       if (match(TOKEN_BRACKET_OPEN)) {
           advance(input);
           char idx_buf[512] = {0};
           // Collect index expression
           while (!match(TOKEN_BRACKET_CLOSE)) {
               strcat(idx_buf, current_token.value);
               advance(input);
           }
           consume(input, TOKEN_BRACKET_CLOSE);
           strcat(lhs_buf, "[");
           strcat(lhs_buf, idx_buf);
           strcat(lhs_buf, "]");
           
           // Bounds check if constant index
           if (is_number_str(idx_buf)) {
               int idx = atoi(idx_buf);
               int arr_size = find_array_size(base_name);
               if (arr_size > 0 && (idx < 0 || idx >= arr_size)) {
                   printf("Error: Array index out of bounds\n");
                   exit(EXIT_FAILURE);
               }
           }
       }
       
       // Check for assignment operator
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
           advance(input);
           
           assign_node = create_node(NODE_ASSIGNMENT);
           lhs_expr = create_node(NODE_EXPRESSION);
           lhs_expr->value = strdup(lhs_buf);
           add_child(assign_node, lhs_expr);
           
           rhs_node = parse_expression(input);
           add_child(assign_node, rhs_node);
           
           consume(input, TOKEN_SEMICOLON);
           add_child(stmt_node, assign_node);
           return stmt_node;
       }
   }

Control Flow Statements
~~~~~~~~~~~~~~~~~~~~~~~

**If statements:**

.. code-block:: c

   if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "if") == 0) {
       advance(input);
       consume(input, TOKEN_PARENTHESIS_OPEN);
       cond_expr = parse_expression(input);
       consume(input, TOKEN_PARENTHESIS_CLOSE);
       consume(input, TOKEN_BRACE_OPEN);
       
       if_node = create_node(NODE_IF_STATEMENT);
       add_child(if_node, cond_expr);  // Condition is first child
       
       // Parse if body
       while (!match(TOKEN_BRACE_CLOSE)) {
           inner_stmt = parse_statement(input);
           add_child(if_node, inner_stmt);
       }
       consume(input, TOKEN_BRACE_CLOSE);
       
       // Handle else if / else
       while (match(TOKEN_KEYWORD) && strcmp(current_token.value, "else") == 0) {
           advance(input);
           if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "if") == 0) {
               // else if
               advance(input);
               consume(input, TOKEN_PARENTHESIS_OPEN);
               elseif_cond = parse_expression(input);
               consume(input, TOKEN_PARENTHESIS_CLOSE);
               consume(input, TOKEN_BRACE_OPEN);
               
               elseif_node = create_node(NODE_ELSE_IF_STATEMENT);
               add_child(elseif_node, elseif_cond);
               while (!match(TOKEN_BRACE_CLOSE)) {
                   inner_stmt = parse_statement(input);
                   add_child(elseif_node, inner_stmt);
               }
               consume(input, TOKEN_BRACE_CLOSE);
               add_child(if_node, elseif_node);
           } else {
               // else
               consume(input, TOKEN_BRACE_OPEN);
               else_node = create_node(NODE_ELSE_STATEMENT);
               while (!match(TOKEN_BRACE_CLOSE)) {
                   inner_stmt = parse_statement(input);
                   add_child(else_node, inner_stmt);
               }
               consume(input, TOKEN_BRACE_CLOSE);
               add_child(if_node, else_node);
               break;  // else must be last
           }
       }
       
       add_child(stmt_node, if_node);
       return stmt_node;
   }

**AST structure for if/else if/else:**

.. code-block:: text

   NODE_IF_STATEMENT
     ├─ condition_expr (child 0)
     ├─ if_body_stmt (child 1)
     ├─ if_body_stmt (child 2)
     ├─ NODE_ELSE_IF_STATEMENT (child 3)
     │    ├─ elseif_condition_expr
     │    └─ elseif_body_stmts...
     └─ NODE_ELSE_STATEMENT (child 4)
          └─ else_body_stmts...

**While loops:**

.. code-block:: c

   if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "while") == 0) {
       advance(input);
       consume(input, TOKEN_PARENTHESIS_OPEN);
       cond_expr = parse_expression(input);
       consume(input, TOKEN_PARENTHESIS_CLOSE);
       consume(input, TOKEN_BRACE_OPEN);
       
       while_node = create_node(NODE_WHILE_STATEMENT);
       add_child(while_node, cond_expr);
       
       s_loop_depth++;  // Track loop nesting for break/continue validation
       while (!match(TOKEN_BRACE_CLOSE)) {
           inner_stmt = parse_statement(input);
           add_child(while_node, inner_stmt);
       }
       s_loop_depth--;
       
       consume(input, TOKEN_BRACE_CLOSE);
       add_child(stmt_node, while_node);
       return stmt_node;
   }

**For loops:**

For loops support both variable declarations and assignments in the initialization clause:

.. code-block:: c

   if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "for") == 0) {
       advance(input);
       consume(input, TOKEN_PARENTHESIS_OPEN);
       
       // Parse initialization (int i = 0 or i = 0)
       if (!match(TOKEN_SEMICOLON)) {
           if (match(TOKEN_KEYWORD)) {
               // Variable declaration: for (int i = 0; ...)
               init_stmt = parse_statement(input);
               init_node = init_stmt->children[0];
           } else if (match(TOKEN_IDENTIFIER)) {
               // Assignment: for (i = 0; ...)
               // ... build assignment node
           }
       }
       if (match(TOKEN_SEMICOLON)) advance(input);
       
       // Parse condition
       if (!match(TOKEN_SEMICOLON)) {
           cond_expr = parse_expression(input);
       }
       consume(input, TOKEN_SEMICOLON);
       
       // Parse increment (i++, i--, i = i + 1)
       if (!match(TOKEN_PARENTHESIS_CLOSE)) {
           if (match(TOKEN_IDENTIFIER)) {
               inc_lhs = current_token;
               advance(input);
               if (match(TOKEN_OPERATOR) && 
                   (strcmp(current_token.value, "++") == 0 || 
                    strcmp(current_token.value, "--") == 0)) {
                   // i++ or i--: convert to i = i + 1 or i = i - 1
                   incr_expr = create_node(NODE_ASSIGNMENT);
                   lhs = create_node(NODE_EXPRESSION);
                   lhs->value = strdup(inc_lhs.value);
                   add_child(incr_expr, lhs);
                   
                   rhs = create_node(NODE_BINARY_EXPR);
                   rhs->value = strdup(strcmp(current_token.value, "++") == 0 ? "+" : "-");
                   op_l = create_node(NODE_EXPRESSION);
                   op_l->value = strdup(inc_lhs.value);
                   op_r = create_node(NODE_EXPRESSION);
                   op_r->value = strdup("1");
                   add_child(rhs, op_l);
                   add_child(rhs, op_r);
                   add_child(incr_expr, rhs);
                   advance(input);
               }
           }
       }
       consume(input, TOKEN_PARENTHESIS_CLOSE);
       consume(input, TOKEN_BRACE_OPEN);
       
       // Build for loop node
       for_node = create_node(NODE_FOR_STATEMENT);
       if (init_node) add_child(for_node, init_node);
       if (cond_expr) {
           add_child(for_node, cond_expr);
       } else {
           // No condition means infinite loop (condition = true)
           true_expr = create_node(NODE_EXPRESSION);
           true_expr->value = strdup("1");
           add_child(for_node, true_expr);
       }
       
       // Parse body
       s_loop_depth++;
       while (!match(TOKEN_BRACE_CLOSE)) {
           inner = parse_statement(input);
           add_child(for_node, inner);
       }
       s_loop_depth--;
       consume(input, TOKEN_BRACE_CLOSE);
       
       // Add increment at the end
       if (incr_expr) add_child(for_node, incr_expr);
       
       add_child(stmt_node, for_node);
       return stmt_node;
   }

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

   static ASTNode* parse_single_parameter(FILE *input);
   static void parse_function_parameters(FILE *input, ASTNode *func_node);
   static void parse_function_body(FILE *input, ASTNode *func_node);

**Implementation:**

.. code-block:: c

   ASTNode* parse_function(FILE *input, Token return_type, Token function_name) {
       if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
           printf("Error: Expected '(' after function name\n");
           exit(EXIT_FAILURE);
       }
       
       ASTNode *func_node = create_node(NODE_FUNCTION);
       func_node->token = return_type;
       func_node->value = strdup(function_name.value);
       
       // Parse parameters
       parse_function_parameters(input, func_node);
       
       if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
           printf("Error: Expected ')' after parameters\n");
           exit(EXIT_FAILURE);
       }
       
       if (!consume(input, TOKEN_BRACE_OPEN)) {
           printf("Error: Expected '{' after function signature\n");
           exit(EXIT_FAILURE);
       }
       
       // Parse function body
       parse_function_body(input, func_node);
       
       if (!consume(input, TOKEN_BRACE_CLOSE)) {
           printf("Error: Expected '}' after function body\n");
           exit(EXIT_FAILURE);
       }
       
       return func_node;
   }

**Parameter Parsing:**

.. code-block:: c

   static void parse_function_parameters(FILE *input, ASTNode *func_node) {
       while (!match(TOKEN_PARENTHESIS_CLOSE) && !match(TOKEN_EOF)) {
           ASTNode *parameter_node = parse_single_parameter(input);
           if (parameter_node) {
               add_child(func_node, parameter_node);
           }
           
           if (match(TOKEN_COMMA)) {
               advance(input);
           }
       }
   }

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

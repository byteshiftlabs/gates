Parser Implementation
=====================

Overview
--------

The parser is a **recursive descent parser** implemented across multiple modules in ``src/parser/``. It consumes tokens from the lexer and constructs an Abstract Syntax Tree (AST) representing the structure of the C source code.

**Key responsibilities:**

* Parse C language constructs (functions, statements, expressions, structs)
* Build AST nodes and establish parent-child relationships
* Perform syntax validation and error reporting
* Handle operator precedence using precedence climbing
* Validate array bounds and loop control flow
* Flatten struct field access and array indexing into string representations

.. note::
   The parser uses a **modular design**, with separate files for different syntactic categories: programs, functions, structs, statements, and expressions.

Location
--------

The parser is split across multiple source files:

- Main parser: ``src/parser/parse.c`` / ``include/parse.h``
- Expressions: ``src/parser/parse_expression.c`` / ``include/parse_expression.h``
- Statements: ``src/parser/parse_statement.c`` / ``include/parse_statement.h``
- Functions: ``src/parser/parse_function.c`` / ``include/parse_function.h``
- Structs: ``src/parser/parse_struct.c`` / ``include/parse_struct.h``
- Utilities: ``src/core/utils.c`` / ``include/utils.h``

Parser Architecture
-------------------

The parser follows a **top-down recursive descent** design:

.. code-block:: text

   parse_program()
     ├─ parse_struct_declaration()
     │    └─ parse_struct()
     └─ parse_function_declaration()
          └─ parse_function()
               └─ parse_statement()
                    ├─ parse_variable_declaration()
                    ├─ parse_assignment_or_expression()
                    │    └─ parse_expression()
                    ├─ parse_if_statement()
                    │    └─ parse_statement() (recursive)
                    ├─ parse_while_statement()
                    │    └─ parse_statement() (recursive)
                    ├─ parse_for_statement()
                    │    └─ parse_statement() (recursive)
                    └─ parse_expression()
                         ├─ parse_expression_prec()
                         └─ parse_primary()

**Design principles:**

* **One function per grammar rule**: Each parsing function corresponds to a syntactic category
* **Mutual recursion**: Statements can contain expressions; expressions can contain parenthesized expressions
* **Lookahead**: Uses ``match()`` to check current token without consuming
* **Error recovery**: Most errors result in ``exit(EXIT_FAILURE)`` (minimal recovery)
* **Symbol tracking**: Registers arrays and structs during parsing for later validation

Grammar Overview
----------------

The parser recognizes a subset of C with the following grammar (simplified):

.. code-block:: text

   program         ::= (struct_decl | function_decl)*
   
   struct_decl     ::= 'struct' IDENTIFIER '{' field_decl* '}' ';'
   field_decl      ::= type IDENTIFIER ';'
   
   function_decl   ::= type IDENTIFIER '(' param_list ')' '{' statement* '}'
   param_list      ::= (param (',' param)*)?
   param           ::= type IDENTIFIER
   
   statement       ::= var_decl | assignment | return_stmt | if_stmt | 
                       while_stmt | for_stmt | break_stmt | continue_stmt
   
   var_decl        ::= type IDENTIFIER ('[' NUMBER ']')? ('=' expr)? ';'
   assignment      ::= IDENTIFIER ('.' IDENTIFIER)* ('[' expr ']')? '=' expr ';'
   return_stmt     ::= 'return' expr ';'
   if_stmt         ::= 'if' '(' expr ')' '{' statement* '}' 
                       ('else' 'if' '(' expr ')' '{' statement* '}')*
                       ('else' '{' statement* '}')?
   while_stmt      ::= 'while' '(' expr ')' '{' statement* '}'
   for_stmt        ::= 'for' '(' init ';' expr ';' incr ')' '{' statement* '}'
   
   expr            ::= primary (OPERATOR primary)*
   primary         ::= IDENTIFIER | NUMBER | '(' expr ')' | unary_op primary
   unary_op        ::= '!' | '~' | '-'

Program Parsing
---------------

parse_program()
~~~~~~~~~~~~~~~

The entry point for parsing. Iterates over top-level declarations:

.. code-block:: c

   ASTNode* parse_program(FILE *input)
   {
       ASTNode *program_node = create_node(NODE_PROGRAM);
       
       advance(input);  // Prime the tokenizer with first token
       
       while (!match(TOKEN_EOF)) {
           if (match(TOKEN_KEYWORD)) {
               if (strcmp(current_token.value, "struct") == 0) {
                   parse_struct_declaration(input, program_node);
                   continue;
               }
               
               // Primitive type function declaration
               Token return_type = current_token;
               advance(input);
               parse_function_declaration(input, return_type, program_node);
           } else {
               advance(input);  // Skip unknown tokens
           }
       }
       
       return program_node;
   }

**Process:**

1. Create root ``NODE_PROGRAM`` node
2. Prime tokenizer by calling ``advance(input)``
3. Loop until ``TOKEN_EOF``
4. Dispatch to struct or function parsing based on keyword
5. Add parsed declarations as children of program node

**Recognized patterns:**

* ``struct Name { ... };`` → ``parse_struct_declaration()``
* ``int function(...) { ... }`` → ``parse_function_declaration()``
* Functions returning structs: ``struct Name func(...) { ... }``

parse_struct_declaration()
~~~~~~~~~~~~~~~~~~~~~~~~~~

Handles struct definitions and functions returning structs:

.. code-block:: c

   static ASTNode* parse_struct_declaration(FILE *input, ASTNode *program_node)
   {
       advance(input);  // consume 'struct'
       
       if (!match(TOKEN_IDENTIFIER)) {
           printf("Warning: 'struct' without name at line %d\n", current_token.line);
           return NULL;
       }
       
       Token struct_name_token = current_token;
       advance(input);
       
       // Struct definition: struct Name { ... };
       if (match(TOKEN_BRACE_OPEN)) {
           ASTNode *struct_node = parse_struct(input, struct_name_token);
           if (struct_node) {
               add_child(program_node, struct_node);
           }
           return struct_node;
       }
       
       // Function returning struct: struct Name funcname(...) { ... }
       if (match(TOKEN_IDENTIFIER)) {
           Token function_name = current_token;
           advance(input);
           
           if (match(TOKEN_PARENTHESIS_OPEN)) {
               ASTNode *function_node = parse_function(input, struct_name_token, function_name);
               if (function_node) {
                   add_child(program_node, function_node);
               }
               return function_node;
           }
       }
       
       return NULL;
   }

**Disambiguation:**

* ``struct Point { int x; int y; };`` → struct definition
* ``struct Point create_point(int x, int y) { ... }`` → function returning struct

parse_function_declaration()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Parses functions with primitive return types:

.. code-block:: c

   static ASTNode* parse_function_declaration(FILE *input, Token return_type, ASTNode *program_node)
   {
       if (!match(TOKEN_IDENTIFIER)) {
           printf("Warning: Expected identifier after type at line %d\n", current_token.line);
           advance(input);
           return NULL;
       }
       
       Token function_name = current_token;
       advance(input);
       
       if (match(TOKEN_PARENTHESIS_OPEN)) {
           ASTNode *function_node = parse_function(input, return_type, function_name);
           if (function_node) {
               add_child(program_node, function_node);
           }
           return function_node;
       } else {
           printf("Warning: Global variable declarations not yet implemented\n");
           skip_to_semicolon(input);
           return NULL;
       }
   }

.. note::
   Global variable declarations are **not yet implemented**. The parser only supports top-level functions and structs.

Struct Parsing
--------------

parse_struct()
~~~~~~~~~~~~~~

Parses struct definitions with field declarations:

.. code-block:: c

   ASTNode* parse_struct(FILE *input, Token struct_name_token)
   {
       if (!consume(input, TOKEN_BRACE_OPEN)) {
           printf("Error: Expected '{' after struct name\n");
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
       
       if (!consume(input, TOKEN_BRACE_CLOSE)) {
           printf("Error: Expected '}' after struct body\n");
       }
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error: Expected ';' after struct declaration\n");
       }
       
       if (struct_index == g_struct_count) {
           g_struct_count++;
       }
       
       return struct_node;
   }

**Process:**

1. Consume opening brace
2. Create ``NODE_STRUCT_DECL`` node with struct name
3. Register struct in global symbol table (``g_structs``)
4. Parse all fields until closing brace
5. Consume closing brace and semicolon
6. Increment global struct count

**Side effects:** Registers struct and its fields in the global ``g_structs`` table for later type checking and code generation.

parse_struct_field()
~~~~~~~~~~~~~~~~~~~~

Parses a single struct field declaration:

.. code-block:: c

   static ASTNode* parse_struct_field(FILE *input, int struct_index)
   {
       if (!match(TOKEN_KEYWORD)) {
           return NULL;
       }
       
       Token field_type = current_token;
       advance(input);
       
       if (!match(TOKEN_IDENTIFIER)) {
           printf("Error: Expected field name in struct\n");
           exit(EXIT_FAILURE);
       }
       
       Token field_name = current_token;
       advance(input);
       
       ASTNode *field_node = create_node(NODE_VAR_DECL);
       field_node->token = field_type;
       field_node->value = strdup(field_name.value);
       
       register_field_in_struct(struct_index, field_type, field_name);
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error: Expected ';' after struct field\n");
           exit(EXIT_FAILURE);
       }
       
       return field_node;
   }

**Process:**

1. Expect type keyword (``int``, ``float``, etc.)
2. Expect field name identifier
3. Create ``NODE_VAR_DECL`` representing the field
4. Register field in struct's field table
5. Consume semicolon

**Limitations:**

* No support for nested structs as field types
* No support for array fields
* No support for pointer fields
* All field types must be primitive types

**Example struct:**

.. code-block:: c

   struct Point {
       int x;
       int y;
   };

**AST structure:**

.. code-block:: text

   NODE_STRUCT_DECL (value = "Point")
     ├─ NODE_VAR_DECL (token.value = "int", value = "x")
     └─ NODE_VAR_DECL (token.value = "int", value = "y")

Symbol Table Registration
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Struct registration:**

.. code-block:: c

   static void register_struct_in_table(int struct_index, Token struct_name_token)
   {
       if (g_struct_count < MAX_STRUCTS) {
           strcpy(g_structs[struct_index].name, struct_name_token.value);
           g_structs[struct_index].field_count = 0;
       }
   }

**Field registration:**

.. code-block:: c

   static void register_field_in_struct(int struct_index, Token field_type, Token field_name)
   {
       if (struct_index != g_struct_count || g_struct_count >= MAX_STRUCTS) {
           return;
       }
       
       StructInfo *struct_info = &g_structs[struct_index];
       if (struct_info->field_count >= MAX_STRUCT_FIELDS) {
           return;
       }
       
       int field_index = struct_info->field_count;
       strcpy(struct_info->fields[field_index].field_name, field_name.value);
       strcpy(struct_info->fields[field_index].field_type, field_type.value);
       struct_info->field_count++;
   }

The parser maintains a global ``g_structs`` table that stores struct metadata:

.. code-block:: c

   typedef struct {
       char field_name[64];
       char field_type[64];
   } FieldInfo;
   
   typedef struct {
       char name[64];
       FieldInfo fields[MAX_STRUCT_FIELDS];
       int field_count;
   } StructInfo;
   
   extern StructInfo g_structs[MAX_STRUCTS];
   extern int g_struct_count;

This table is used during code generation to resolve struct field types and offsets.

Function Parsing
----------------

parse_function()
~~~~~~~~~~~~~~~~

Entry point for parsing function declarations:

.. code-block:: c

   ASTNode* parse_function(FILE *input, Token return_type, Token function_name)
   {
       ASTNode *function_node = NULL;
       
       // Initialize function node
       function_node = create_node(NODE_FUNCTION_DECL);
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

**Process:**

1. Create ``NODE_FUNCTION_DECL`` node
2. Store return type in ``token`` field
3. Store function name in ``value`` field
4. Reset array count (arrays are function-scoped)
5. Parse parameter list
6. Parse function body

**Important:** The ``g_array_count`` reset means arrays are **function-scoped** in this compiler. Arrays declared in one function don't persist to another.

parse_function_parameters()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Parses the parameter list between parentheses:

.. code-block:: c

   static void parse_function_parameters(FILE *input, ASTNode *function_node)
   {
       Token parameter_type = {0};
       ASTNode *parameter_node = NULL;
       
       if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
           printf("Error: Expected '(' after function name\n");
           free_node(function_node);
           exit(EXIT_FAILURE);
       }
       
       while (!match(TOKEN_PARENTHESIS_CLOSE) && !match(TOKEN_EOF)) {
           if (match(TOKEN_KEYWORD)) {
               parameter_node = parse_single_parameter(input, &parameter_type);
               if (!parameter_node) {
                   break;
               }
               
               add_child(function_node, parameter_node);
               
               // Handle comma between parameters
               if (match(TOKEN_COMMA)) {
                   advance(input);
               }
           } else {
               advance(input);
           }
       }
       
       if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
           printf("Error: Expected ')' after parameter list\n");
           free_node(function_node);
           exit(EXIT_FAILURE);
       }
   }

**Process:**

1. Consume opening parenthesis
2. Loop until closing parenthesis:
   
   - Parse parameter type and name
   - Create ``NODE_VAR_DECL`` for parameter
   - Add as child of function node
   - Consume comma if present

3. Consume closing parenthesis

parse_single_parameter()
~~~~~~~~~~~~~~~~~~~~~~~~~

Parses one parameter declaration:

.. code-block:: c

   static ASTNode* parse_single_parameter(FILE *input, Token *parameter_type)
   {
       Token parameter_name = {0};
       ASTNode *parameter_node = NULL;
       
       // Handle struct types
       if (strcmp(current_token.value, "struct") == 0) {
           advance(input);
           if (!match(TOKEN_IDENTIFIER)) {
               printf("Error: Expected struct name in parameter list\n");
               return NULL;
           }
           *parameter_type = current_token;
           advance(input);
       } else {
           *parameter_type = current_token;
           advance(input);
       }
       
       // Get parameter name
       if (!match(TOKEN_IDENTIFIER)) {
           printf("Error: Expected parameter name\n");
           return NULL;
       }
       
       parameter_name = current_token;
       advance(input);
       
       // Create parameter node
       parameter_node = create_node(NODE_VAR_DECL);
       parameter_node->token = *parameter_type;
       parameter_node->value = strdup(parameter_name.value);
       
       return parameter_node;
   }

**Handles two parameter type patterns:**

1. **Primitive types:** ``int x`` → type = ``"int"``, name = ``"x"``
2. **Struct types:** ``struct Point p`` → type = ``"Point"``, name = ``"p"``

parse_function_body()
~~~~~~~~~~~~~~~~~~~~~

Parses the function body between braces:

.. code-block:: c

   static void parse_function_body(FILE *input, ASTNode *function_node)
   {
       int brace_depth = 1;  // Start at 1 (opening brace already consumed)
       ASTNode *statement_node = NULL;
       
       if (!consume(input, TOKEN_BRACE_OPEN)) {
           printf("Error: Expected '{' to start function body\n");
           free_node(function_node);
           exit(EXIT_FAILURE);
       }
       
       while (brace_depth > 0 && !match(TOKEN_EOF)) {
           if (match(TOKEN_BRACE_OPEN)) {
               brace_depth++;
               advance(input);
           } else if (match(TOKEN_BRACE_CLOSE)) {
               brace_depth--;
               advance(input);
           } else {
               statement_node = parse_statement(input);
               if (statement_node) {
                   add_child(function_node, statement_node);
               }
           }
       }
   }

**Brace depth tracking:**

The function tracks nested braces to correctly handle statement blocks. This allows parsing of constructs like:

.. code-block:: c

   int foo() {
       if (x) {
           // brace_depth = 2
       }
       // brace_depth = 1
   }
   // brace_depth = 0, exit loop

**Note:** This implementation has a quirk where it manually tracks braces even though ``parse_statement()`` already handles brace-delimited blocks. This is redundant but doesn't cause issues.

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
------------------

The expression parser uses **precedence climbing** (also called **Pratt parsing**) to handle operator precedence correctly.

parse_expression()
~~~~~~~~~~~~~~~~~~

Top-level entry point for expressions:

.. code-block:: c

   ASTNode* parse_expression(FILE *input)
   { 
       return parse_expression_prec(input, TOP_LEVEL_EXPR_MIN_PRECEDENCE);
   }

Delegates to ``parse_expression_prec()`` with minimum precedence of ``-2`` (includes all operators down to logical OR).

parse_expression_prec()
~~~~~~~~~~~~~~~~~~~~~~~

Core expression parser using **precedence climbing**:

.. code-block:: c

   ASTNode* parse_expression_prec(FILE *input, int min_prec)
   {
       ASTNode *left_operand = NULL;
       ASTNode *right_operand = NULL;
       ASTNode *binary_expr = NULL;
       const char *operator = NULL;
       int operator_precedence = 0;
       
       left_operand = parse_primary(input);
       if (!left_operand) {
           return NULL;
       }
       
       while (match(TOKEN_OPERATOR)) {
           operator = current_token.value;
           operator_precedence = get_precedence(operator);
           
           if (operator_precedence < min_prec) {
               break;  // Operator has lower precedence than minimum
           }
           
           advance(input);
           right_operand = parse_expression_prec(input, operator_precedence + 1);
           if (!right_operand) {
               printf("Error: Expected right operand after operator '%s'\n", operator);
               exit(EXIT_FAILURE);
           }
           
           binary_expr = create_node(NODE_BINARY_EXPR);
           binary_expr->value = strdup(operator);
           add_child(binary_expr, left_operand);
           add_child(binary_expr, right_operand);
           left_operand = binary_expr;
       }
       
       return left_operand;
   }

**Algorithm:**

1. Parse left operand using ``parse_primary()``
2. While current token is an operator with precedence ≥ ``min_prec``:
   
   a. Save operator and its precedence
   b. Recursively parse right operand with precedence = ``operator_precedence + 1``
   c. Create ``NODE_BINARY_EXPR`` combining left, operator, and right
   d. New expression becomes left operand for next iteration

3. Return final expression tree

**Precedence levels** (from ``get_precedence()`` in ``utils.c``):

.. code-block:: c

   7:  *  /                    // Multiplication, division
   6:  +  -                    // Addition, subtraction
   5:  << >>                   // Bitwise shifts
   4:  <  <= >  >=             // Relational comparisons
   3:  == !=                   // Equality comparisons
   2:  &                       // Bitwise AND
   1:  ^                       // Bitwise XOR
   0:  |                       // Bitwise OR
  -1:  &&                      // Logical AND
  -2:  ||                      // Logical OR

**Example:** ``3 + 4 * 5``

.. code-block:: text

   parse_expression_prec(input, -2)
     left = parse_primary() → "3"
     operator = "+", precedence = 6
     right = parse_expression_prec(input, 7)
       left = parse_primary() → "4"
       operator = "*", precedence = 7
       right = parse_expression_prec(input, 8)
         left = parse_primary() → "5"
         no more operators
         return "5"
       return BINARY("*", "4", "5")
     return BINARY("+", "3", BINARY("*", "4", "5"))

parse_primary()
~~~~~~~~~~~~~~~

Parses atomic expressions and unary operators:

.. code-block:: c

   ASTNode* parse_primary(FILE *input)
   {
       // Logical NOT operator
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "!") == 0) {
           return parse_logical_not(input);
       }
       
       // Bitwise NOT operator
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "~") == 0) {
           return parse_bitwise_not(input);
       }
       
       // Unary minus operator
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "-") == 0) {
           return parse_unary_minus(input);
       }
       
       // Parenthesized expression
       if (match(TOKEN_PARENTHESIS_OPEN)) {
           return parse_parenthesized_expr(input);
       }
       
       // Identifier (with optional field access and array indexing)
       if (match(TOKEN_IDENTIFIER)) {
           return parse_identifier(input);
       }
       
       // Number literal
       if (match(TOKEN_NUMBER)) {
           return parse_number(input);
       }
       
       return NULL;
   }

**Recognized patterns:**

* ``!expr`` → logical NOT (``NODE_BINARY_OP`` with value ``"!"``)
* ``~expr`` → bitwise NOT (``NODE_BINARY_OP`` with value ``"~"``)
* ``-expr`` → unary minus (special handling)
* ``(expr)`` → parenthesized expression
* ``identifier`` → variable reference
* ``identifier.field`` → struct field access
* ``identifier[index]`` → array access
* ``123`` → number literal

Unary Operators
~~~~~~~~~~~~~~~

**Logical NOT** (``!``):

.. code-block:: c

   static ASTNode* parse_logical_not(FILE *input)
   {
       advance(input);
       ASTNode *operand = parse_primary(input);
       if (!operand) return NULL;
       
       ASTNode *not_node = create_node(NODE_BINARY_OP);
       not_node->value = strdup("!");
       add_child(not_node, operand);
       return not_node;
   }

**Bitwise NOT** (``~``):

.. code-block:: c

   static ASTNode* parse_bitwise_not(FILE *input)
   {
       advance(input);
       ASTNode *operand = parse_primary(input);
       if (!operand) return NULL;
       
       ASTNode *not_node = create_node(NODE_BINARY_OP);
       not_node->value = strdup("~");
       add_child(not_node, operand);
       return not_node;
   }

**Unary minus** (``-``):

.. code-block:: c

   static ASTNode* parse_unary_minus(FILE *input)
   {
       advance(input);
       ASTNode *operand = parse_primary(input);
       if (!operand) return NULL;
       
       // Optimization: Fold into literal if operand is NODE_EXPRESSION
       if (operand->type == NODE_EXPRESSION && operand->value) {
           char negated_value[128];
           snprintf(negated_value, sizeof(negated_value), "-%s", operand->value);
           ASTNode *result_node = create_node(NODE_EXPRESSION);
           result_node->value = strdup(negated_value);
           free_node(operand);
           return result_node;
       }
       
       // Otherwise, create "0 - operand"
       ASTNode *zero_node = create_node(NODE_EXPRESSION);
       zero_node->value = strdup("0");
       ASTNode *binary_expr = create_node(NODE_BINARY_EXPR);
       binary_expr->value = strdup("-");
       add_child(binary_expr, zero_node);
       add_child(binary_expr, operand);
       return binary_expr;
   }

**Optimization:** For simple expressions like ``-x``, the parser creates ``"-x"`` as a single node. For complex expressions, it creates ``0 - expr``.

Parenthesized Expressions
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_parenthesized_expr(FILE *input)
   {
       advance(input);  // consume '('
       ASTNode *expr_node = parse_expression_prec(input, PARENTHESIZED_EXPR_MIN_PRECEDENCE);
       
       if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
           printf("Error: Expected ')' after expression\n");
           exit(EXIT_FAILURE);
       }
       
       return expr_node;
   }

**Note:** Parenthesized expressions use minimum precedence of ``1`` (above bitwise OR), which allows all operators inside parentheses.

Identifier Parsing
~~~~~~~~~~~~~~~~~~

Handles identifiers with optional struct field access and array indexing:

.. code-block:: c

   static ASTNode* parse_identifier(FILE *input)
   {
       char identifier_name[128] = {0};
       char index_expression[512] = {0};
       char full_expression[643] = {0};
       
       strcpy(identifier_name, current_token.value);
       advance(input);
       
       // Handle field access (e.g., struct.field.subfield)
       parse_field_access(input, identifier_name, sizeof(identifier_name));
       
       // Handle array indexing
       if (match(TOKEN_BRACKET_OPEN)) {
           parse_array_index(input, index_expression, sizeof(index_expression));
           
           snprintf(full_expression, sizeof(full_expression), 
                    "%s[%s]", identifier_name, index_expression);
           ASTNode *identifier_node = create_node(NODE_EXPRESSION);
           identifier_node->value = strdup(full_expression);
           
           validate_array_bounds(identifier_name, index_expression);
           return identifier_node;
       }
       
       // Simple identifier
       ASTNode *identifier_node = create_node(NODE_EXPRESSION);
       identifier_node->value = strdup(identifier_name);
       return identifier_node;
   }

**Field access flattening:**

``point.x.y`` → stored as ``"point__x__y"`` (double underscore separator)

**Array indexing flattening:**

``arr[i + 1]`` → stored as ``"arr[i+1]"`` (index expression stored as string)

Field Access Parsing
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void parse_field_access(FILE *input, char *identifier_buffer, size_t buffer_size)
   {
       while (match(TOKEN_OPERATOR) && strcmp(current_token.value, ".") == 0) {
           advance(input);
           
           if (!match(TOKEN_IDENTIFIER)) {
               printf("Error: Expected field name after '.'\n");
               exit(EXIT_FAILURE);
           }
           
           safe_append(identifier_buffer, buffer_size, "__");
           safe_append(identifier_buffer, buffer_size, current_token.value);
           advance(input);
       }
   }

**Process:** Loops while seeing ``.`` operators, appending ``__`` followed by field name.

Array Index Parsing
~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void parse_array_index(FILE *input, char *index_buffer, size_t buffer_size)
   {
       int parenthesis_depth = 0;
       
       advance(input);  // consume '['
       
       while (!match(TOKEN_EOF)) {
           if (match(TOKEN_BRACKET_CLOSE) && parenthesis_depth == 0) {
               break;
           }
           
           if (match(TOKEN_PARENTHESIS_OPEN)) {
               safe_append(index_buffer, buffer_size, "(");
               advance(input);
               parenthesis_depth++;
               continue;
           }
           
           if (match(TOKEN_PARENTHESIS_CLOSE)) {
               safe_append(index_buffer, buffer_size, ")");
               advance(input);
               if (parenthesis_depth > 0) {
                   parenthesis_depth--;
               }
               continue;
           }
           
           if (current_token.value) {
               safe_append(index_buffer, buffer_size, current_token.value);
           }
           advance(input);
       }
       
       if (!consume(input, TOKEN_BRACKET_CLOSE)) {
           printf("Error: Expected ']' after array index\n");
           exit(EXIT_FAILURE);
       }
   }

**Process:** Consumes tokens until matching ``]``, tracking parenthesis depth to avoid stopping at ``]`` inside parentheses.

**Example:** ``arr[(i + 1) * 2]`` → index_buffer = ``"(i+1)*2"``

Array Bounds Validation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void validate_array_bounds(const char *identifier_name, const char *index_expr)
   {
       if (!is_number_str(index_expr)) {
           return;  // Dynamic index, cannot validate
       }
       
       int index_value = atoi(index_expr);
       int array_size = find_array_size(identifier_name);
       
       if (array_size > 0 && (index_value < 0 || index_value >= array_size)) {
           printf("Error: Array index %d out of bounds for '%s' with size %d\n",
                  index_value, identifier_name, array_size);
           exit(EXIT_FAILURE);
       }
   }

**Behavior:**

* Only validates constant numeric indices (e.g., ``arr[5]``)
* Looks up array size from symbol table (``find_array_size()``)
* Reports error and exits if index is out of bounds

Statement Parsing
-----------------

parse_statement()
~~~~~~~~~~~~~~~~~

Main statement dispatcher:

.. code-block:: c

   ASTNode* parse_statement(FILE *input)
   {
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
       
       // Assignment or expression statement
       if (match(TOKEN_IDENTIFIER)) {
           sub_statement = parse_assignment_or_expression(input);
           if (sub_statement) {
               add_child(stmt_node, sub_statement);
           }
           return stmt_node;
       }
       
       // Control flow statements
       if (match(TOKEN_KEYWORD)) {
           if (strcmp(current_token.value, "return") == 0) {
               return parse_return_statement(input);
           }
           if (strcmp(current_token.value, "if") == 0) {
               sub_statement = parse_if_statement(input);
               // ...
           }
           if (strcmp(current_token.value, "while") == 0) {
               sub_statement = parse_while_statement(input);
               // ...
           }
           if (strcmp(current_token.value, "for") == 0) {
               sub_statement = parse_for_statement(input);
               // ...
           }
           if (strcmp(current_token.value, "break") == 0) {
               sub_statement = parse_break_statement(input);
               // ...
           }
           if (strcmp(current_token.value, "continue") == 0) {
               sub_statement = parse_continue_statement(input);
               // ...
           }
       }
       
       // Unknown statement - skip to semicolon
       while (!match(TOKEN_SEMICOLON) && !match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
           advance(input);
       }
       if (match(TOKEN_SEMICOLON)) {
           advance(input);
       }
       
       return stmt_node;
   }

**Statement types recognized:**

* Variable declarations (``int x = 10;``)
* Assignments (``x = 20;``)
* Return statements (``return x;``)
* If/else-if/else chains
* While loops
* For loops
* Break statements
* Continue statements

Variable Declaration
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_variable_declaration(FILE *input, Token type_token)
   {
       // Handle struct types
       if (strcmp(type_token.value, "struct") == 0) {
           if (!match(TOKEN_IDENTIFIER)) {
               printf("Error: Expected struct name after 'struct'\n");
               exit(EXIT_FAILURE);
           }
           type_token = current_token;
           advance(input);
       }
       
       // Get variable name
       if (!match(TOKEN_IDENTIFIER)) {
           printf("Error: Expected variable name after type\n");
           exit(EXIT_FAILURE);
       }
       
       Token name_token = current_token;
       advance(input);
       
       ASTNode *var_decl_node = create_node(NODE_VAR_DECL);
       var_decl_node->token = type_token;
       var_decl_node->value = strdup(name_token.value);
       
       // Handle array declaration
       if (match(TOKEN_BRACKET_OPEN)) {
           advance(input);
           
           if (!match(TOKEN_NUMBER)) {
               printf("Error: Expected array size after '['\n");
               exit(EXIT_FAILURE);
           }
           
           char buf[1024];
           snprintf(buf, sizeof(buf), "%s[%s]", name_token.value, current_token.value);
           free(var_decl_node->value);
           var_decl_node->value = strdup(buf);
           
           register_array(name_token.value, atoi(current_token.value));
           advance(input);
           
           if (!consume(input, TOKEN_BRACKET_CLOSE)) {
               printf("Error: Expected ']' after array size\n");
               exit(EXIT_FAILURE);
           }
       }
       
       // Handle initialization
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
           advance(input);
           
           if (match(TOKEN_BRACE_OPEN)) {
               // Array or struct initializer list
               ASTNode *init_list = parse_initializer_list(input, is_array);
               add_child(var_decl_node, init_list);
           } else {
               // Single expression initializer
               ASTNode *init_expr = parse_expression(input);
               if (init_expr) {
                   add_child(var_decl_node, init_expr);
               }
           }
       }
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error: Expected ';' after variable declaration\n");
           exit(EXIT_FAILURE);
       }
       
       return var_decl_node;
   }

**Features:**

* Primitive types: ``int``, ``float``, ``char``, ``double``
* Struct types: ``struct Point p;``
* Arrays: ``int arr[10];``
* Initialization: ``int x = 42;``
* Array initialization: ``int arr[3] = {1, 2, 3};``
* Struct initialization: ``struct Point p = {10, 20};``

**Side effects:** Calls ``register_array()`` to track array sizes for bounds checking.

Assignment Statement
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_assignment_or_expression(FILE *input)
   {
       Token lhs_token = current_token;
       char lhs_buf[1024] = {0};
       char idx_buf[512] = {0};
       char base_name[128] = {0};
       
       advance(input);
       
       // Parse left-hand side (identifier with optional field access and array indexing)
       parse_lhs_expression(input, lhs_token, lhs_buf, sizeof(lhs_buf),
                           idx_buf, sizeof(idx_buf), base_name, sizeof(base_name));
       
       ASTNode *lhs_expr = create_node(NODE_EXPRESSION);
       lhs_expr->value = strdup(lhs_buf);
       
       // Check for assignment operator
       if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
           advance(input);
           ASTNode *assign_node = create_node(NODE_ASSIGNMENT);
           add_child(assign_node, lhs_expr);
           
           ASTNode *rhs_node = parse_expression(input);
           if (rhs_node) {
               add_child(assign_node, rhs_node);
           }
           
           if (!consume(input, TOKEN_SEMICOLON)) {
               printf("Error: Expected ';' after assignment\n");
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

**Left-hand side patterns:**

* Simple variable: ``x = 10;``
* Struct field: ``point.x = 5;`` → ``"point__x"``
* Array element: ``arr[i] = 42;`` → ``"arr[i]"``
* Combined: ``points[i].x = 3;`` → ``"points[i]__x"``

Control Flow Statements
-----------------------

If Statement
~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_if_statement(FILE *input)
   {
       advance(input);  // consume 'if'
       
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
       
       // Parse body statements
       while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
           ASTNode *inner_stmt = parse_statement(input);
           if (inner_stmt) {
               add_child(if_node, inner_stmt);
           }
       }
       
       if (!consume(input, TOKEN_BRACE_CLOSE)) {
           printf("Error: Expected '}' after if block\n");
           exit(EXIT_FAILURE);
       }
       
       // Parse optional else-if and else blocks
       parse_else_blocks(input, if_node);
       
       return if_node;
   }

**AST structure for if-else-if-else:**

.. code-block:: text

   NODE_IF_STATEMENT
     ├─ condition (NODE_BINARY_EXPR or NODE_EXPRESSION)
     ├─ statement 1 (body of if)
     ├─ statement 2 (body of if)
     ├─ NODE_ELSE_IF_STATEMENT
     │    ├─ condition
     │    └─ statements...
     └─ NODE_ELSE_STATEMENT
          └─ statements...

While Loop
~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_while_statement(FILE *input)
   {
       advance(input);  // consume 'while'
       
       if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
           printf("Error: Expected '(' after 'while'\n");
           exit(EXIT_FAILURE);
       }
       
       ASTNode *cond_expr = parse_expression(input);
       
       if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
           printf("Error: Expected ')' after while condition\n");
           exit(EXIT_FAILURE);
       }
       if (!consume(input, TOKEN_BRACE_OPEN)) {
           printf("Error: Expected '{' after while condition\n");
           exit(EXIT_FAILURE);
       }
       
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
       
       if (!consume(input, TOKEN_BRACE_CLOSE)) {
           printf("Error: Expected '}' after while block\n");
           exit(EXIT_FAILURE);
       }
       
       return while_node;
   }

**Loop depth tracking:** The static variable ``s_loop_depth`` tracks loop nesting to validate ``break`` and ``continue`` statements.

For Loop
~~~~~~~~

.. code-block:: c

   static ASTNode* parse_for_statement(FILE *input)
   {
       advance(input);  // consume 'for'
       
       if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
           printf("Error: Expected '(' after 'for'\n");
           exit(EXIT_FAILURE);
       }
       
       // Parse initialization
       ASTNode *init_node = parse_for_init(input);
       
       if (match(TOKEN_SEMICOLON)) {
           advance(input);
       }
       
       // Parse condition
       ASTNode *cond_expr = NULL;
       if (!match(TOKEN_SEMICOLON)) {
           cond_expr = parse_expression(input);
       }
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error: Expected ';' after for condition\n");
           exit(EXIT_FAILURE);
       }
       
       // Parse increment
       ASTNode *incr_expr = parse_for_increment(input);
       
       if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
           printf("Error: Expected ')' after for header\n");
           exit(EXIT_FAILURE);
       }
       if (!consume(input, TOKEN_BRACE_OPEN)) {
           printf("Error: Expected '{' after for header\n");
           exit(EXIT_FAILURE);
       }
       
       // Build for node
       ASTNode *for_node = create_node(NODE_FOR_STATEMENT);
       
       if (init_node) {
           add_child(for_node, init_node);
       }
       
       if (cond_expr) {
           add_child(for_node, cond_expr);
       } else {
           // No condition means "while (true)"
           ASTNode *true_expr = create_node(NODE_EXPRESSION);
           true_expr->value = strdup("1");
           add_child(for_node, true_expr);
       }
       
       // Parse body
       s_loop_depth++;
       while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
           ASTNode *inner = parse_statement(input);
           if (inner) {
               add_child(for_node, inner);
           }
       }
       s_loop_depth--;
       
       if (!consume(input, TOKEN_BRACE_CLOSE)) {
           printf("Error: Expected '}' after for body\n");
           exit(EXIT_FAILURE);
       }
       
       // Add increment as last child
       if (incr_expr) {
           add_child(for_node, incr_expr);
       }
       
       return for_node;
   }

**For-loop initialization parsing:**

The ``parse_for_init()`` helper handles two cases:

1. **Variable declaration:** ``for (int i = 0; ...)``
2. **Assignment:** ``for (i = 0; ...)``

.. code-block:: c

   static ASTNode* parse_for_init(FILE *input)
   {
       if (match(TOKEN_SEMICOLON)) {
           return NULL;  // Empty initialization
       }
       
       // Try parsing as variable declaration
       if (match(TOKEN_KEYWORD) && (strcmp(current_token.value, "int") == 0 ||
                                     strcmp(current_token.value, "float") == 0 ||
                                     /* ... */)) {
           ASTNode *init_stmt = parse_statement(input);
           if (init_stmt && init_stmt->num_children > 0) {
               ASTNode *child0 = init_stmt->children[0];
               if (child0->type == NODE_VAR_DECL || child0->type == NODE_ASSIGNMENT) {
                   return child0;
               }
           }
       }
       
       // Try parsing as assignment
       if (match(TOKEN_IDENTIFIER)) {
           Token temp_lhs = current_token;
           advance(input);
           if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
               advance(input);
               ASTNode *assign_tmp = create_node(NODE_ASSIGNMENT);
               ASTNode *lhs_expr_tmp = create_node(NODE_EXPRESSION);
               lhs_expr_tmp->value = strdup(temp_lhs.value);
               add_child(assign_tmp, lhs_expr_tmp);
               
               ASTNode *rhs_expr = parse_expression(input);
               if (rhs_expr) {
                   add_child(assign_tmp, rhs_expr);
               }
               
               if (!consume(input, TOKEN_SEMICOLON)) {
                   printf("Error: Expected ';' after for-init assignment\n");
                   exit(EXIT_FAILURE);
               }
               return assign_tmp;
           }
       }
       
       return NULL;
   }

**For-loop increment parsing:**

The ``parse_for_increment()`` helper handles:

1. **Increment/decrement:** ``i++``, ``i--``
2. **Assignment:** ``i = i + 1``

.. code-block:: c

   static ASTNode* parse_for_increment(FILE *input)
   {
       if (match(TOKEN_PARENTHESIS_CLOSE)) {
           return NULL;  // Empty increment
       }
       
       if (match(TOKEN_IDENTIFIER)) {
           Token inc_lhs = current_token;
           advance(input);
           
           // Handle ++ and --
           if (match(TOKEN_OPERATOR) && (strcmp(current_token.value, "++") == 0 ||
                                          strcmp(current_token.value, "--") == 0)) {
               ASTNode *incr_expr = create_node(NODE_ASSIGNMENT);
               ASTNode *lhs = create_node(NODE_EXPRESSION);
               lhs->value = strdup(inc_lhs.value);
               add_child(incr_expr, lhs);
               
               ASTNode *rhs = create_node(NODE_BINARY_EXPR);
               rhs->value = strdup(strcmp(current_token.value, "++") == 0 ? "+" : "-");
               ASTNode *op_l = create_node(NODE_EXPRESSION);
               op_l->value = strdup(inc_lhs.value);
               ASTNode *op_r = create_node(NODE_EXPRESSION);
               op_r->value = strdup("1");
               add_child(rhs, op_l);
               add_child(rhs, op_r);
               add_child(incr_expr, rhs);
               advance(input);
               return incr_expr;
           }
           
           // Handle assignment
           if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
               advance(input);
               ASTNode *incr_expr = create_node(NODE_ASSIGNMENT);
               ASTNode *lhs = create_node(NODE_EXPRESSION);
               lhs->value = strdup(inc_lhs.value);
               add_child(incr_expr, lhs);
               
               ASTNode *rhs = parse_expression(input);
               if (rhs) {
                   add_child(incr_expr, rhs);
               }
               return incr_expr;
           }
       }
       
       return NULL;
   }

**AST structure for for-loop:**

.. code-block:: text

   NODE_FOR_STATEMENT
     ├─ initialization (NODE_VAR_DECL or NODE_ASSIGNMENT)
     ├─ condition (NODE_BINARY_EXPR or NODE_EXPRESSION)
     ├─ body statement 1
     ├─ body statement 2
     └─ increment (NODE_ASSIGNMENT, last child)

Break and Continue
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_break_statement(FILE *input)
   {
       if (s_loop_depth <= 0) {
           printf("Error: 'break' not within a loop\n");
           exit(EXIT_FAILURE);
       }
       
       advance(input);
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error: Expected ';' after 'break'\n");
           exit(EXIT_FAILURE);
       }
       
       ASTNode *break_node = create_node(NODE_BREAK_STATEMENT);
       return break_node;
   }

.. code-block:: c

   static ASTNode* parse_continue_statement(FILE *input)
   {
       if (s_loop_depth <= 0) {
           printf("Error: 'continue' not within a loop\n");
           exit(EXIT_FAILURE);
       }
       
       advance(input);
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error: Expected ';' after 'continue'\n");
           exit(EXIT_FAILURE);
       }
       
       ASTNode *continue_node = create_node(NODE_CONTINUE_STATEMENT);
       return continue_node;
   }

**Validation:** Both functions check ``s_loop_depth`` to ensure they're only used inside loops.

Return Statement
~~~~~~~~~~~~~~~~

.. code-block:: c

   static ASTNode* parse_return_statement(FILE *input)
   {
       ASTNode *stmt_node = create_node(NODE_STATEMENT);
       stmt_node->token = current_token;  // Store 'return' keyword
       advance(input);
       
       ASTNode *return_expr = parse_expression(input);
       if (return_expr) {
           add_child(stmt_node, return_expr);
       }
       
       if (!consume(input, TOKEN_SEMICOLON)) {
           printf("Error: Expected ';' after return statement\n");
           exit(EXIT_FAILURE);
       }
       
       return stmt_node;
   }

**Special marker:** Return statements use ``NODE_STATEMENT`` with ``token.value == "return"`` to distinguish them from regular statements.

Error Handling
--------------

The parser uses a **fail-fast** approach:

* Most errors call ``printf()`` followed by ``exit(EXIT_FAILURE)``
* No error recovery or synchronization
* Provides line numbers from ``current_token.line``
* Some functions return ``NULL`` on error and let caller handle it

**Example error messages:**

.. code-block:: text

   Error (line 42): Expected ')' after if condition
   Error (line 15): Array index 10 out of bounds for 'arr' with size 5
   Error (line 23): 'break' not within a loop
   Warning: 'struct' without name at line 8

Helper Functions
----------------

match()
~~~~~~~

Checks if current token matches expected type without consuming:

.. code-block:: c

   int match(TokenType type) {
       return current_token.type == type;
   }

consume()
~~~~~~~~~

Checks if current token matches expected type, and if so, advances:

.. code-block:: c

   int consume(FILE *input, TokenType type) {
       if (match(type)) {
           advance(input);
           return 1;
       }
       return 0;
   }

advance()
~~~~~~~~~

Gets next token from lexer and updates ``current_token``:

.. code-block:: c

   void advance(FILE *input) {
       current_token = get_next_token(input);
   }

safe_append()
~~~~~~~~~~~~~

Safely appends string to buffer with bounds checking:

.. code-block:: c

   static inline void safe_append(char *dst, size_t dst_size, const char *src)
   {
       size_t used = strlen(dst);
       if (used >= dst_size - 1) {
           return;
       }
       size_t available_space = dst_size - 1 - used;
       size_t copy_length = strlen(src);
       if (copy_length > available_space) {
           copy_length = available_space;
       }
       memcpy(dst + used, src, copy_length);
       dst[used + copy_length] = '\0';
   }

safe_copy()
~~~~~~~~~~~

Safely copies string to buffer with length limit:

.. code-block:: c

   static inline void safe_copy(char *dst, size_t dst_size, const char *src, size_t limit)
   {
       if (!dst_size) {
           return;
       }
       size_t source_length = strlen(src);
       if (source_length > limit) {
           source_length = limit;
       }
       if (source_length >= dst_size) {
           source_length = dst_size - 1;
       }
       memcpy(dst, src, source_length);
       dst[source_length] = '\0';
   }

Design Decisions
----------------

**Recursive descent:**

* **Pro**: Simple, intuitive mapping from grammar to code
* **Pro**: Easy to debug and extend
* **Con**: Cannot handle left-recursive grammars (not an issue for C subset)
* **Con**: Stack depth proportional to expression/statement nesting

**Precedence climbing for expressions:**

* **Pro**: Handles all binary operators with correct precedence
* **Pro**: Right-associative by using ``min_prec + 1`` for recursion
* **Pro**: Avoids deep recursion compared to naive recursive descent
* **Con**: More complex than simple recursive descent

**Flattened field access and array indexing:**

* **Pro**: Simplifies code generation (no tree traversal needed)
* **Pro**: Reduces AST node count
* **Con**: Loses semantic structure (can't analyze index expressions in AST)
* **Con**: String manipulation prone to buffer overflow bugs

**Global token variable:**

* **Pro**: Simplifies function signatures (no need to pass token around)
* **Con**: Non-reentrant (cannot parse multiple files concurrently)
* **Con**: Harder to test in isolation

**Fail-fast error handling:**

* **Pro**: Simple implementation
* **Pro**: Forces user to fix errors immediately
* **Con**: Cannot report multiple errors in one pass
* **Con**: No error recovery or partial compilation

**Static loop depth tracking:**

* **Pro**: Simple validation of break/continue
* **Con**: Non-reentrant
* **Con**: Could be incorrect if error recovery is added

Limitations
-----------

**Current limitations:**

* No support for global variables
* No support for function pointers
* No support for typedefs
* No support for multi-dimensional arrays (``arr[i][j]``)
* No support for compound literals
* No support for designated initializers
* No error recovery (single error stops compilation)
* Non-reentrant (global state prevents concurrent parsing)
* Fixed-size buffers (potential buffer overflows)

**Grammar restrictions:**

* All if/while/for bodies must use braces (``{ ... }``)
* Array sizes must be constant integers
* Struct field access uses ``__`` separator (conflicts with identifiers containing ``__``)
* Break/continue only work in loops (not in switches, which aren't supported)

Performance Characteristics
----------------------------

**Time complexity:**

* Overall: O(n) where n is number of tokens
* Expression parsing: O(n) for n operators (precedence climbing is linear)
* Statement parsing: O(n) for n statements (each parsed once)

**Space complexity:**

* AST size: O(n) nodes for n tokens
* Parser stack depth: O(d) where d is maximum nesting depth
* Fixed buffers: ~2-3 KB per parsing function call

**Bottlenecks:**

* String operations (``strdup``, ``strcmp``, ``snprintf``)
* Fixed-size buffer copies (could use dynamic allocation)
* Linear search in keyword table (could use hash table)

Future Enhancements
-------------------

Potential improvements:

1. **Error recovery**: Continue parsing after errors to report multiple errors
2. **Better error messages**: Show context lines and suggest fixes
3. **Reentrant design**: Pass parser state as parameter instead of globals
4. **Symbol table integration**: Resolve identifiers during parsing
5. **Type checking**: Validate types during parsing (currently deferred to code generation)
6. **AST validation**: Separate validation pass after parsing
7. **Dynamic buffers**: Use ``malloc`` instead of fixed-size arrays
8. **Multi-dimensional arrays**: Parse ``arr[i][j]`` as nested indexing
9. **Switch statements**: Add support for switch/case
10. **Ternary operator**: Support ``cond ? true_expr : false_expr``
11. **Compound assignment**: Support ``+=``, ``-=``, ``*=``, etc.
12. **Prefix/postfix distinction**: Distinguish ``++i`` from ``i++``

Summary
-------

The parser is a **modular recursive descent parser** with **precedence climbing** for expressions. It:

* Consumes tokens from the lexer and builds an AST
* Handles C subset: functions, structs, statements, expressions
* Validates syntax with detailed error messages
* Flattens field access and array indexing into strings
* Tracks symbols for array bounds checking
* Uses fail-fast error handling with ``exit(EXIT_FAILURE)``

The design prioritizes **simplicity and clarity** over advanced features like error recovery or performance optimization. It provides a solid foundation for the compiler while being easy to understand and extend.


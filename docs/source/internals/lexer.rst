Lexer Implementation
====================

Overview
--------

The lexer (also called tokenizer or scanner) is implemented in ``src/parser/token.c`` and defined in ``include/token.h``. It performs lexical analysis by reading the input C source code character by character from a ``FILE*`` stream and grouping them into tokens.

**Key responsibilities:**

* Read source file stream using standard C I/O operations (``fgetc``, ``ungetc``)
* Skip whitespace and handle C-style comments (``//`` line comments and ``/* */`` block comments)
* Recognize keywords, identifiers, numbers, operators, and punctuation
* Track line numbers for error reporting
* Provide token stream to the parser via global state variables

.. note::
   The lexer implementation uses global state (``current_token`` and ``current_line``), making it non-reentrant. Only one file can be lexically analyzed at a time.

Location
--------

- Source file: ``src/parser/token.c``
- Header file: ``include/token.h``

Token Structure
---------------

Tokens are represented by the ``Token`` struct defined in ``include/token.h``:

.. code-block:: c

   typedef struct {
       TokenType type;
       char value[256];
       int line;
   } Token;

**Fields:**

* ``type``: The category of the token (keyword, identifier, operator, etc.)
* ``value``: The actual text of the token (fixed-size buffer of 256 bytes)
* ``line``: Line number where the token appears (for error messages)

**Global state variables:**

.. code-block:: c

   Token current_token;  // Currently active token
   int current_line = 1; // Current line number being scanned

The global ``current_token`` holds the most recently scanned token that the parser is currently examining. The global ``current_line`` tracks the line number and is incremented whenever a newline character is encountered.

Token Types
-----------

The lexer recognizes 14 distinct token types defined in ``include/token.h``:

.. code-block:: c

   typedef enum {
       TOKEN_IDENTIFIER,          // Variable names, function names (e.g., foo, my_var)
       TOKEN_KEYWORD,             // Reserved words (if, while, int, struct, etc.)
       TOKEN_NUMBER,              // Numeric literals (integers and floats: 42, 3.14)
       TOKEN_STRING,              // String literals (currently unused in implementation)
       TOKEN_OPERATOR,            // All operators (+, -, ==, !=, ++, --, etc.)
       TOKEN_SEMICOLON,           // Statement terminator ;
       TOKEN_PARENTHESIS_OPEN,    // (
       TOKEN_PARENTHESIS_CLOSE,   // )
       TOKEN_BRACE_OPEN,          // {
       TOKEN_BRACE_CLOSE,         // }
       TOKEN_BRACKET_OPEN,        // [
       TOKEN_BRACKET_CLOSE,       // ]
       TOKEN_COMMA,               // ,
       TOKEN_EOF                  // End of file marker
   } TokenType;

**Design notes:**

* Operators are unified into a single ``TOKEN_OPERATOR`` type. The actual operator is distinguished by the token's ``value`` field.
* Keywords are identified after tokenization by checking against a keyword table.
* Punctuation characters have dedicated token types for efficient parsing.

Keyword Recognition
-------------------

Keywords are recognized through a static lookup table defined in ``src/parser/token.c``:

.. code-block:: c

   const char *keywords[] = {
       "if", "else", "while", "for", "return", "break", "continue",
       "struct",
       "int", "float", "char", "double", "void",
       NULL
   };

The ``is_keyword()`` function checks if a given identifier string matches any entry in this table:

.. code-block:: c

   int is_keyword(const char *str) {
       int i = 0;
       for (i = 0; keywords[i] != NULL; i++) {
           if (strcmp(str, keywords[i]) == 0) {
               return 1;
           }
       }
       return 0;
   }

When the lexer encounters an alphabetic character or underscore, it scans a complete identifier and then uses ``is_keyword()`` to determine whether it should be classified as ``TOKEN_KEYWORD`` or ``TOKEN_IDENTIFIER``.

Core Lexer Functions
--------------------

The lexer provides four main API functions for the parser:

get_next_token()
~~~~~~~~~~~~~~~~

.. code-block:: c

   Token get_next_token(FILE *input);

This is the primary lexer function. It reads characters from the input stream and returns the next token. The function:

1. **Skips whitespace**: Any sequence of spaces, tabs, or newlines
2. **Increments line counter**: When encountering ``\n``
3. **Handles comments**: 
   
   - ``//`` causes the lexer to skip until end of line
   - ``/* */`` causes the lexer to skip until the closing ``*/``

4. **Recognizes token patterns**:

   - **Identifiers/keywords**: Start with ``[a-zA-Z_]``, continue with ``[a-zA-Z0-9_]``
   - **Numbers**: Start with ``[0-9]``, can include ``.`` for floats
   - **Operators**: Single or multi-character (``=``, ``==``, ``<``, ``<=``, etc.)
   - **Punctuation**: Individual characters mapped to dedicated token types

5. **Returns EOF token** when input is exhausted

**Comment handling implementation:**

.. code-block:: c

   if (c == '/') {
       d = fgetc(input);
       if (d == '/') {
           // Line comment: skip until newline
           while ((c = fgetc(input)) != EOF && c != '\n');
           if (c == '\n') current_line++;
           return get_next_token(input);  // Recursively get next token
       } else if (d == '*') {
           // Block comment: skip until */
           prev = 0;
           while ((c = fgetc(input)) != EOF) {
               if (c == '\n') current_line++;
               if (prev == '*' && c == '/') break;
               prev = c;
           }
           return get_next_token(input);  // Recursively get next token
       } else {
           // Just the division operator
           if (d != EOF) ungetc(d, input);
           token.type = TOKEN_OPERATOR;
           token.value[0] = '/';
           token.value[1] = '\0';
           return token;
       }
   }

The lexer uses **recursive calls** to ``get_next_token()`` after skipping comments, automatically continuing to the next meaningful token.

advance()
~~~~~~~~~

.. code-block:: c

   void advance(FILE *input);

Wrapper function that calls ``get_next_token()`` and updates the global ``current_token``:

.. code-block:: c

   void advance(FILE *input) {
       current_token = get_next_token(input);
   }

This is the function the parser calls to move forward in the token stream.

match()
~~~~~~~

.. code-block:: c

   int match(TokenType type);

Checks if the current token matches the expected type without consuming it:

.. code-block:: c

   int match(TokenType type) {
       return current_token.type == type;
   }

Returns 1 (true) if ``current_token.type == type``, otherwise returns 0 (false). This is used extensively in the parser for lookahead.

consume()
~~~~~~~~~

.. code-block:: c

   int consume(FILE *input, TokenType type);

Checks if the current token matches the expected type, and if so, advances to the next token:

.. code-block:: c

   int consume(FILE *input, TokenType type) {
       if (match(type)) {
           advance(input);
           return 1;
       }
       return 0;
   }

Returns 1 on successful match and consumption, 0 otherwise. This is used for optional tokens or for validation with custom error handling.

Lexer State Machine
-------------------

The ``get_next_token()`` function implements a **character-driven state machine**:

.. code-block:: text

   START
     ↓
   Skip whitespace/comments
     ↓
   Read first character
     ↓
   ┌─────────────────┬──────────────┬───────────────┬────────────┐
   ↓                 ↓              ↓               ↓            ↓
   Alpha/_         Digit          Operator      Punctuation    EOF
   ↓                 ↓              ↓               ↓            ↓
   Read identifier Read number    Lookahead for  Return token  Return EOF
   ↓                 ↓            multi-char ops    type        token
   Check keywords  Return NUMBER  ↓                            
   ↓                              Return OPERATOR              
   Return KEYWORD/IDENTIFIER

**State transitions:**

1. **Whitespace state**: Loop until non-whitespace, track newlines
2. **Comment state**: Skip ``//`` or ``/* */`` blocks, then recurse
3. **Identifier state**: Accumulate ``[a-zA-Z0-9_]`` characters, then check keyword table
4. **Number state**: Accumulate ``[0-9.]`` characters
5. **Operator state**: Try to match multi-character operators first (``==``, ``!=``, ``++``, ``--``, ``<<``, ``>>``, ``<=``, ``>=``, ``&&``, ``||``), then fall back to single-character
6. **Punctuation state**: Direct mapping to token types
7. **EOF state**: Return ``TOKEN_EOF``

Identifier Recognition
----------------------

When the lexer encounters an alphabetic character or underscore, it enters identifier recognition mode:

.. code-block:: c

   if (isalpha(c) || c == '_') {
       i = 0;
       token.value[i++] = c;
       // Read all alphanumeric characters and underscores
       while ((c = fgetc(input)) != EOF && (isalnum(c) || c == '_')) {
           if (i < 255) token.value[i++] = c;  // Prevent buffer overflow
       }
       token.value[i] = '\0';
       if (c != EOF) ungetc(c, input);  // Put back lookahead character
       
       // Determine if keyword or identifier
       token.type = is_keyword(token.value) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
       return token;
   }

**Process:**

1. Store first character
2. Loop while ``isalnum(c) || c == '_'``
3. Null-terminate the string
4. Push back the lookahead character (which broke the loop)
5. Call ``is_keyword()`` to classify as keyword or identifier

Number Recognition
------------------

Number recognition supports both integers and floating-point literals:

.. code-block:: c

   else if (isdigit(c)) {
       i = 0;
       token.value[i++] = c;
       // Read digits and decimal points
       while ((c = fgetc(input)) != EOF && (isdigit(c) || c == '.')) {
           if (i < 255) token.value[i++] = c;
       }
       token.value[i] = '\0';
       if (c != EOF) ungetc(c, input);
       token.type = TOKEN_NUMBER;
       return token;
   }

**Features:**

* Accepts any sequence of digits: ``42``, ``123456``
* Accepts decimal points for floats: ``3.14``, ``0.5``
* **No validation** of number format (e.g., ``3.14.15`` would be accepted as a single token)
* No support for scientific notation (``1e10``), hexadecimal (``0xFF``), or binary (``0b1010``) literals

.. note::
   Number validation is expected to occur in later compilation phases (semantic analysis).

Operator and Punctuation Recognition
-------------------------------------

The lexer handles operators with **lookahead for multi-character operators**:

.. code-block:: c

   else {
       d = fgetc(input);  // Lookahead one character
       token.value[0] = (char)c;
       token.value[1] = '\0';
       token.value[2] = '\0';
       
       // Check for single-character punctuation first
       switch (c) {
           case ';': token.type = TOKEN_SEMICOLON; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           case '(': token.type = TOKEN_PARENTHESIS_OPEN; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           case ')': token.type = TOKEN_PARENTHESIS_CLOSE; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           case '{': token.type = TOKEN_BRACE_OPEN; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           case '}': token.type = TOKEN_BRACE_CLOSE; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           case '[': token.type = TOKEN_BRACKET_OPEN; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           case ']': token.type = TOKEN_BRACKET_CLOSE; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           case ',': token.type = TOKEN_COMMA; 
                     if (d != EOF) ungetc(d, input); 
                     return token;
           default: break;
       }
       
       // Check for multi-character operators
       token.type = TOKEN_OPERATOR;
       if (c == '=' && d == '=') {
           token.value[0] = '=';
           token.value[1] = '=';
           token.value[2] = '\0';
       } else if (c == '!' && d == '=') {
           token.value[0] = '!';
           token.value[1] = '=';
           token.value[2] = '\0';
       } else if (c == '<' && d == '=') {
           token.value[0] = '<';
           token.value[1] = '=';
           token.value[2] = '\0';
       } else if (c == '>' && d == '=') {
           token.value[0] = '>';
           token.value[1] = '=';
           token.value[2] = '\0';
       } else if (c == '<' && d == '<') {
           token.value[0] = '<';
           token.value[1] = '<';
           token.value[2] = '\0';
       } else if (c == '>' && d == '>') {
           token.value[0] = '>';
           token.value[1] = '>';
           token.value[2] = '\0';
       } else if (c == '&' && d == '&') {
           token.value[0] = '&';
           token.value[1] = '&';
           token.value[2] = '\0';
       } else if (c == '|' && d == '|') {
           token.value[0] = '|';
           token.value[1] = '|';
           token.value[2] = '\0';
       } else if (c == '+' && d == '+') {
           token.value[0] = '+';
           token.value[1] = '+';
           token.value[2] = '\0';
       } else if (c == '-' && d == '-') {
           token.value[0] = '-';
           token.value[1] = '-';
           token.value[2] = '\0';
       } else {
           // Single-character operator
           if (d != EOF) ungetc(d, input);
           token.value[0] = (char)c;
           token.value[1] = '\0';
       }
       return token;
   }

**Multi-character operators recognized:**

* ``==`` (equality)
* ``!=`` (inequality)
* ``<=`` (less than or equal)
* ``>=`` (greater than or equal)
* ``<<`` (left shift)
* ``>>`` (right shift)
* ``&&`` (logical AND)
* ``||`` (logical OR)
* ``++`` (increment)
* ``--`` (decrement)

**Single-character operators:**

* ``+``, ``-``, ``*``, ``/`` (arithmetic)
* ``<``, ``>`` (relational)
* ``=`` (assignment)
* ``&``, ``|``, ``^``, ``~`` (bitwise)
* ``!`` (logical NOT)
* ``.`` (struct member access)

Error Handling
--------------

The lexer has **minimal error handling**:

* No explicit error reporting for invalid characters
* Buffer overflow protection: limits token value to 255 characters
* Undefined behavior for malformed input (e.g., unterminated block comment at EOF)

The lexer operates on a "best-effort" basis. Malformed tokens or invalid syntax are expected to be caught by the parser or later compilation phases.

Line Tracking
-------------

The global ``current_line`` variable tracks the current line number:

.. code-block:: c

   // In whitespace skipping loop:
   while ((c = fgetc(input)) != EOF && isspace(c)) {
       if (c == '\n') current_line++;  // Increment on newline
   }

   // In line comment handling:
   while ((c = fgetc(input)) != EOF && c != '\n');
   if (c == '\n') current_line++;

   // In block comment handling:
   while ((c = fgetc(input)) != EOF) {
       if (c == '\n') current_line++;  // Track newlines inside block comments
       if (prev == '*' && c == '/') break;
       prev = c;
   }

Each token stores its line number in the ``token.line`` field, which is set at the start of token recognition:

.. code-block:: c

   Token get_next_token(FILE *input) {
       Token token = {0};
       // ...
       token.line = current_line;  // Track line at start of token
       // ... rest of tokenization
   }

This allows the parser and semantic analyzer to report precise error locations.

Limitations and Design Tradeoffs
---------------------------------

**Global state:**

* The lexer uses global variables (``current_token``, ``current_line``), making it non-reentrant
* Cannot tokenize multiple files concurrently
* Simplifies parser interface (no need to pass lexer context)

**Fixed-size buffers:**

* Token values are limited to 255 characters
* Long identifiers or numbers will be silently truncated
* No dynamic memory allocation in token structure

**Minimal validation:**

* No validation of number formats (``3.14.15`` accepted)
* No error reporting for unexpected characters
* Relies on parser to catch syntax errors

**Recursive comment handling:**

* Uses recursion to skip comments (``return get_next_token(input)``)
* Deeply nested comments could theoretically cause stack overflow
* Elegant and concise implementation

**No token reuse:**

* Each call to ``get_next_token()`` allocates a new token struct on the stack
* No token recycling or pooling
* Simple and straightforward memory model

Future Enhancements
-------------------

Potential improvements to the lexer:

1. **Reentrant design**: Pass lexer state as parameter instead of using globals
2. **Dynamic token values**: Use ``malloc()`` for token strings instead of fixed buffers
3. **Better error reporting**: Report specific lexer errors with line/column information
4. **String literal support**: Implement ``TOKEN_STRING`` handling with escape sequences
5. **Character literals**: Support ``'a'`` syntax for char constants
6. **Number validation**: Validate numeric literal formats during tokenization
7. **Preprocessor integration**: Handle ``#include``, ``#define``, etc.
8. **Column tracking**: Add column numbers alongside line numbers
9. **Token position**: Store start and end positions for better error messages
10. **Hexadecimal/binary literals**: Support ``0x`` and ``0b`` prefixes

Summary
-------

The lexer is a straightforward, character-driven tokenizer that:

* Reads from ``FILE*`` streams
* Skips whitespace and C-style comments
* Recognizes identifiers, keywords, numbers, operators, and punctuation
* Tracks line numbers for error reporting
* Uses global state for simplicity
* Provides a clean API for the parser: ``advance()``, ``match()``, ``consume()``

The implementation prioritizes **simplicity and clarity** over advanced features. It handles the subset of C syntax required by the compiler without unnecessary complexity.

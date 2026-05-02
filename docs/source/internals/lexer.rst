Lexer Implementation
====================

Overview
--------

The lexer (also called tokenizer or scanner) is implemented in ``src/parser/token.c`` with its internal API in ``src/parser/tokenizer.h`` and public types in ``include/token.h``. It performs lexical analysis by reading the input C source code character by character from a ``FILE*`` stream and grouping them into tokens.

**Key responsibilities:**

* Read source file stream using standard C I/O operations (``fgetc``, ``ungetc``)
* Skip whitespace and handle C-style comments (``//`` line comments and ``/* */`` block comments)
* Recognize keywords, identifiers, numbers, operators, and punctuation
* Track line numbers for error reporting
* Provide token stream to the parser via ``ParserContext``

.. note::
   The lexer uses a ``ParserContext`` struct to encapsulate all mutable state, making
   it reentrant and testable. All lexer functions take a ``ParserContext*`` parameter.

Location
--------

- Source file: ``src/parser/token.c``
- Internal header: ``src/parser/tokenizer.h``
- Public types: ``include/token.h``

ParserContext
-------------

All mutable lexer state is encapsulated in the ``ParserContext`` struct defined in ``include/token.h``:

.. code-block:: c

   typedef struct {
       Token current_token;   // Most recently read token
       int current_line;      // Current source line number (1-based)
       FILE *input;           // Source file being parsed
   } ParserContext;

A context is initialized once per parse invocation via ``parser_context_init()``:

.. code-block:: c

   void parser_context_init(ParserContext *ctx, FILE *input);

This sets ``current_line`` to 1, zeros the token, and stores the input handle.

Token Structure
---------------

Tokens are represented by the ``Token`` struct defined in ``include/token.h``:

.. code-block:: c

   #define TOKEN_VALUE_SIZE 256

   typedef struct {
       TokenType type;
       char value[TOKEN_VALUE_SIZE];
       int line;
   } Token;

**Fields:**

* ``type``: The category of the token (keyword, identifier, operator, etc.)
* ``value``: The actual text of the token (stored in a fixed-size buffer)
* ``line``: Line number where the token appears (for error messages)

Token Types
-----------

The lexer recognizes 13 distinct token types defined in ``include/token.h``:

.. code-block:: c

   typedef enum {
       TOKEN_IDENTIFIER,          // Variable names, function names (e.g., foo, my_var)
       TOKEN_KEYWORD,             // Reserved words (if, while, int, struct, etc.)
       TOKEN_NUMBER,              // Numeric literals (integers and floats: 42, 3.14)
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
* Punctuation characters have dedicated token types for straightforward parsing.

Keyword Recognition
-------------------

Keywords are recognized through a static lookup table defined in ``src/parser/token.c``:

.. code-block:: c

   static const char *keywords[] = {
       "if", "else", "while", "for", "return", "break", "continue",
       "struct",
       "int", "float", "char", "double", "void",
       NULL
   };

The ``is_keyword()`` function checks if a given identifier string matches any entry in this table:

.. code-block:: c

   int is_keyword(const char *str) {
       for (int keyword_idx = 0; keywords[keyword_idx] != NULL; keyword_idx++) {
           if (strcmp(str, keywords[keyword_idx]) == 0) {
               return 1;
           }
       }
       return 0;
   }

When the lexer encounters an alphabetic character or underscore, it scans a complete identifier and then uses ``is_keyword()`` to determine whether it should be classified as ``TOKEN_KEYWORD`` or ``TOKEN_IDENTIFIER``.

Core Lexer Functions
--------------------

The lexer provides four main API functions for the parser, all taking a ``ParserContext*``:

get_next_token()
~~~~~~~~~~~~~~~~

.. code-block:: c

   Token get_next_token(ParserContext *ctx);

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

advance()
~~~~~~~~~

.. code-block:: c

   void advance(ParserContext *ctx);

Calls ``get_next_token()`` and stores the result in ``ctx->current_token``.
This is the function the parser calls to move forward in the token stream.

match()
~~~~~~~

.. code-block:: c

   int match(const ParserContext *ctx, TokenType type);

Checks if ``ctx->current_token`` matches the expected type without consuming it.
Returns 1 (true) on match, 0 (false) otherwise. Used extensively in the parser for lookahead.

consume()
~~~~~~~~~

.. code-block:: c

   int consume(ParserContext *ctx, TokenType type);

Checks if the current token matches the expected type, and if so, advances to the next token.
Returns 1 on successful match and consumption, 0 on mismatch. On mismatch, logs an error
via ``log_error()``.

Lexer State Machine
-------------------

The ``get_next_token()`` function implements a **character-driven state machine**:

.. code-block:: text

   START
     |
   Skip whitespace/comments
     |
   Read first character
     |
   +-------------------+--------------+---------------+------------+
   |                   |              |               |            |
   Alpha/_           Digit          Operator      Punctuation    EOF
   |                   |              |               |            |
   Read identifier   Read number    Lookahead for  Return token  Return EOF
   |                   |            multi-char ops    type        token
   Check keywords    Return NUMBER  |
   |                               Return OPERATOR
   Return KEYWORD/IDENTIFIER

**State transitions:**

1. **Whitespace state**: Loop until non-whitespace, track newlines
2. **Comment state**: Skip ``//`` or ``/* */`` blocks, then recurse
3. **Identifier state**: Accumulate ``[a-zA-Z0-9_]`` characters, then check keyword table
4. **Number state**: Accumulate ``[0-9.]`` characters
5. **Operator state**: Try to match multi-character operators first (``==``, ``!=``, ``++``, ``--``, ``<<``, ``>>``, ``<=``, ``>=``, ``&&``, ``||``), then fall back to single-character
6. **Punctuation state**: Direct mapping to token types
7. **EOF state**: Return ``TOKEN_EOF``

Buffer Overflow Protection
--------------------------

Token values are bounded by ``MAX_TOKEN_VALUE_LEN`` (``TOKEN_VALUE_SIZE - 1 = 255``):

.. code-block:: c

   #define MAX_TOKEN_VALUE_LEN (TOKEN_VALUE_SIZE - 1)

   // In identifier scanning:
   if (value_idx < MAX_TOKEN_VALUE_LEN) {
       token.value[value_idx++] = current_char;
   }

If a token exceeds the buffer, the value is silently truncated and a warning is logged via the error handler:

.. code-block:: c

   log_warning(ERROR_CATEGORY_LEXER, ctx->current_line,
               "Identifier truncated to %d characters", MAX_TOKEN_VALUE_LEN);

Recognized Operators
--------------------

**Multi-character operators:**

* ``==`` (equality), ``!=`` (inequality)
* ``<=`` (less/equal), ``>=`` (greater/equal)
* ``<<`` (left shift), ``>>`` (right shift)
* ``&&`` (logical AND), ``||`` (logical OR)
* ``++`` (increment), ``--`` (decrement)

**Single-character operators:**

* ``+``, ``-``, ``*``, ``/`` (arithmetic)
* ``<``, ``>`` (relational)
* ``=`` (assignment)
* ``&``, ``|``, ``^``, ``~`` (bitwise)
* ``!`` (logical NOT)
* ``.`` (struct member access)

Error Handling
--------------

The lexer reports errors and warnings through the project's error handler (``error_handler.h``):

* **Buffer overflow**: Tokens exceeding ``MAX_TOKEN_VALUE_LEN`` characters are truncated with a warning
* **Unterminated comments**: Detected when EOF is reached inside a block comment
* **Failed consume**: ``consume()`` logs an error when the expected token type is not found

Line Tracking
-------------

The ``current_line`` field of ``ParserContext`` tracks the current line number.
Each token stores its line number in the ``token.line`` field, which is set at the
start of token recognition. This allows the parser and error handler to report
precise error locations.

Design Tradeoffs
----------------

**Reentrant design:**

* All mutable state is in ``ParserContext`` --- no global variables
* Multiple files can theoretically be tokenized concurrently
* ``ParserContext`` is passed explicitly to all functions

**Fixed-size buffers:**

* Token values are limited to ``TOKEN_VALUE_SIZE - 1`` (255) characters
* Long identifiers or numbers will be truncated with a warning
* No dynamic memory allocation in token structure

**Recursive comment handling:**

* Uses recursion to skip comments (``return get_next_token(ctx)``)
* Deeply nested comments could theoretically cause stack overflow
* Elegant and concise implementation

Summary
-------

The lexer is a straightforward, character-driven tokenizer that:

* Reads from ``FILE*`` streams via ``ParserContext``
* Skips whitespace and C-style comments
* Recognizes identifiers, keywords, numbers, operators, and punctuation
* Tracks line numbers for error reporting
* Uses ``ParserContext`` to encapsulate all mutable state
* Reports errors through the project's error handler
* Provides a clean API for the parser: ``advance()``, ``match()``, ``consume()``

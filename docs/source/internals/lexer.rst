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
       int depth;             // Current expression/statement nesting depth
   } ParserContext;

``depth`` is lexer-adjacent state only in the sense that it lives in the same
struct; the lexer itself never reads or writes it. It's maintained entirely
by the parser's recursion-depth guard (see :doc:`parser`) and exists here
because ``ParserContext`` is the one piece of mutable state threaded through
both layers.

A context is initialized once per parse invocation via ``parser_context_init()``:

.. code-block:: c

   void parser_context_init(ParserContext *ctx, FILE *input)
   {
       memset(ctx, 0, sizeof(*ctx));
       ctx->current_line = 1;
       ctx->input = input;
   }

This zeros the whole struct (so ``depth`` and the token both start at zero) before setting ``current_line`` to 1 and storing the input handle.

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

A second table, ``type_keywords[]`` (also in ``token.c``), holds just the primitive-type subset of ``keywords[]`` (``int``, ``float``, ``char``, ``double`` — notably not ``void`` or ``struct``), checked by ``is_type_keyword()``. This is what the *parser* (not the lexer) uses to decide whether a keyword-led statement is a variable declaration; see :doc:`parser`. The two tables are maintained by hand and must be kept in sync — a comment above each one says so.

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

If an identifier exceeds the buffer, the stored value is truncated to ``MAX_TOKEN_VALUE_LEN`` characters and an **error** — not a warning — is logged, so the compile still fails overall even though a (truncated) token is returned:

.. code-block:: c

   log_error(ERROR_CATEGORY_LEXER, ctx->current_line,
             "Identifier too long (truncated to %d characters)", MAX_TOKEN_VALUE_LEN);

This is the tokenizer's own 255-character limit on raw token text
(``TOKEN_VALUE_SIZE``); it's separate from — and much larger than — the
parser's 128-character limit on identifiers used as AST node values, which
rejects the identifier outright rather than truncating it (see
:doc:`parser`). Number literals have no equivalent check: an over-length
number is silently truncated with no diagnostic at all.

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

The lexer reports errors through the project's error handler (``error_handler.h``):

* **Buffer overflow**: identifiers exceeding ``MAX_TOKEN_VALUE_LEN`` characters are truncated *and* logged as an ``ERROR_CATEGORY_LEXER`` error (not merely a warning) — see above
* **Failed consume**: ``consume()`` itself never logs anything; it just returns 0 on mismatch, and every call site in the parser logs its own error message (see :doc:`parser`)

.. note::
   **Unterminated block comments are not specifically detected.**
   ``skip_comment_or_division()`` loops reading characters looking for
   ``*/`` until either it finds one or ``fgetc()`` returns ``EOF`` — on EOF
   it just stops, with no error of its own. An unterminated ``/* ...`` at
   the end of a file silently consumes everything after it, including any
   real code and the function's closing brace, and only surfaces indirectly
   as whatever generic parse error results (e.g. ``"Expected '}' after
   function body"``) rather than a comment-specific diagnostic.

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
* Long identifiers are truncated and reported as an error; long numbers are truncated silently, with no diagnostic
* No dynamic memory allocation in token structure

**Comment skipping via an explicit loop, not recursion:**

* ``get_next_token()`` is a ``for (;;) { ... continue; ... }`` loop, not a function that recurses on itself after a comment
* This is a deliberate choice, called out in a comment in the source: a run of consecutive comments would otherwise cost one stack frame each via ``return get_next_token(ctx)``, and comment-heavy input could overflow the stack
* Every other branch (identifier, number, punctuation, operator, EOF) still returns directly from inside the loop, via ``return``

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

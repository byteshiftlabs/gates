=======================
Error Handling System
=======================

Overview
========

The error handling system provides a comprehensive, multi-level error reporting mechanism for the compiler.
It supports three severity levels with colored output, precise location tracking, source context display,
error codes, and helpful hints and suggestions for error recovery.

Architecture
============

Components
----------

The error handling system consists of:

1. **Severity Levels**: Three distinct levels (INFO, WARNING, ERROR)
2. **Error Categories**: Five categories for organizing errors (Lexer, Parser, Semantic, Codegen, General)
3. **Location Tracking**: File name, line number, column number, and source text
4. **Error Codes**: Unique identifiers for documentation reference
5. **Enhanced Output**: Colored output, source context, hints, and suggestions
6. **Counter System**: Tracks errors and warnings separately

Data Structures
---------------

ErrorSeverity Enum
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   typedef enum {
       SEVERITY_INFO,      // Informational messages (green)
       SEVERITY_WARNING,   // Warnings that don't stop compilation (yellow)
       SEVERITY_ERROR      // Errors that stop compilation (red)
   } ErrorSeverity;

**Purpose**: Defines the three severity levels for error messages.

- ``SEVERITY_INFO``: Informational messages that don't indicate problems
- ``SEVERITY_WARNING``: Issues that should be addressed but don't stop compilation
- ``SEVERITY_ERROR``: Critical issues that prevent successful compilation

ErrorCategory Enum
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   typedef enum {
       ERROR_CATEGORY_LEXER,     // Tokenization errors
       ERROR_CATEGORY_PARSER,    // Syntax errors
       ERROR_CATEGORY_SEMANTIC,  // Semantic analysis errors
       ERROR_CATEGORY_CODEGEN,   // Code generation errors
       ERROR_CATEGORY_GENERAL    // General errors
   } ErrorCategory;

**Purpose**: Categorizes errors by the compiler phase that detected them.

This helps users understand which part of the compilation process failed and
aids in debugging and error tracking.

ErrorLocation Structure
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   typedef struct {
       const char* filename;    // Source file name (NULL if not applicable)
       int line;                // Line number (0 if not applicable)
       int column;              // Column number (0 if not applicable)
       const char* source_line; // The actual source line text (NULL if not available)
   } ErrorLocation;

**Purpose**: Encapsulates all location information for an error.

**Fields**:

- ``filename``: Path to the source file containing the error
- ``line``: 1-based line number where the error occurred
- ``column``: 1-based column number for precise error location
- ``source_line``: The actual text of the line containing the error (for context display)

Internal Implementation
=======================

Color System
------------

The error handler uses ANSI escape codes for colored terminal output:

.. code-block:: c

   #define COLOR_RESET   "\033[0m"
   #define COLOR_GREEN   "\033[32m"    // INFO
   #define COLOR_YELLOW  "\033[33m"    // WARNING
   #define COLOR_RED     "\033[31m"    // ERROR
   #define COLOR_BOLD    "\033[1m"
   #define COLOR_CYAN    "\033[36m"    // Hints
   #define COLOR_MAGENTA "\033[35m"    // Suggestions
   #define COLOR_NONE    ""            // When colors disabled

**Color Mapping**:

- Green: Informational messages
- Yellow: Warnings
- Red: Errors
- Cyan: Hints and source context
- Magenta: "Did you mean?" suggestions

Global State
------------

The error handler maintains the following global state:

.. code-block:: c

   static int error_count = 0;
   static int warning_count = 0;
   static int colored_output_enabled = 1;

**State Variables**:

- ``error_count``: Cumulative count of all errors reported
- ``warning_count``: Cumulative count of all warnings reported
- ``colored_output_enabled``: Flag to enable/disable ANSI color codes

Static Helper Functions
-----------------------

The implementation follows best practices by using static helper functions:

get_color_for_severity()
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static const char* get_color_for_severity(ErrorSeverity severity)

Returns the appropriate ANSI color code for a given severity level.
Returns empty string if colors are disabled.

get_reset_color() / get_bold_color()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static const char* get_reset_color(void)
   static const char* get_bold_color(void)

Return ANSI codes for resetting color and bold text.
Return empty strings if colors are disabled.

print_severity_header()
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void print_severity_header(ErrorSeverity severity, ErrorCategory category)

Formats and prints the colored severity and category header:

.. code-block:: none

   error[Parser]
   warning[Semantic]
   info[General]

print_filename()
~~~~~~~~~~~~~~~~

.. code-block:: c

   static void print_filename(const char* filename)

Prints the filename with colon separator if filename is not NULL.

print_location()
~~~~~~~~~~~~~~~~

.. code-block:: c

   static void print_location(int line, int column)

Prints line and column numbers in the format ``line:column:`` if valid.

print_error_code()
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void print_error_code(const char* error_code)

Prints the error code in cyan color within brackets: ``[E0001]``

print_source_context()
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void print_source_context(const char* source_line, int column)

Prints the source line and a caret (^) indicator at the column position:

.. code-block:: none

       int result = x + y * z;
                        ^

The caret aligns precisely with the column where the error occurred.

print_formatted_message()
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void print_formatted_message(const char* format, va_list args)

Handles variadic argument formatting using ``vfprintf()``.

update_counters()
~~~~~~~~~~~~~~~~~

.. code-block:: c

   static void update_counters(ErrorSeverity severity)

Increments the appropriate counter based on severity level.

Public API Functions
====================

Extended Reporting
------------------

report_message_ex()
~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void report_message_ex(ErrorSeverity severity, ErrorCategory category,
                         const ErrorLocation* location, const char* error_code,
                         const char* format, ...)

**Purpose**: Main function for reporting errors with full location and code information.

**Parameters**:

- ``severity``: Error severity level
- ``category``: Error category
- ``location``: Pointer to ErrorLocation structure (can be NULL)
- ``error_code``: Error code string (e.g., "E0001", can be NULL)
- ``format``: Printf-style format string
- ``...``: Variable arguments for format string

**Output Format**:

.. code-block:: none

   filename:line:column: [CODE]severity[Category] message
       source line
       ^

**Example**:

.. code-block:: c

   ErrorLocation loc = {
       .filename = "main.c",
       .line = 42,
       .column = 18,
       .source_line = "int result = x + y * z"
   };
   report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_SEMANTIC, &loc, "E0100",
                    "Variable 'y' not declared in this scope");

**Output**:

.. code-block:: none

   main.c:42:18: [E0100]error[Semantic] Variable 'y' not declared in this scope
       int result = x + y * z
                        ^

add_error_hint()
~~~~~~~~~~~~~~~~

.. code-block:: c

   void add_error_hint(const char* format, ...)

**Purpose**: Adds a helpful hint line after an error message.

**Output Format**:

.. code-block:: none

       hint: <message>

**Example**:

.. code-block:: c

   add_error_hint("Declare the variable before using it");

**Output**:

.. code-block:: none

       hint: Declare the variable before using it

Multiple hints can be added to provide comprehensive guidance.

add_suggestion()
~~~~~~~~~~~~~~~~

.. code-block:: c

   void add_suggestion(const char* suggestion)

**Purpose**: Adds a "did you mean?" suggestion for common mistakes.

**Output Format**:

.. code-block:: none

       help: did you mean 'suggestion'?

**Example**:

.. code-block:: c

   add_suggestion("printf");

**Output**:

.. code-block:: none

       help: did you mean 'printf'?

Legacy Interface
----------------

report_message()
~~~~~~~~~~~~~~~~

.. code-block:: c

   void report_message(ErrorSeverity severity, ErrorCategory category,
                      int line, const char* format, ...)

Simple error reporting with just line number (no filename or column).

Convenience Functions
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void log_info(ErrorCategory category, int line, const char* format, ...)
   void log_warning(ErrorCategory category, int line, const char* format, ...)
   void log_error(ErrorCategory category, int line, const char* format, ...)

Wrappers around ``report_message()`` for each severity level.

Counter Management
------------------

.. code-block:: c

   int get_error_count(void);
   int get_warning_count(void);
   void reset_error_counters(void);
   int has_errors(void);

Functions for managing and querying error/warning counters.

Configuration
-------------

.. code-block:: c

   void set_colored_output(int enable);

Enables or disables colored output (useful for log files or non-terminal output).

Usage Patterns
==============

Basic Error Reporting
---------------------

.. code-block:: c

   // Simple error
   log_error(ERROR_CATEGORY_PARSER, 42, "Expected ';' but found '}'");

Enhanced Error with Context
----------------------------

.. code-block:: c

   ErrorLocation loc = {
       .filename = "calculator.c",
       .line = 25,
       .column = 10,
       .source_line = "result = x / 0;"
   };
   report_message_ex(SEVERITY_WARNING, ERROR_CATEGORY_SEMANTIC, 
                    &loc, "W0100", "Division by zero");
   add_error_hint("This will cause a runtime error");

Complete Error with Suggestions
--------------------------------

.. code-block:: c

   ErrorLocation loc = {
       .filename = "main.c",
       .line = 99,
       .column = 5,
       .source_line = "retrun result;"
   };
   report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, 
                    &loc, "E0200", "Unknown keyword 'retrun'");
   add_error_hint("Check the spelling of the keyword");
   add_error_hint("Keywords are case-sensitive in C");
   add_suggestion("return");

Checking for Errors
-------------------

.. code-block:: c

   if (has_errors()) {
       fprintf(stderr, "\nCompilation failed with %d error(s) and %d warning(s)\n",
               get_error_count(), get_warning_count());
       return EXIT_FAILURE;
   }

Design Decisions
================

Why stderr?
-----------

All error messages are written to ``stderr`` rather than ``stdout`` because:

1. Separates error output from program output
2. Allows users to redirect errors separately: ``./compi file.c 2> errors.log``
3. Follows Unix conventions for diagnostic messages

Why Static Helper Functions?
-----------------------------

Internal helper functions are declared ``static`` to:

1. Prevent name collisions in multi-file projects
2. Enable compiler optimizations (inlining, dead code elimination)
3. Clearly indicate implementation details vs. public API

Why Constants Instead of Magic Numbers?
----------------------------------------

All numeric values are defined as constants:

.. code-block:: c

   #define INVALID_LINE_NUMBER 0
   #define COLORED_OUTPUT_ENABLED 1
   #define COLORED_OUTPUT_DISABLED 0

Benefits:

1. Self-documenting code
2. Easy to modify behavior
3. Type safety and consistency
4. Better maintainability

Why Variadic Functions?
------------------------

Printf-style variadic functions (``...``) are used because:

1. Familiar interface for C programmers
2. Efficient string formatting
3. Type-safe with compiler warnings
4. Flexible message composition

Testing
=======

The error handling system has 35 comprehensive tests covering:

1. **Basic functionality**: Info, warning, error messages
2. **Counter management**: Accumulation, reset, has_errors()
3. **Extended features**: File locations, error codes, source context
4. **Multi-line errors**: Hints and suggestions
5. **Edge cases**: Missing filenames, zero line numbers, empty source lines
6. **Color toggling**: Verify ANSI codes are present/absent

Test Implementation
-------------------

Tests use a custom ``StderrCapture`` class that:

1. Redirects ``stderr`` to a pipe using ``dup2()``
2. Captures output from C ``fprintf()`` calls
3. Returns captured output as a string for assertion checking

.. code-block:: cpp

   class StderrCapture {
       // Captures C stderr output for testing
       int old_stderr;
       int pipe_fds[2];
   public:
       StderrCapture() {
           fflush(stderr);
           old_stderr = dup(STDERR_FILENO);
           pipe(pipe_fds);
           dup2(pipe_fds[1], STDERR_FILENO);
           close(pipe_fds[1]);
       }
       
       std::string get_output() {
           // Read from pipe and return as string
       }
   };

Future Enhancements
===================

Potential improvements:

1. **Error Limit**: Stop compilation after N errors to prevent spam
2. **Warning Control**: Enable/disable specific warning categories
3. **JSON Output**: Machine-readable error format for IDE integration
4. **Error Context Stack**: Track nested parsing contexts
5. **Color Scheme Customization**: User-configurable color themes
6. **Multi-line Source Context**: Show multiple lines around error
7. **Error Statistics**: Histogram of error types
8. **Suggestion Database**: More intelligent "did you mean?" using edit distance

See Also
========

- :doc:`parser` - How the parser integrates error reporting
- :doc:`lexer` - Lexer error detection and reporting
- :doc:`../testing` - Testing methodology and tools

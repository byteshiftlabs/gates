Usage
=====

Command Line
------------

.. code-block:: bash

   ./gates <input.c> <output.vhdl>

- **Input**: A C source file (``.c``). Must contain at least one function or
  struct definition.
- **Output**: A VHDL file (``.vhdl``). Created or overwritten.
- Both arguments are required.

Exit Codes
----------

- **0** — Compilation succeeded. VHDL output written.
- **1** — Error. Covers usage errors, file-not-found, and parse/codegen failures.

Output Format
-------------

Diagnostic messages are printed to stderr with the format::

   severity[Category] line N: message

Severity levels:

- ``info`` — Informational (e.g., "Parsing input file...")
- ``warning`` — Non-fatal issue (e.g., buffer truncation)
- ``error`` — Fatal issue that prevents VHDL generation

Categories: ``General``, ``Lexer``, ``Parser``, ``Semantic``, ``Codegen``.

Colors are enabled by default on terminals that support ANSI escape codes
(red for errors, yellow for warnings, blue for info).

Examples
--------

Successful compilation:

.. code-block:: bash

   $ ./gates examples/example.c output.vhdl
   info[General] Parsing input file 'examples/example.c'...
   info[General] Generating VHDL code...
   info[General] Compilation finished successfully.

Parse error:

.. code-block:: bash

   $ ./gates bad.c output.vhdl
   error[Parser] line 2: Expected ')' after parameter list
   error[General] Parsing failed with 1 error(s)

Missing file:

.. code-block:: bash

   $ ./gates nonexistent.c out.vhdl
   error[General] Error opening input file 'nonexistent.c': No such file or directory

See :doc:`examples` for full C-to-VHDL translation samples.

Developer Debug Output
----------------------

To enable verbose debug output, configure the build with ``-DDEBUG=ON``:

.. code-block:: bash

   cmake -DDEBUG=ON ..
   make

This enables additional diagnostic prints during parsing and code generation,
useful for debugging the compiler itself.

Testing Quick Reference
-----------------------

Unit tests use GoogleTest and are auto-discovered. Common commands:

.. code-block:: bash

   # Build + run everything
   cmake --build build --target test_all -j 4

   # List available tests
   ctest --test-dir build -N

   # Run subset by regex
   ctest --test-dir build -R UtilsTests

   # Run single test directly
   ./build/gates_tests --gtest_filter=TokenTests.BasicLexing

See :doc:`testing` for full details.


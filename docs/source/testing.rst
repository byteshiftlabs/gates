Testing
=======

The project uses **GoogleTest** for unit and integration testing. Tests are organized into subdirectories by type, with each test file containing a doxygen header describing its purpose and scope.

Test Organization
-----------------

Tests are organized by their purpose and scope:

**Unit Tests** (``tests/unit/``):
  Tests for individual components in isolation. Each file targets a specific module or subsystem:

  * ``basic_tests.cpp``: Core component tests (AST nodes, utilities, tokens, symbol tables)
  * ``edge_case_tests.cpp``: Boundary conditions and robustness tests
  * ``test_error_handler.cpp``: Error handler module tests

**Integration Tests** (``tests/integration/``):
  Tests that verify interaction between components and full pipeline behavior:

  * ``integration_tests.cpp``: Component boundary tests, end-to-end smoke tests, negative tests, and VHDL validation

Each test file begins with a doxygen header (``@file``, ``@brief``) documenting what is tested.

Quick Start
-----------

.. code-block:: bash

  # Full repository validation (same script used by Docker/CI)
  ./run_validation.sh

   # Configure with tests enabled (default ENABLE_TESTING=ON)
   cmake -S . -B build -DENABLE_TESTING=ON

   # Build and run all tests using the aggregate target
   cmake --build build --target test_all -j 4

   # Or use the helper script (configures if needed)
   ./run_tests.sh

The authoritative public validation path is ``./run_validation.sh``. GitHub Actions runs that script inside the repository Docker image, so local Docker verification and CI share the same command surface.

Running Tests
-------------

.. code-block:: bash

   # Run all tests with output on failure
   ctest --test-dir build --output-on-failure

   # List tests without running
   ctest --test-dir build -N

   # Run tests whose names match a regex
   ctest --test-dir build -R UtilsTests

   # Run a single test directly via GoogleTest filtering
   ./build/gates_tests --gtest_filter=TokenTests.BasicLexing

Convenience Targets
-------------------

.. code-block:: bash

   cmake --build build --target test_all   # Build and run all tests
   cmake --build build --target docs       # Build Sphinx docs

Adding New Tests
----------------

Add new test files to the appropriate subdirectory:

* **Unit tests**: Place in ``tests/unit/`` if testing a single module in isolation
* **Integration tests**: Place in ``tests/integration/`` if testing component interactions or end-to-end behavior

Each new test file should include a doxygen header documenting its purpose:

.. code-block:: cpp

   /**
    * @file my_new_tests.cpp
    * @brief Brief description of what this test file covers.
    *
    * More detailed description of:
    * - What components are tested
    * - What aspects are verified
    * - Any special setup or considerations
    */

CMake automatically discovers tests via ``file(GLOB_RECURSE)`` in both ``tests/unit/`` and ``tests/integration/`` subdirectories—no manual edit to ``CMakeLists.txt`` is required.

Internals / Notes
-----------------

* Tests link against the reusable ``gates_gtest`` static library (all C sources
  except the CLI ``gates.c``) to avoid duplicating logic.
* Some tests touch internal global variables (e.g. ``g_array_count``,
  ``current_token``). Future refactors may wrap these with a fixture to
  improve isolation.
* Enable additional parser/code generation diagnostics by configuring with
  ``-DDEBUG=ON``.

Test Strategy
-------------

The test suite follows a layered testing approach to verify correctness at multiple levels:

1. **Unit Tests**: Verify individual components in isolation (AST nodes, utilities, token parsing)
2. **Integration Tests**: Verify component boundaries and data flow between modules
3. **End-to-End Tests**: Verify complete pipeline behavior (smoke tests)
4. **Negative Tests**: Verify error handling and graceful degradation
5. **VHDL Validation Tests**: Verify output structure and correctness

This strategy ensures:

* **Component Isolation**: Failures are localized to specific modules via integration tests
* **Error Propagation**: Negative tests verify errors are detected and reported correctly
* **Output Correctness**: VHDL validation tests verify generated code has proper structure
* **Pipeline Stability**: End-to-end tests catch regressions in the full compilation flow

Test Types
----------

End-to-End Tests (``EndToEndTest`` fixture)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Full pipeline smoke tests that verify complete C → VHDL translation:

* **Purpose**: Catch regressions in the entire compilation pipeline
* **Scope**: Parse C source → Generate VHDL → Verify expected patterns exist
* **Method**: String pattern matching for VHDL keywords (``entity``, ``architecture``, ``signal``, etc.)
* **Example**: ``SimpleFunctionProducesEntity``, ``IfElseControlFlow``, ``WhileLoop``
* **Limitations**: Do not test component boundaries or error propagation in isolation

These tests verify that common C constructs translate successfully to VHDL output but do not validate VHDL correctness beyond basic structural patterns.

Integration Tests (``IntegrationTest`` fixture)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Component boundary tests that verify data flow between modules:

* **Purpose**: Verify components interact correctly through their interfaces
* **Scope**: Test parser→symbol tables and symbol tables→codegen boundaries
* **Method**: Direct inspection of symbol table state after parsing, verification that codegen reads from symbol tables
* **Examples**:

  * ``ParserPopulatesArraySymbolTable``: Verify parser registers arrays in symbol table
  * ``ParserPopulatesStructSymbolTable``: Verify parser registers structs in symbol table
  * ``CodegenReadsArraySymbolTable``: Verify codegen uses array information from symbol table
  * ``CodegenReadsStructSymbolTable``: Verify codegen uses struct information from symbol table

* **Key Benefit**: Isolates failures to specific component interactions

These tests ensure that the parser correctly populates symbol tables and that the code generator correctly reads from them, verifying the contract between components.

Negative Tests (``NegativeTest`` fixture)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Error case tests that verify graceful handling of invalid input:

* **Purpose**: Verify compiler handles malformed input without crashing
* **Scope**: Invalid C syntax, missing elements, unbalanced constructs
* **Method**: Verify error counters increment and parser returns expected results
* **Examples**:

  * ``InvalidSyntaxReturnsNull``: Missing closing parenthesis
  * ``MissingFunctionBodyReturnsNull``: Function declaration without definition
  * ``UnbalancedBracesReturnsNull``: Missing closing brace
  * ``EmptySourceProducesValidOutput``: Empty input file

* **Key Benefit**: Ensures compiler fails gracefully with clear error messages

These tests verify that the error handler is invoked correctly and that the parser doesn't crash or produce undefined behavior when given invalid input.

VHDL Validation Tests (``VHDLValidationTest`` fixture)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Output structure verification tests:

* **Purpose**: Verify generated VHDL has proper structure beyond simple pattern matching
* **Scope**: Library declarations, entity/architecture pairing, signal declarations, structural correctness
* **Method**: Helper functions that verify VHDL structural properties:

  * ``hasValidLibraryDeclarations()``: Checks for proper IEEE library imports
  * ``hasValidEntityArchitecture()``: Verifies entity appears before architecture, both have matching ``end`` statements
  * Counting/verification that multiple entities are generated correctly

* **Examples**:

  * ``SimpleFunction_HasValidStructure``: Verify basic VHDL structure elements
  * ``FunctionWithLocals_HasSignalDeclarations``: Verify local variables become signals
  * ``MultipleEntities_AllHaveValidStructure``: Verify multiple functions generate paired entities/architectures

* **Key Benefit**: Stronger validation than simple string matching, verifies structural correctness

These tests go beyond presence of keywords to verify the generated VHDL has proper structure that could be processed by VHDL tools.

Future Improvements
~~~~~~~~~~~~~~~~~~~

* **VHDL Parser**: Use a real VHDL parser to validate syntax correctness (current validation uses structural checks, not full parsing)
* **Mocking**: Add mock implementations for testing component boundaries without running real parser/codegen
* **Error Recovery**: Test parser's ability to continue after errors and report multiple issues
* **VHDL Semantics**: Validate generated VHDL is semantically correct (e.g., signal types match, no undefined references)

Planned Enhancements
--------------------

* Coverage reporting (gcov / lcov) optional target
* Test fixtures around global state

Current Boundary
----------------

The current validation contract stops at parser/codegen correctness, structural VHDL checks, smoke translation of the curated positive examples, documentation build health, and ``cppcheck``. External simulator or synthesis validation is intentionally out of scope until it is added to the public automation.

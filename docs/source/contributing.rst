Contributing
============

We welcome contributions to the C-to-VHDL compiler project!

Development Setup
-----------------

.. code-block:: bash

   git clone https://github.com/byteshiftlabs/compi.git
   cd compi && mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build .
   ctest   # Verify all 62 tests pass

Branch Naming
-------------

Branch names follow the pattern ``[label]/[brief-description]`` where the label
matches a GitHub label in the repository:

- ``feature/switch-case-support``
- ``fix/array-initializer-width``
- ``docs-refactor/expand-examples``
- ``tests/fuzz-parser``

Commit Messages
---------------

Use **past participle** for all commit messages. Be brief and list each change:

.. code-block:: text

   Added switch/case statement support
   - Added parse_switch() in parse_control_flow.c
   - Added VHDL case generation in codegen_vhdl_helpers.c
   - Added 8 tests covering nested cases and fallthrough

Pull Request Process
--------------------

1. Fork the repository and create a branch from ``main``.
2. Make your changes following the coding guidelines below.
3. Before submitting, verify:

   - **All tests pass**: ``cd build && ctest --output-on-failure``
   - **Zero compiler warnings**: ``cmake --build . 2>&1 | grep -iE 'warning|error'``
   - **cppcheck clean**: ``cppcheck --enable=all --suppress=missingIncludeSystem --suppressions-list=tools/cppcheck_suppressions.txt src/``
   - **Docs build**: ``cd docs && make html`` (if you changed documentation)

4. Submit a pull request with:

   - **Summary**: What was changed and why
   - **Changes**: Bullet list of specific modifications
   - **Testing**: How the changes were verified

Coding Guidelines
-----------------

We maintain high code quality standards throughout the project. Please review
our :doc:`code_quality` documentation for detailed guidelines on code
organization, naming conventions, helper functions, and best practices.

**Key principles:**

- Ensure code compiles and passes all tests before submitting
- Follow the established patterns and conventions in the codebase
- Write self-documenting code with clear, descriptive names

See the :doc:`code_quality` section for concrete examples and patterns.

Reporting Issues
----------------
- Please report bugs or feature requests via GitHub Issues.
- Include steps to reproduce, expected behavior, and relevant code snippets.

Algorithms
==========

Key algorithms used in the compiler implementation.

Precedence Climbing
-------------------

Expression parsing uses the **precedence climbing** algorithm (also known as
Pratt parsing). The core function ``parse_expression_prec()`` in
``parse_expression.c`` takes a minimum precedence level and builds a binary
expression tree by consuming operators left-to-right.

**Precedence levels** (defined in ``utils.h``):

.. list-table::
   :header-rows: 1

   * - Level
     - Operators
     - Meaning
   * - 7
     - ``*`` ``/``
     - Multiplicative
   * - 6
     - ``+`` ``-``
     - Additive
   * - 5
     - ``<<`` ``>>``
     - Shift
   * - 4
     - ``<`` ``<=`` ``>`` ``>=``
     - Relational
   * - 3
     - ``==`` ``!=``
     - Equality
   * - 2
     - ``&``
     - Bitwise AND
   * - 1
     - ``^``
     - Bitwise XOR
   * - 0
     - ``|``
     - Bitwise OR
   * - -1
     - ``&&``
     - Logical AND
   * - -2
     - ``||``
     - Logical OR

**Algorithm outline:**

1. Parse a primary expression (literal, identifier, parenthesized expression,
   unary operator, or function call).
2. While the current token is an operator with precedence >= ``min_prec``:

   a. Save the operator and its precedence.
   b. Advance past the operator.
   c. Recursively parse the right operand with ``min_prec = operator_prec + 1``
      (this ensures left-to-right associativity).
   d. Build a ``NODE_BINARY_EXPR`` with the operator, left operand, and right operand.
   e. The new binary node becomes the left operand for the next iteration.

3. Return the accumulated expression tree.

The entry point ``parse_expression()`` calls ``parse_expression_prec()`` with
``PREC_TOP_LEVEL_MIN (-2)`` to accept all operators. Higher-precedence operators
bind tighter, so ``3 + 4 * 5`` correctly parses as ``3 + (4 * 5)``.

Recursive Descent
-----------------

The parser uses **recursive descent** with keyword-driven dispatch.
Each grammar rule maps to a dedicated C function.

**Top-level dispatch** (``parse.c``):

The ``parse_program()`` function loops over tokens and dispatches based on the
first keyword:

- ``struct`` → ``parse_struct_declaration()``
- Type keyword (``int``, ``float``, etc.) → ``parse_function_declaration()``
- Anything else → error

**Statement dispatch** (``parse_statement.c``):

Inside function bodies, ``parse_statement()`` inspects the current token:

- Type keyword or ``struct`` → ``parse_variable_declaration()``
- Identifier → ``parse_assignment_or_expression()``
- ``return`` → ``parse_return_statement()``
- ``if`` → ``parse_if_statement()``
- ``while`` → ``parse_while_statement()``
- ``for`` → ``parse_for_statement()``
- ``break`` / ``continue`` → dedicated handlers

Each specialized parser consumes its tokens and returns an AST subtree.
Errors are reported through the centralized error handler with source location,
and parsing continues where possible.

For-Loop Desugaring
-------------------

C ``for`` loops have no direct VHDL equivalent. The parser desugars them
during AST construction rather than in a separate pass.

Given: ``for (int i = 0; i < n; i++) { body }``

The parser builds a ``NODE_FOR_STATEMENT`` with children:

.. code-block:: text

   NODE_FOR_STATEMENT
     ├─ NODE_VAR_DECL (int i = 0)       ← init
     ├─ NODE_BINARY_EXPR (i < n)         ← condition
     ├─ NODE_STATEMENT (body)            ← loop body
     └─ NODE_ASSIGNMENT (i = i + 1)      ← increment (desugared)

The key transformation is in ``parse_for_increment()`` (``parse_for.c``):

- ``i++`` is rewritten as the assignment ``i = i + 1``
- ``i--`` is rewritten as ``i = i - 1``
- Direct assignments (``i = i + 2``) are kept as-is

This desugaring means the code generator never sees ``++`` or ``--``
operators — only assignments and binary expressions it already handles.

The VHDL code generator emits this as initialization followed by a
``while`` loop with the condition, body, and increment inside the loop.

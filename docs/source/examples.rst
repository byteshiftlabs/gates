Examples
========

This section shows example C files and their actual generated VHDL output.
Every snippet below is pulled directly from ``examples/doc/`` with
``literalinclude`` — not pasted by hand — so the docs cannot show anything
other than what is actually on disk. ``ci/run_validation.sh`` regenerates
each ``.vhdl`` file from its ``.c`` source and fails the build if the
current ``gates`` build no longer produces byte-identical output, or if the
result no longer analyzes with GHDL; see :doc:`testing`. If that check ever
fails, regenerating the ``.vhdl`` files and re-running ``ghdl -a`` on them is
the fix — not editing this page.

The ``examples/`` folder also contains additional, larger input files not
reproduced here.

Basic Function
--------------

A simple function with a local variable and a return value.

**C input** (``examples/doc/basic_function.c``):

.. literalinclude:: ../../examples/doc/basic_function.c
   :language: c

**VHDL output:**

.. literalinclude:: ../../examples/doc/basic_function.vhdl
   :language: vhdl

Each C function becomes an entity with clock/reset ports, parameters as
``std_logic_vector`` inputs, and a ``result`` output. Local variables become
signals with zero-initialized reset logic. Arithmetic operands are wrapped in
``unsigned()`` and the result in ``std_logic_vector(...)``, per
:doc:`internals/codegen`.

Cross-function calls are still a documented limitation: multi-function inputs
emit separate entities, but gates does not yet synthesize the inter-entity
wiring needed to connect them automatically.

Control Flow (if/else)
----------------------

Conditional branching maps to VHDL ``if``/``elsif``/``else`` inside the
synchronous process.

**C input** (``examples/doc/control_flow.c``):

.. literalinclude:: ../../examples/doc/control_flow.c
   :language: c

**VHDL output:**

.. literalinclude:: ../../examples/doc/control_flow.vhdl
   :language: vhdl

Note the ``else`` branch, ``result <= x;``: a plain identifier returned as-is
is emitted without a cast — only literals and computed expressions get one.
A function with no local variables has no signal declarations and an empty
reset branch, which is why they are missing here compared to the ``add``
example above.

For Loop
--------

C ``for`` loops are desugared into an initialization assignment, a VHDL
``while`` loop over the condition, the body, and the increment emitted as the
last statement in the loop body.

**C input** (``examples/doc/for_loop.c``):

.. literalinclude:: ../../examples/doc/for_loop.c
   :language: c

**VHDL output:**

.. literalinclude:: ../../examples/doc/for_loop.vhdl
   :language: vhdl

``i = i + 1`` in the for-header desugars the same way ``i++`` would (see
:doc:`internals/parser`); every numeric literal is emitted inline as
``to_unsigned(N, 32)`` — there is no named-constant folding.

Structs
-------

C structs become VHDL record types, wrapped in a shared package. See
:doc:`internals/codegen` (Struct Support, and the full ``distance()``
example) for the generated output, verified the same way as the examples
above.

Running the Examples
--------------------

.. code-block:: bash

   # Compile any C file to VHDL
   ./build/gates examples/example.c output.vhdl

   # View the generated output
   cat output.vhdl

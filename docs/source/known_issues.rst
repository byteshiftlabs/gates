Known Issues
============

Last reviewed: |today|

Language Limitations
--------------------

.. list-table::
   :header-rows: 1
   :widths: 15 40 30

   * - Severity
     - Issue
     - Workaround / Status
   * - High
     - Global variable declarations are not handled. Variables defined outside
       functions are rejected.
     - Move all variables inside functions. Planned for Phase 2
       (see ``ROADMAP.md``).
   * - Medium
     - ``switch/case`` and ``do-while`` are not supported.
     - Use ``if/else`` chains and ``while`` loops. Planned for Phase 2.
   * - Medium
     - No pointer support (``&``, ``*``, pointer arithmetic).
     - Use array indexing for data access. Planned for Phase 2.
   * - Low
     - No nested structs or arrays of structs.
     - Flatten struct hierarchies. Planned for Phase 2.

VHDL Generation
---------------

.. list-table::
   :header-rows: 1
   :widths: 15 40 30

   * - Severity
     - Issue
     - Workaround / Status
   * - High
     - Function calls are parsed but VHDL generation does not create component
       instantiations. Multi-function files compile but functions are independent
       entities with no inter-entity wiring.
     - Keep functions self-contained (no cross-function calls that need hardware
       wiring). Planned for Phase 4.
   * - High
     - Several supported constructs emit VHDL that an analyzer rejects:
       initialized arrays produce a duplicate ``signal`` declaration; struct
       record types are emitted outside any design unit; struct field writes and
       some operands reference undeclared signals; a parameter named ``result``
       duplicates the output port; VHDL reserved words used as C identifiers are
       emitted unquoted; ``return f(x);`` is dropped, leaving ``result``
       undriven; array indexing in a condition emits C bracket syntax.
     - Review generated VHDL before use. Tracked as Phase 0 release blockers in
       ``ROADMAP.md``.
   * - Medium
     - VHDL codegen does not optimize for hardware resources or timing. No
       resource sharing, pipelining, constant folding, or dead code elimination.
     - Generated VHDL may use more hardware than necessary. Planned for Phase 5.
   * - Medium
     - Signal assignment is deferred, so a value assigned inside a clocked
       process is not visible to later statements in the same process until the
       next clock edge. Outputs are pipelined rather than combinational.
     - Inherent to the current translation model. Account for the extra cycle of
       latency when integrating generated entities.
   * - Low
     - Short-circuit evaluation (``&&`` / ``||``) uses pure combinational
       evaluation, not sequential C semantics.
     - Avoid side effects in boolean sub-expressions.

Robustness
----------

.. list-table::
   :header-rows: 1
   :widths: 15 40 30

   * - Severity
     - Issue
     - Workaround / Status
   * - High
     - Deeply nested expressions or statements exhaust the stack. There is no
       recursion depth limit in the parser, code generator, or ``free_node``.
     - Keep nesting shallow. Tracked as a Phase 0 release blocker.
   * - High
     - Empty conditions such as ``if ()`` or ``while ()`` crash code generation,
       because the parse failure is not recorded as an error.
     - Avoid empty conditions. Tracked as a Phase 0 release blocker.
   * - Medium
     - Array bounds checking is silently skipped when the array name exceeds 63
       characters, because the symbol table truncates on insert but looks up by
       exact match.
     - Keep array names short. Tracked as a Phase 0 release blocker.
   * - Medium
     - Parsing stops at the first error, so only one diagnostic is reported per
       run and the recovery helpers are unreachable.
     - Fix errors one at a time. Error recovery is planned for Phase 4.
   * - Medium
     - Identifiers longer than 127 characters are silently truncated in
       expressions while parameter lists keep the full name, so distinct
       identifiers can collapse into one undeclared signal.
     - Keep identifiers under 128 characters.
   * - Medium
     - ``int x = 1, y = 2;`` parses but silently discards every declarator after
       the first, and trailing garbage after an initializer is ignored.
     - Declare one variable per statement.
   * - Low
     - ``-DGATES_MAX_ARRAYS`` and ``-DGATES_MAX_STRUCTS`` are accepted by CMake
       but ignored; the limits are hardcoded in the symbol table headers.
     - Only ``GATES_VHDL_BIT_WIDTH`` and ``GATES_MAX_PARAMETERS`` take effect.
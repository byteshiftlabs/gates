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
   * - Medium
     - VHDL codegen does not optimize for hardware resources or timing. No
       resource sharing, pipelining, constant folding, or dead code elimination.
     - Generated VHDL may use more hardware than necessary, and automated
       simulator verification is not yet part of the regular test flow.
       Planned for Phase 5.
   * - Low
     - Short-circuit evaluation (``&&`` / ``||``) uses pure combinational
       evaluation, not sequential C semantics.
     - Avoid side effects in boolean sub-expressions.
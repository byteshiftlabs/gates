Known Issues
============

Last reviewed: |today|

- Signal name collision for 'result' in generated VHDL output ports.
- Global variable declarations are not handled.
- VHDL codegen does not optimize for hardware resources or timing.
- Short-circuit evaluation semantics (&& / ||) are not modeled exactly as in C (pure combinational evaluation used).
- Function calls: basic parsing implemented, but VHDL generation doesn't create component instantiations yet.
- Limited test coverage for full parser end-to-end pathways (unit-level focus so far).
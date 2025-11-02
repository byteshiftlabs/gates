Internals
=========

This section provides exhaustive documentation of the internal architecture, data structures, algorithms, and implementation details of the Compi C-to-VHDL compiler.

.. toctree::
   :maxdepth: 3
   :titlesonly:

   internals/overview
   internals/lexer
   internals/parser
   internals/ast
   internals/symbols
   internals/codegen
   internals/types
   internals/algorithms

Overview
--------

See :doc:`internals/overview` for a high-level architecture overview.

Components
----------

- **Lexer**: Tokenization and lexical analysis
- **Parser**: Syntax analysis and AST construction  
- **Symbol Tables**: Type checking and scope management
- **Code Generator**: VHDL output generation
- **Type System**: C to VHDL type mapping

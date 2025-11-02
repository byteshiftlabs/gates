Symbol Tables
=============

Symbol tables track variables, functions, types, and their scopes throughout the compilation process.

Location
--------

- Array symbols: ``src/symbols/symbol_arrays.c`` (``include/symbol_arrays.h``)
- Struct symbols: ``src/symbols/symbol_structs.c`` (``include/symbol_structs.h``)

Purpose
-------

Symbol tables maintain:

- Variable names and types
- Function signatures
- Struct definitions
- Array dimensions and element types
- Scope information (global vs local)

Implementation details coming soon...

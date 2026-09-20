Symbol Tables
=============

Symbol tables track variables, functions, types, and their scopes throughout the compilation process.

Location
--------

- Array symbols: ``src/symbols/symbol_arrays.c`` (``include/symbol_arrays.h``)
- Struct symbols: ``src/symbols/symbol_structs.c`` (``include/symbol_structs.h``)

Struct Table
------------

Stores struct type definitions with their fields.

.. code-block:: c

   typedef struct {
       char name[STRUCT_NAME_LENGTH];
       struct {
           char field_name[FIELD_NAME_LENGTH];
           char field_type[FIELD_TYPE_LENGTH];
       } fields[MAX_STRUCT_FIELDS];  // Up to 32 fields
       int field_count;
   } StructInfo;

Key operations:

- ``register_struct(name)`` — Returns the existing index if already registered (no duplicate entries), or registers and returns a new index; returns ``-1`` and logs an ``ERROR_CATEGORY_SEMANTIC`` error on a NULL name or a full table
- ``register_struct_field(index, type, name)`` — Adds a field (max ``MAX_STRUCT_FIELDS`` per struct); returns ``-1`` and logs an error on an invalid index, NULL type/name, or a full field list
- ``find_struct_index(name)`` — Linear search by struct name, returns ``-1`` if not found
- ``get_struct_info(index)`` — Bounds-checked access by index; returns ``NULL`` if out of range
- ``get_struct_count()`` — Current number of registered structs
- ``reset_struct_table()`` — Clears all entries between compilations

Global capacity: ``MAX_STRUCTS`` (``GATES_MAX_STRUCTS``, default 64). Used to emit VHDL record type definitions (see :doc:`codegen`).

Array Table
-----------

Stores array variable names and their sizes for bounds checking and VHDL generation.

.. code-block:: c

   typedef struct {
       char name[ARRAY_NAME_LENGTH];
       int size;
   } ArrayInfo;

Key operations:

- ``register_array(name, size)`` — Adds a new entry, or updates the size of an existing one with the same name (no duplicate entries); logs an ``ERROR_CATEGORY_SEMANTIC`` error and does nothing on a NULL name, ``size <= 0``, or a full table
- ``find_array_size(name)`` — Linear search by array name, returns size or ``-1`` if not found
- ``get_array_count()`` — Current number of registered arrays
- ``reset_array_table()`` — Clears all entries

Global capacity: ``MAX_ARRAYS`` (``GATES_MAX_ARRAYS``, default 128).

Scope Handling
--------------

Both tables are **flat/global** — no hierarchical scoping. All structs and arrays
live in a single namespace per compilation. Scope management is planned for Phase 2
(global variables) and would require distinguishing global vs local symbols.

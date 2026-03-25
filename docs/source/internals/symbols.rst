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

- ``register_struct(name)`` — Returns index, prevents duplicates
- ``register_struct_field(index, type, name)`` — Adds field (max 32 per struct)
- ``find_struct_index(name)`` — Linear search by struct name
- ``get_struct_info(index)`` — Direct access by stored index
- ``reset_struct_table()`` — Clears all entries between compilations

Global capacity: ``MAX_STRUCTS`` (64). Used to emit VHDL record type definitions.

Array Table
-----------

Stores array variable names and their sizes for bounds checking and VHDL generation.

.. code-block:: c

   typedef struct {
       char name[ARRAY_NAME_LENGTH];
       int size;
   } ArrayInfo;

Key operations:

- ``register_array(name, size)`` — Adds or updates (prevents duplicates)
- ``find_array_size(name)`` — Linear search by array name, returns size or -1
- ``reset_array_table()`` — Clears all entries

Global capacity: ``MAX_ARRAYS`` (128). Validates ``size > 0`` on registration.

Scope Handling
--------------

Both tables are **flat/global** — no hierarchical scoping. All structs and arrays
live in a single namespace per compilation. Scope management is planned for Phase 2
(global variables) and would require distinguishing global vs local symbols.

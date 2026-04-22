Type System
===========

The type system handles type checking and type coercion for C-to-VHDL compilation.

Supported Types
---------------

- ``int`` - 32-bit signed integer
- ``float`` - Floating-point (single precision)
- ``double`` - Floating-point (double precision)
- ``char`` - Single character
- ``void`` - No return type
- Arrays of the above types
- Structs (partial support)

C-to-VHDL Type Mapping
-----------------------

The ``ctype_to_vhdl()`` function in ``codegen_vhdl_helpers.c`` maps C types to VHDL
signal types using named constants defined in ``codegen_vhdl_constants.c``:

.. list-table::
   :header-rows: 1

   * - C Type
     - VHDL Type
     - Width
   * - ``int``
     - ``std_logic_vector(31 downto 0)``
     - 32-bit
   * - ``float``
     - ``std_logic_vector(31 downto 0)``
     - 32-bit (bit-level)
   * - ``double``
     - ``std_logic_vector(63 downto 0)``
     - 64-bit
   * - ``char``
     - ``std_logic_vector(7 downto 0)``
     - 8-bit
   * - Unknown
     - ``std_logic_vector(31 downto 0)``
     - 32-bit default

All numeric types use ``std_logic_vector`` for bit-level representation. IEEE 754
float support can be added by changing the ``VHDL_TYPE_FLOAT`` constant.

Type-Specific Formatting
------------------------

Array initializers and reset values use type-aware formatting
(``codegen_vhdl_types.c``):

- **Integers**: Converted to 32-bit binary strings (e.g., ``"00000000000000000000000000000101"``)
- **Floats/Doubles**: Passed through as-is (bit-level representation deferred)
- **Characters**: Wrapped in VHDL character quotes (e.g., ``'a'``)
- **Unknown types**: Emitted as-is

// VHDL Code Generator - Constants Implementation
// -------------------------------------------------------------

#include "codegen_vhdl_constants.h"

// -------------------------------------------------------------
// String constants
// -------------------------------------------------------------
const char * const RESERVED_PORT_NAME_RESULT = "result";
const char * const SIGNAL_SUFFIX_LOCAL = "_local";
const char * const UNKNOWN_IDENTIFIER = "unknown";
const char * const STRUCT_INIT_MARKER = "struct_init";
const char * const ARRAY_INIT_MARKER = "array_init";
const char * const DEFAULT_FUNCTION_NAME = "anon";
const char * const DEFAULT_ZERO_VALUE = "0";
const char * const VHDL_FALSE = "false";

// VHDL port name constants
const char * const VHDL_PORT_CLK = "clk";
const char * const VHDL_PORT_RESET = "reset";
const char * const VHDL_PORT_RESULT = "result";

// -------------------------------------------------------------
// C operator constants
// -------------------------------------------------------------
const char * const OP_EQUAL = "==";
const char * const OP_NOT_EQUAL = "!=";
const char * const OP_LESS_THAN = "<";
const char * const OP_LESS_EQUAL = "<=";
const char * const OP_GREATER_THAN = ">";
const char * const OP_GREATER_EQUAL = ">=";
const char * const OP_LOGICAL_AND = "&&";
const char * const OP_LOGICAL_OR = "||";
const char * const OP_LOGICAL_NOT = "!";
const char * const OP_BITWISE_AND = "&";
const char * const OP_BITWISE_OR = "|";
const char * const OP_BITWISE_XOR = "^";
const char * const OP_BITWISE_NOT = "~";
const char * const OP_SHIFT_LEFT = "<<";
const char * const OP_SHIFT_RIGHT = ">>";

// -------------------------------------------------------------
// VHDL operator constants
// -------------------------------------------------------------
const char * const VHDL_OP_EQUAL = "=";
const char * const VHDL_OP_NOT_EQUAL = "/=";
const char * const VHDL_OP_AND = " and ";
const char * const VHDL_OP_OR = " or ";

// -------------------------------------------------------------
// C type name constants
// -------------------------------------------------------------
const char * const C_TYPE_INT = "int";
const char * const C_TYPE_FLOAT = "float";
const char * const C_TYPE_DOUBLE = "double";
const char * const C_TYPE_CHAR = "char";

// VHDL type mapping strings.
// int and float both map to 32-bit std_logic_vector for now (bit-level mapping).
// When IEEE 754 float support is needed, change VHDL_TYPE_FLOAT to
// "ieee.float_pkg.float32" or an equivalent VHDL-2008 type.
const char * const VHDL_TYPE_INT    = "std_logic_vector(31 downto 0)";
const char * const VHDL_TYPE_FLOAT  = "std_logic_vector(31 downto 0)";
const char * const VHDL_TYPE_DOUBLE = "std_logic_vector(63 downto 0)";
const char * const VHDL_TYPE_CHAR   = "std_logic_vector(7 downto 0)";
const char * const VHDL_TYPE_DEFAULT = "std_logic_vector(31 downto 0)";
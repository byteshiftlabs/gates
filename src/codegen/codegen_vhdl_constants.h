// VHDL Code Generator - Constants and Configuration
// -------------------------------------------------------------
// Purpose: Centralized constants for VHDL code generation
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_CONSTANTS_H
#define CODEGEN_VHDL_CONSTANTS_H

#include "config.h"  // User-configurable limits

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------
// Buffer size constants (use config values)
// -------------------------------------------------------------
#define MAX_PARAMETERS GATES_MAX_PARAMETERS
#define MAX_BUFFER_SIZE GATES_MAX_BUFFER_SIZE
#define ARRAY_NAME_BUFFER_SIZE GATES_ARRAY_NAME_BUFFER_SIZE
#define ARRAY_INDEX_BUFFER_SIZE 64
#define ARRAY_SIZE_BUFFER_SIZE 32
// Bitstring buffer: VHDL_BIT_WIDTH binary digits + quotes + null + safety margin
#define BITSTRING_BUFFER_SIZE (GATES_VHDL_BIT_WIDTH + 8)

// -------------------------------------------------------------
// VHDL type constants (use config value)
// -------------------------------------------------------------
#define VHDL_BIT_WIDTH GATES_VHDL_BIT_WIDTH

// -------------------------------------------------------------
// Array index constants
// -------------------------------------------------------------
#define FIRST_CHILD_INDEX 0
#define FIRST_STATEMENT_INDEX 1

// -------------------------------------------------------------
// String constants
// -------------------------------------------------------------
extern const char * const RESERVED_PORT_NAME_RESULT;
extern const char * const SIGNAL_SUFFIX_LOCAL;
extern const char * const UNKNOWN_IDENTIFIER;
extern const char * const STRUCT_INIT_MARKER;
extern const char * const ARRAY_INIT_MARKER;
extern const char * const DEFAULT_FUNCTION_NAME;
extern const char * const DEFAULT_ZERO_VALUE;
extern const char * const VHDL_FALSE;

// VHDL port name constants (centralized to avoid scattered magic strings)
extern const char * const VHDL_PORT_CLK;
extern const char * const VHDL_PORT_RESET;
extern const char * const VHDL_PORT_RESULT;

// -------------------------------------------------------------
// C operator constants
// -------------------------------------------------------------
extern const char * const OP_EQUAL;
extern const char * const OP_NOT_EQUAL;
extern const char * const OP_LESS_THAN;
extern const char * const OP_LESS_EQUAL;
extern const char * const OP_GREATER_THAN;
extern const char * const OP_GREATER_EQUAL;
extern const char * const OP_LOGICAL_AND;
extern const char * const OP_LOGICAL_OR;
extern const char * const OP_LOGICAL_NOT;
extern const char * const OP_BITWISE_AND;
extern const char * const OP_BITWISE_OR;
extern const char * const OP_BITWISE_XOR;
extern const char * const OP_BITWISE_NOT;
extern const char * const OP_SHIFT_LEFT;
extern const char * const OP_SHIFT_RIGHT;

// -------------------------------------------------------------
// VHDL operator constants
// -------------------------------------------------------------
extern const char * const VHDL_OP_EQUAL;
extern const char * const VHDL_OP_NOT_EQUAL;
extern const char * const VHDL_OP_AND;
extern const char * const VHDL_OP_OR;

// -------------------------------------------------------------
// C type name constants
// -------------------------------------------------------------
extern const char * const C_TYPE_INT;
extern const char * const C_TYPE_FLOAT;
extern const char * const C_TYPE_DOUBLE;
extern const char * const C_TYPE_CHAR;

// VHDL type mapping strings — one per C primitive type.
// int and float currently share the same bit representation;
// float becomes ieee.float_pkg.float32 when IEEE 754 support is added.
extern const char * const VHDL_TYPE_INT;
extern const char * const VHDL_TYPE_FLOAT;
extern const char * const VHDL_TYPE_DOUBLE;
extern const char * const VHDL_TYPE_CHAR;
extern const char * const VHDL_TYPE_DEFAULT;

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_VHDL_CONSTANTS_H

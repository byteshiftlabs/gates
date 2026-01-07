// VHDL Code Generator - Constants and Configuration
// -------------------------------------------------------------
// Purpose: Centralized constants for VHDL code generation
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_CONSTANTS_H
#define CODEGEN_VHDL_CONSTANTS_H

// -------------------------------------------------------------
// Buffer size constants
// -------------------------------------------------------------
#define MAX_PARAMETERS 128
#define MAX_BUFFER_SIZE 256
#define ARRAY_NAME_BUFFER_SIZE 64
#define ARRAY_INDEX_BUFFER_SIZE 64
#define ARRAY_SIZE_BUFFER_SIZE 32
#define BITSTRING_BUFFER_SIZE 40

// -------------------------------------------------------------
// VHDL type constants
// -------------------------------------------------------------
#define VHDL_BIT_WIDTH 32

// -------------------------------------------------------------
// Array index constants
// -------------------------------------------------------------
#define FIRST_CHILD_INDEX 0
#define FIRST_STATEMENT_INDEX 1

// -------------------------------------------------------------
// String constants
// -------------------------------------------------------------
extern const char *RESERVED_PORT_NAME_RESULT;
extern const char *SIGNAL_SUFFIX_LOCAL;
extern const char *UNKNOWN_IDENTIFIER;
extern const char *STRUCT_INIT_MARKER;
extern const char *ARRAY_INIT_MARKER;
extern const char *DEFAULT_FUNCTION_NAME;
extern const char *DEFAULT_ZERO_VALUE;
extern const char *VHDL_FALSE;

// -------------------------------------------------------------
// C operator constants
// -------------------------------------------------------------
extern const char *OP_EQUAL;
extern const char *OP_NOT_EQUAL;
extern const char *OP_LESS_THAN;
extern const char *OP_LESS_EQUAL;
extern const char *OP_GREATER_THAN;
extern const char *OP_GREATER_EQUAL;
extern const char *OP_LOGICAL_AND;
extern const char *OP_LOGICAL_OR;
extern const char *OP_LOGICAL_NOT;
extern const char *OP_BITWISE_AND;
extern const char *OP_BITWISE_OR;
extern const char *OP_BITWISE_XOR;
extern const char *OP_BITWISE_NOT;
extern const char *OP_SHIFT_LEFT;
extern const char *OP_SHIFT_RIGHT;

// -------------------------------------------------------------
// VHDL operator constants
// -------------------------------------------------------------
extern const char *VHDL_OP_EQUAL;
extern const char *VHDL_OP_NOT_EQUAL;
extern const char *VHDL_OP_AND;
extern const char *VHDL_OP_OR;

// -------------------------------------------------------------
// C type name constants
// -------------------------------------------------------------
extern const char *C_TYPE_INT;
extern const char *C_TYPE_FLOAT;
extern const char *C_TYPE_DOUBLE;
extern const char *C_TYPE_CHAR;

// -------------------------------------------------------------
// Indentation constants
// -------------------------------------------------------------
extern const char *INDENT_LEVEL_0;
extern const char *INDENT_LEVEL_1;
extern const char *INDENT_LEVEL_2;
extern const char *INDENT_LEVEL_3;
extern const char *INDENT_LEVEL_4;

#endif // CODEGEN_VHDL_CONSTANTS_H

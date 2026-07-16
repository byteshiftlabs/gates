/**
 * @file config.h
 * @brief Compile-time configuration for gates.
 * 
 * These values can be overridden at compile time via -D flags:
 *   cmake -DGATES_VHDL_BIT_WIDTH=64 ..
 * 
 * Or by defining them before including this header.
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------
// VHDL Generation Configuration
// -------------------------------------------------------------

/**
 * Default bit width for integer types in generated VHDL.
 * Common values: 8, 16, 32, 64
 */
#ifndef GATES_VHDL_BIT_WIDTH
#define GATES_VHDL_BIT_WIDTH 32
#endif

// -------------------------------------------------------------
// Parser Limits
// -------------------------------------------------------------

/**
 * Maximum number of function parameters supported.
 * Exceeding this limit will result in a compile error.
 */
#ifndef GATES_MAX_PARAMETERS
#define GATES_MAX_PARAMETERS 128
#endif

/**
 * Maximum number of struct types that can be defined.
 */
#ifndef GATES_MAX_STRUCTS
#define GATES_MAX_STRUCTS 64
#endif

/**
 * Maximum number of fields per struct.
 */
#ifndef GATES_MAX_STRUCT_FIELDS
#define GATES_MAX_STRUCT_FIELDS 32
#endif

/**
 * Maximum number of arrays that can be tracked.
 */
#ifndef GATES_MAX_ARRAYS
#define GATES_MAX_ARRAYS 128
#endif

/**
 * Maximum nesting depth for expressions and statements.
 *
 * The parser is recursive descent, so input nesting consumes stack. This
 * bound keeps deeply nested input a diagnostic rather than a crash. It is
 * far above anything hand-written C reaches and far below the depth that
 * exhausts a default 8 MB stack.
 */
#ifndef GATES_MAX_PARSE_DEPTH
#define GATES_MAX_PARSE_DEPTH 512
#endif

// -------------------------------------------------------------
// Buffer Sizes (internal, generally no need to change)
// -------------------------------------------------------------

#ifndef GATES_MAX_BUFFER_SIZE
#define GATES_MAX_BUFFER_SIZE 256
#endif

#ifndef GATES_ARRAY_NAME_BUFFER_SIZE
#define GATES_ARRAY_NAME_BUFFER_SIZE 64
#endif

// -------------------------------------------------------------
// Validation
// -------------------------------------------------------------

#if GATES_VHDL_BIT_WIDTH < 8 || GATES_VHDL_BIT_WIDTH > 128
#error "GATES_VHDL_BIT_WIDTH must be between 8 and 128"
#endif

#if GATES_MAX_PARAMETERS < 1 || GATES_MAX_PARAMETERS > 1024
#error "GATES_MAX_PARAMETERS must be between 1 and 1024"
#endif

#if GATES_MAX_PARSE_DEPTH < 8 || GATES_MAX_PARSE_DEPTH > 100000
#error "GATES_MAX_PARSE_DEPTH must be between 8 and 100000"
#endif

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H

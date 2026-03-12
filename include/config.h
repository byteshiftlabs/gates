/**
 * @file config.h
 * @brief Compile-time configuration for compi.
 * 
 * These values can be overridden at compile time via -D flags:
 *   cmake -DCOMPI_VHDL_BIT_WIDTH=64 ..
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
#ifndef COMPI_VHDL_BIT_WIDTH
#define COMPI_VHDL_BIT_WIDTH 32
#endif

// -------------------------------------------------------------
// Parser Limits
// -------------------------------------------------------------

/**
 * Maximum number of function parameters supported.
 * Exceeding this limit will result in a compile error.
 */
#ifndef COMPI_MAX_PARAMETERS
#define COMPI_MAX_PARAMETERS 128
#endif

/**
 * Maximum number of struct types that can be defined.
 */
#ifndef COMPI_MAX_STRUCTS
#define COMPI_MAX_STRUCTS 64
#endif

/**
 * Maximum number of fields per struct.
 */
#ifndef COMPI_MAX_STRUCT_FIELDS
#define COMPI_MAX_STRUCT_FIELDS 32
#endif

/**
 * Maximum number of arrays that can be tracked.
 */
#ifndef COMPI_MAX_ARRAYS
#define COMPI_MAX_ARRAYS 128
#endif

// -------------------------------------------------------------
// Buffer Sizes (internal, generally no need to change)
// -------------------------------------------------------------

#ifndef COMPI_MAX_BUFFER_SIZE
#define COMPI_MAX_BUFFER_SIZE 256
#endif

#ifndef COMPI_ARRAY_NAME_BUFFER_SIZE
#define COMPI_ARRAY_NAME_BUFFER_SIZE 64
#endif

// -------------------------------------------------------------
// Validation
// -------------------------------------------------------------

#if COMPI_VHDL_BIT_WIDTH < 8 || COMPI_VHDL_BIT_WIDTH > 128
#error "COMPI_VHDL_BIT_WIDTH must be between 8 and 128"
#endif

#if COMPI_MAX_PARAMETERS < 1 || COMPI_MAX_PARAMETERS > 1024
#error "COMPI_MAX_PARAMETERS must be between 1 and 1024"
#endif

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H

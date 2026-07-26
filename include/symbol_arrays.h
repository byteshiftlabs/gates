#ifndef SYMBOL_ARRAYS_H
#define SYMBOL_ARRAYS_H

/**
 * @file symbol_arrays.h
 * @brief Array symbol table for tracking declared arrays and their sizes.
 */

#include "config.h"

// Table and buffer limits. The table size follows the configured
// GATES_MAX_ARRAYS so the documented -D override actually takes effect.
#define MAX_ARRAYS        GATES_MAX_ARRAYS
#define ARRAY_NAME_LENGTH  GATES_ARRAY_NAME_BUFFER_SIZE

#ifdef __cplusplus
extern "C" {
#endif

// Simple per-function array table info (used for static bounds checking)
typedef struct {
    char name[ARRAY_NAME_LENGTH];
    int size; // number of elements
} ArrayInfo;

/**
 * @brief Register an array declaration in the global table.
 * @param name  Array variable name.
 * @param size  Number of elements.
 */
void register_array(const char *name, int size);

/**
 * @brief Look up the declared size of an array.
 * @param name  Array variable name.
 * @return Number of elements, or -1 if not found.
 */
int find_array_size(const char *name);

/**
 * @brief Get the current number of registered arrays.
 * @return Count of arrays in the table.
 */
int get_array_count(void);

/**
 * @brief Reset the array table (clear all entries).
 */
void reset_array_table(void);

#ifdef __cplusplus
}
#endif

#endif // SYMBOL_ARRAYS_H

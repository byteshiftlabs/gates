#ifndef UTILS_H
#define UTILS_H

/**
 * @file utils.h
 * @brief Shared utility functions, operator precedence constants, and helpers.
 */

#include <stddef.h>
#include "astnode.h"  // ASTNode definition

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Safely append a string to a buffer, preventing overflow.
 * @param dst       Destination buffer.
 * @param dst_size  Total size of destination buffer.
 * @param src       Source string to append.
 */
void safe_append(char *dst, size_t dst_size, const char *src);

/**
 * @brief Safely copy a string to a buffer with a character limit.
 * @param dst       Destination buffer.
 * @param dst_size  Total size of destination buffer.
 * @param src       Source string to copy.
 * @param limit     Maximum characters to copy.
 */
void safe_copy(char *dst, size_t dst_size, const char *src, size_t limit);

// Operator precedence constants (mirrors C precedence ordering)
// Higher number = higher precedence
#define PREC_MULTIPLICATIVE   7   // * /
#define PREC_ADDITIVE         6   // + -
#define PREC_SHIFT            5   // << >>
#define PREC_RELATIONAL       4   // < <= > >=
#define PREC_EQUALITY         3   // == !=
#define PREC_BITWISE_AND      2   // &
#define PREC_BITWISE_XOR      1   // ^
#define PREC_BITWISE_OR       0   // |
#define PREC_LOGICAL_AND     -1   // &&
#define PREC_LOGICAL_OR      -2   // || (lowest)
#define PREC_UNKNOWN       -999   // Unknown operator

// Expression parsing precedence bounds
#define PREC_PARENTHESIZED_MIN  1   // Minimum precedence for parenthesized expressions
#define PREC_TOP_LEVEL_MIN     -2   // Minimum precedence for top-level expressions

/**
 * @brief Pretty-print an AST tree to stdout.
 * @param node   Root node to print.
 * @param level  Current indentation level (pass 0 for root).
 */
void print_ast(ASTNode* node, int level);

/**
 * @brief Check if a string represents an integer (with optional leading sign).
 * @param str  String to check.
 * @return 1 if valid integer string, 0 otherwise.
 */
int is_number_str(const char *str);

/**
 * @brief Get the precedence level of a C operator.
 * @param op  Operator string (e.g. "+", "==", "&&").
 * @return Precedence value (higher = tighter binding), or PREC_UNKNOWN.
 */
int get_precedence(const char *op);

/**
 * @brief Safely convert a string to an integer using strtol.
 * @param str      String to convert.
 * @param result   Pointer to store the converted value.
 * @return 1 on success, 0 on failure (invalid input or overflow).
 */
int safe_strtoi(const char *str, int *result);

/**
 * @brief Duplicate a string, exiting on allocation failure.
 * @param src  String to duplicate (NULL returns NULL).
 * @return Newly allocated copy of src.
 */
char *safe_strdup(const char *src);

#ifdef __cplusplus
}
#endif

#endif
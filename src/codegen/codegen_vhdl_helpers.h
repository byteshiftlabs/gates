// VHDL Code Generator - Helper Utilities
// -------------------------------------------------------------
// Purpose: Utility functions for name mapping, type checking
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_HELPERS_H
#define CODEGEN_VHDL_HELPERS_H

#include <stdio.h>
#include "astnode.h"

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------
// Signal name mapping and VHDL identifier emission
// -------------------------------------------------------------
void emit_mapped_signal_name(const char *variable_name);

/**
 * Emit a sanitized VHDL identifier via the emitter.
 * If the name is already valid VHDL, emits directly.
 * Otherwise, sanitizes to ensure VHDL compliance.
 */
void emit_safe_identifier(const char *name);

// -------------------------------------------------------------
// Type checking
// -------------------------------------------------------------
int is_boolean_comparison_operator(const char *op);
int is_node_boolean_expression(const ASTNode *node);
int is_plain_identifier(const char *expression_value);

// -------------------------------------------------------------
// Numeric literal detection
// -------------------------------------------------------------
int is_numeric_literal(const char *value);
int is_negative_numeric_literal(const char *value);

// -------------------------------------------------------------
// Type conversion utilities
// -------------------------------------------------------------
const char* ctype_to_vhdl(const char* ctype);
int is_negative_literal(const char* value);
void emit_unsigned_cast(const char *value);
void emit_signed_cast(const char *value);
void emit_typed_operand(ASTNode *operand, int is_signed, void (*node_generator)(ASTNode*));

// -------------------------------------------------------------
// VHDL identifier validation
// -------------------------------------------------------------
/**
 * Check if a string is a valid VHDL identifier.
 * Rules: starts with letter, contains only letters/digits/underscores,
 * no consecutive underscores, doesn't end with underscore.
 * @return 1 if valid, 0 if invalid
 */
int is_valid_vhdl_identifier(const char *name);

/**
 * Sanitize a C identifier to be valid in VHDL.
 * - Prefixes with 'c_' if starts with digit or underscore
 * - Replaces consecutive underscores
 * - Checks against VHDL reserved words
 * @param name Input identifier
 * @param buffer Output buffer for sanitized name
 * @param buffer_size Size of output buffer
 * @return 1 on success, 0 if buffer too small
 */
int sanitize_vhdl_identifier(const char *name, char *buffer, size_t buffer_size);

// -------------------------------------------------------------
// Statement utilities
// -------------------------------------------------------------
ASTNode* unwrap_statement_node(ASTNode *node);

// -------------------------------------------------------------
// Struct field utilities
// -------------------------------------------------------------
void emit_struct_field_assignments(int struct_index, const char *target_name, 
                                   const char *source_name);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_VHDL_HELPERS_H

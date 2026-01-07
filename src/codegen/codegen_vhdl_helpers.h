// VHDL Code Generator - Helper Utilities
// -------------------------------------------------------------
// Purpose: Utility functions for name mapping, type checking
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_HELPERS_H
#define CODEGEN_VHDL_HELPERS_H

#include <stdio.h>
#include "astnode.h"

// -------------------------------------------------------------
// Signal name mapping
// -------------------------------------------------------------
int is_signal_name_reserved(const char *variable_name);
void emit_mapped_signal_name(const char *variable_name, FILE *output_file);

// -------------------------------------------------------------
// Type checking
// -------------------------------------------------------------
int is_boolean_comparison_operator(const char *operator);
int is_node_boolean_expression(ASTNode *node);
int is_plain_identifier(const char *expression_value);

// -------------------------------------------------------------
// Numeric literal detection
// -------------------------------------------------------------
int is_numeric_literal(const char *value);
int is_negative_numeric_literal(const char *value);

// -------------------------------------------------------------
// Type conversion utilities
// -------------------------------------------------------------
void emit_unsigned_cast(const char *value, FILE *output_file);
void emit_signed_cast(const char *value, FILE *output_file);
void emit_typed_operand(ASTNode *operand, FILE *output_file, int is_signed, void (*node_generator)(ASTNode*, FILE*));

// -------------------------------------------------------------
// Array utilities
// -------------------------------------------------------------
int parse_array_declaration(const char *value, char *name_out, int name_size, int *size_out);
void emit_array_type_and_signal(const char *name, const char *type, int size, FILE *output_file);

// -------------------------------------------------------------
// Statement utilities
// -------------------------------------------------------------
ASTNode* unwrap_statement_node(ASTNode *node);

// -------------------------------------------------------------
// Struct field utilities
// -------------------------------------------------------------
void emit_struct_field_assignments(int struct_index, const char *target_name, 
                                   const char *source_name, FILE *output_file, 
                                   const char *indentation);

#endif // CODEGEN_VHDL_HELPERS_H

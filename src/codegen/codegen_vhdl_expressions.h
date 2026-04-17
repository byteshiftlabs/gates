// VHDL Code Generator - Expression Generation
// -------------------------------------------------------------
// Purpose: Generate VHDL code for expressions (binary, unary, etc.)
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_EXPRESSIONS_H
#define CODEGEN_VHDL_EXPRESSIONS_H

#include <stdio.h>
#include "astnode.h"

// -------------------------------------------------------------
// Expression generation
// -------------------------------------------------------------
void generate_binary_expression(ASTNode *node, FILE *output_file);
void generate_expression(ASTNode *node, FILE *output_file);
void generate_unary_operation(ASTNode *node, FILE *output_file);
void generate_function_call(ASTNode *node, FILE *output_file);

// -------------------------------------------------------------
// Expression emission helpers
// -------------------------------------------------------------
void emit_array_element_access(const char *array_expression, FILE *output_file);
void emit_conditional_expression(ASTNode *condition, FILE *output_file);
void emit_boolean_gate_expression(ASTNode *left_operand, ASTNode *right_operand, 
                                  const char *logical_operator, FILE *output_file);

#endif // CODEGEN_VHDL_EXPRESSIONS_H

// VHDL Code Generator - Expression Generation
// -------------------------------------------------------------
// Purpose: Generate VHDL code for expressions (binary, unary, etc.)
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_EXPRESSIONS_H
#define CODEGEN_VHDL_EXPRESSIONS_H

#include "astnode.h"

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------
// Expression generation
// -------------------------------------------------------------
void generate_binary_expression(ASTNode *node, void (*node_generator)(ASTNode*));
void generate_expression(ASTNode *node);
void generate_unary_operation(ASTNode *node, void (*node_generator)(ASTNode*));
void generate_function_call(ASTNode *node, void (*node_generator)(ASTNode*));

// -------------------------------------------------------------
// Expression emission helpers
// -------------------------------------------------------------
void emit_conditional_expression(ASTNode *condition, void (*node_generator)(ASTNode*));

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_VHDL_EXPRESSIONS_H

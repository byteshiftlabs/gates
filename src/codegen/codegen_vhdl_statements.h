// VHDL Code Generator - Statement Generation
// -------------------------------------------------------------
// Purpose: Generate VHDL code for statements (assignments, loops, if/else)
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_STATEMENTS_H
#define CODEGEN_VHDL_STATEMENTS_H

#include "astnode.h"

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------
// Statement generation
// -------------------------------------------------------------
void generate_statement_block(ASTNode *node, void (*node_generator)(ASTNode*));
void generate_while_loop(ASTNode *node, void (*node_generator)(ASTNode*));
void generate_for_loop(ASTNode *node, void (*node_generator)(ASTNode*));
void generate_if_statement(ASTNode *node, void (*node_generator)(ASTNode*));
void generate_break_statement(ASTNode *node);
void generate_continue_statement(ASTNode *node);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_VHDL_STATEMENTS_H

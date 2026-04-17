// VHDL Code Generator - Statement Generation
// -------------------------------------------------------------
// Purpose: Generate VHDL code for statements (assignments, loops, if/else)
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_STATEMENTS_H
#define CODEGEN_VHDL_STATEMENTS_H

#include <stdio.h>
#include "astnode.h"

// -------------------------------------------------------------
// Statement generation
// -------------------------------------------------------------
void generate_statement_block(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*));
void generate_while_loop(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*));
void generate_for_loop(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*));
void generate_if_statement(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*));
void generate_break_statement(ASTNode *node, FILE *output_file);
void generate_continue_statement(ASTNode *node, FILE *output_file);

// -------------------------------------------------------------
// Statement emission helpers
// -------------------------------------------------------------
void emit_variable_initializer(ASTNode *declaration, FILE *output_file, const char *indentation, void (*node_generator)(ASTNode*, FILE*));
void emit_variable_assignment(ASTNode *assignment, FILE *output_file, const char *indentation, void (*node_generator)(ASTNode*, FILE*));
void emit_struct_field_initializations(ASTNode *var_decl, int struct_index, 
                                       ASTNode *initializer, FILE *output_file, void (*node_generator)(ASTNode*, FILE*));
void emit_expression_as_return(ASTNode *expression, ASTNode *parent_statement, 
                              FILE *output_file, void (*node_generator)(ASTNode*, FILE*));
void emit_struct_field_copy_to_result(ASTNode *expression, ASTNode *function_node, 
                                      FILE *output_file, const char *indentation);

#endif // CODEGEN_VHDL_STATEMENTS_H

// VHDL Code Generator - Type and Signal Declarations
// -------------------------------------------------------------
// Purpose: Generate VHDL type declarations and signal declarations
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_TYPES_H
#define CODEGEN_VHDL_TYPES_H

#include <stdio.h>
#include "astnode.h"

// -------------------------------------------------------------
// Struct type declarations
// -------------------------------------------------------------
void emit_all_struct_declarations(FILE *output_file);

// -------------------------------------------------------------
// Signal declarations
// -------------------------------------------------------------
void emit_function_local_signals(ASTNode *function_declaration, FILE *output_file);
void emit_struct_signal_declaration(ASTNode *var_decl, FILE *output_file);
void emit_array_signal_declaration(ASTNode *var_decl, FILE *output_file);
void emit_simple_signal_declaration(ASTNode *var_decl, FILE *output_file);

// -------------------------------------------------------------
// Array helpers
// -------------------------------------------------------------
int parse_array_dimensions(const char *var_name, char *array_name, 
                          char *array_size, int name_buf_size, int size_buf_size);
void emit_array_initializer_constant(ASTNode *var_decl, ASTNode *init_list, 
                                     const char *array_name, FILE *output_file);

// -------------------------------------------------------------
// Declaration processing
// -------------------------------------------------------------
void process_variable_declaration_for_signals(ASTNode *var_decl, FILE *output_file);
void process_for_loop_declarations(ASTNode *for_statement, FILE *output_file);

#endif // CODEGEN_VHDL_TYPES_H

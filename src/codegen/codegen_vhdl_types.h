// VHDL Code Generator - Type and Signal Declarations
// -------------------------------------------------------------
// Purpose: Generate VHDL type declarations and signal declarations
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_TYPES_H
#define CODEGEN_VHDL_TYPES_H

#include "astnode.h"

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------
// Struct type declarations
// -------------------------------------------------------------
void emit_all_struct_declarations(void);

// -------------------------------------------------------------
// Signal declarations
// -------------------------------------------------------------
void emit_function_local_signals(ASTNode *function_declaration);

/**
 * Emit reset logic for all local signals in a function.
 * Initializes all declared signals to their default values (0, '0', etc.)
 */
void emit_function_reset_logic(ASTNode *function_declaration);


#ifdef __cplusplus
}
#endif

#endif // CODEGEN_VHDL_TYPES_H

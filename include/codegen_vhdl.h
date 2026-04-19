#ifndef CODEGEN_VHDL_H
#define CODEGEN_VHDL_H

/**
 * @file codegen_vhdl.h
 * @brief Top-level VHDL code generation interface.
 */

#include <stdio.h>
#include "astnode.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate VHDL output from a parsed AST.
 * @param root    Root AST node (typically NODE_PROGRAM).
 * @param output_file  Destination file handle for the generated VHDL.
 */
void generate_vhdl(ASTNode* root, FILE* output_file);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_VHDL_H

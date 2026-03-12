#ifndef PARSE_H
#define PARSE_H

/**
 * @file parse.h
 * @brief Top-level parsing interface and sub-parser includes.
 */

#include "astnode.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse an entire C source file into an AST.
 * @param input  Source file handle to read from.
 * @return Root AST node (NODE_PROGRAM), or NULL on failure.
 */
ASTNode* parse_program(FILE *input);

#ifdef __cplusplus
}
#endif

#endif
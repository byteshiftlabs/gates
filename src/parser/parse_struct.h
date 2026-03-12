#ifndef PARSE_STRUCT_H
#define PARSE_STRUCT_H

/**
 * @file parse_struct.h
 * @brief Struct definition parser.
 */

#include "astnode.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a struct definition: struct Name { fields... };
 * @param ctx             Parser context.
 * @param struct_name_token Token holding the struct's name.
 * @return AST node (NODE_STRUCT_DECL) with field declarations as children.
 */
ASTNode* parse_struct(ParserContext *ctx, Token struct_name_token);

#ifdef __cplusplus
}
#endif

#endif // PARSE_STRUCT_H

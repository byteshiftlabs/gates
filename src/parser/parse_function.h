#ifndef PARSE_FUNCTION_H
#define PARSE_FUNCTION_H

/**
 * @file parse_function.h
 * @brief Function definition parser.
 */

#include "astnode.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a function definition (return type, name, params, body).
 * @param ctx          Parser context.
 * @param return_type     Token holding the function's return type.
 * @param function_name   Token holding the function's name.
 * @return AST node (NODE_FUNCTION_DECL) with parameters and body as children.
 */
ASTNode* parse_function(ParserContext *ctx, Token return_type, Token function_name);

#ifdef __cplusplus
}
#endif

#endif // PARSE_FUNCTION_H

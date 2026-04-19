#ifndef PARSE_EXPRESSION_H
#define PARSE_EXPRESSION_H

/**
 * @file parse_expression.h
 * @brief Expression parser with Pratt/precedence-climbing.
 */

#include "astnode.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a primary expression (literal, identifier, parenthesized expr).
 * @param ctx  Parser context.
 * @return AST node for the primary expression.
 */
ASTNode* parse_primary(ParserContext *ctx);

/**
 * @brief Parse an expression using precedence climbing.
 * @param ctx       Parser context.
 * @param min_prec  Minimum precedence level to bind.
 * @return AST node for the parsed expression.
 */
ASTNode* parse_expression_prec(ParserContext *ctx, int min_prec);

/**
 * @brief Parse a top-level expression (lowest precedence).
 * @param ctx  Parser context.
 * @return AST node for the parsed expression.
 */
ASTNode* parse_expression(ParserContext *ctx);

/**
 * @brief Parse a function call's argument list.
 * @param ctx            Parser context.
 * @param function_name  Name of the function being called.
 * @return AST node (NODE_FUNC_CALL) with arguments as children.
 */
ASTNode* parse_function_call_args(ParserContext *ctx, const char *function_name);

#ifdef __cplusplus
}
#endif

#endif // PARSE_EXPRESSION_H

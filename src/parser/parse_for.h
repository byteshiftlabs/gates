#ifndef PARSE_FOR_H
#define PARSE_FOR_H

/**
 * @file parse_for.h
 * @brief For-loop parser — separated from control flow to keep file sizes manageable.
 */

#include "astnode.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a for loop (init; condition; increment).
 * @param ctx  Parser context.
 * @return AST node (NODE_FOR_STATEMENT) with init, condition, increment, body children.
 */
ASTNode* parse_for_statement(ParserContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // PARSE_FOR_H

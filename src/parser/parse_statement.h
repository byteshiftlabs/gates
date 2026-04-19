#ifndef PARSE_STATEMENT_H
#define PARSE_STATEMENT_H

/**
 * @file parse_statement.h
 * @brief Statement parser (declarations, assignments, returns, dispatch).
 */

#include "astnode.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a single statement and return its AST node.
 * @param ctx  Parser context.
 * @return AST node for the parsed statement.
 */
ASTNode* parse_statement(ParserContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // PARSE_STATEMENT_H

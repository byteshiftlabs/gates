#ifndef PARSE_CONTROL_FLOW_H
#define PARSE_CONTROL_FLOW_H

/**
 * @file parse_control_flow.h
 * @brief Control flow statement parsers (if, while, break, continue)
 *        and loop-depth tracking shared with parse_for.c.
 */

#include "astnode.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse an if/else-if/else statement chain.
 * @param ctx  Parser context.
 * @return AST node (NODE_IF_STATEMENT) with condition and body children.
 */
ASTNode* parse_if_statement(ParserContext *ctx);

/**
 * @brief Parse a while loop.
 * @param ctx  Parser context.
 * @return AST node (NODE_WHILE_STATEMENT) with condition and body children.
 */
ASTNode* parse_while_statement(ParserContext *ctx);

/**
 * @brief Increment the loop nesting depth (call before parsing loop body).
 */
void loop_depth_inc(void);

/**
 * @brief Decrement the loop nesting depth (call after parsing loop body).
 */
void loop_depth_dec(void);

/**
 * @brief Parse a break statement.
 * @param ctx  Parser context.
 * @return AST node (NODE_BREAK_STATEMENT).
 */
ASTNode* parse_break_statement(ParserContext *ctx);

/**
 * @brief Parse a continue statement.
 * @param ctx  Parser context.
 * @return AST node (NODE_CONTINUE_STATEMENT).
 */
ASTNode* parse_continue_statement(ParserContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // PARSE_CONTROL_FLOW_H

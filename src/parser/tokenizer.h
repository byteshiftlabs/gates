// Tokenizer - Internal Parser API
// -------------------------------------------------------------
// Purpose: Lexer functions used only within the parser module.
//          Public token types live in include/token.h.
// -------------------------------------------------------------

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if a string is a C keyword.
 * @param str  String to check.
 * @return 1 if keyword, 0 otherwise.
 */
int is_keyword(const char *str);

/**
 * @brief Check if a string is a C primitive type keyword (int, float, char, double).
 * @param str  String to check.
 * @return 1 if type keyword, 0 otherwise.
 */
int is_type_keyword(const char *str);

/**
 * @brief Read the next token from the input stream.
 * @param ctx  Parser context (updates current_line on newlines).
 * @return The next Token.
 */
Token get_next_token(ParserContext *ctx);

/**
 * @brief Advance to the next token (reads and stores in ctx->current_token).
 * @param ctx  Parser context.
 */
void advance(ParserContext *ctx);

/**
 * @brief Check if ctx->current_token matches the expected type.
 * @param ctx   Parser context.
 * @param type  Expected token type.
 * @return 1 if match, 0 otherwise.
 */
int match(const ParserContext *ctx, TokenType type);

/**
 * @brief Consume the current token if it matches, otherwise report error.
 * @param ctx   Parser context.
 * @param type  Expected token type.
 * @return 1 on success, 0 on mismatch.
 */
int consume(ParserContext *ctx, TokenType type);

#ifdef __cplusplus
}
#endif

#endif // TOKENIZER_H

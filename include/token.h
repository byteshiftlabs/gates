#ifndef TOKEN_H
#define TOKEN_H

/**
 * @file token.h
 * @brief Lexer token types, Token structure, ParserContext, and tokenizer interface.
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Token types
typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_SEMICOLON,
    TOKEN_PARENTHESIS_OPEN,
    TOKEN_PARENTHESIS_CLOSE,
    TOKEN_BRACE_OPEN,
    TOKEN_BRACE_CLOSE,
    TOKEN_BRACKET_OPEN,
    TOKEN_BRACKET_CLOSE,
    TOKEN_COMMA,
    TOKEN_EOF
} TokenType;

// Token value buffer size
#define TOKEN_VALUE_SIZE 256

// Token structure
typedef struct {
    TokenType type;
    char value[TOKEN_VALUE_SIZE];
    int line;
} Token;

/**
 * @brief Parser context — encapsulates all mutable parsing state.
 *
 * Eliminates global mutable state, making the parser reentrant and testable.
 * Create one per parse invocation and pass it to all parser functions.
 */
typedef struct {
    Token current_token;   /**< Most recently read token. */
    int current_line;      /**< Current source line number (1-based). */
    FILE *input;           /**< Source file being parsed. */
} ParserContext;

/**
 * @brief Initialize a ParserContext for parsing.
 * @param ctx    Context to initialize.
 * @param input  Source file handle.
 */
void parser_context_init(ParserContext *ctx, FILE *input);

#ifdef __cplusplus
}
#endif

#endif // TOKEN_H
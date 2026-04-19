#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "error_handler.h"
#include "tokenizer.h"

#define MAX_TOKEN_VALUE_LEN (TOKEN_VALUE_SIZE - 1)

// Keywords drive parser dispatch — recognized tokens bypass identifier handling.
// NOTE: Type keywords (int, float, char, double) must also appear in
// type_keywords[] below. Keep both lists in sync when adding new types.
static const char *keywords[] = {
    "if", "else", "while", "for", "return", "break", "continue",
    "struct",
    "int", "float", "char", "double", "void",
    NULL
};

// Zero the context so no stale state leaks between parse invocations
void parser_context_init(ParserContext *ctx, FILE *input)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->current_line = 1;
    ctx->input = input;
}

// Advance consumes the current token so the parser always looks one token ahead
void advance(ParserContext *ctx)
{
    ctx->current_token = get_next_token(ctx);
}


// Non-destructive lookahead — lets the parser branch without consuming
int match(const ParserContext *ctx, TokenType type)
{
    return ctx->current_token.type == type;
}


// Combines match + advance for mandatory syntax elements
int consume(ParserContext *ctx, TokenType type)
{
    if (match(ctx, type)) {
        advance(ctx);
        return 1;
    }
    return 0;
}

// Check if a string is a keyword
int is_keyword(const char *str)
{
    for (int keyword_idx = 0; keywords[keyword_idx] != NULL; keyword_idx++) {
        if (strcmp(str, keywords[keyword_idx]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Subset of keywords[] that are C primitive types — used by the parser to
// distinguish variable declarations from other keyword-led statements.
// NOTE: Must stay in sync with the type entries in keywords[] above.
// When you add a new type keyword, add it to BOTH arrays.
static const char *type_keywords[] = {
    "int", "float", "char", "double",
    NULL
};

int is_type_keyword(const char *str)
{
    for (int i = 0; type_keywords[i] != NULL; i++) {
        if (strcmp(str, type_keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Push back a character to the input stream (safe wrapper for ungetc)
static void unread_char(int ch, FILE *input)
{
    if (ch != EOF) {
        ungetc(ch, input);
    }
}

// Skip whitespace, advancing line counter on newlines.
// Returns the first non-whitespace character, or EOF.
static int skip_whitespace(ParserContext *ctx)
{
    int ch = 0;
    while ((ch = fgetc(ctx->input)) != EOF && isspace(ch)) {
        if (ch == '\n') {
            ctx->current_line++;
        }
    }
    return ch;
}

// Handle '/' — may be line comment, block comment, or division operator.
// Returns 1 if a comment was consumed (caller should re-lex), 0 otherwise.
static int skip_comment_or_division(ParserContext *ctx, int lookahead, Token *token)
{
    FILE *input = ctx->input;

    if (lookahead == '/') {
        int ch = 0;
        while ((ch = fgetc(input)) != EOF && ch != '\n');
        if (ch == '\n') {
            ctx->current_line++;
        }
        return 1;
    }

    if (lookahead == '*') {
        int prev = 0;
        int ch = 0;
        while ((ch = fgetc(input)) != EOF) {
            if (ch == '\n') {
                ctx->current_line++;
            }
            if (prev == '*' && ch == '/') {
                break;
            }
            prev = ch;
        }
        return 1;
    }

    // Division operator — put back the lookahead
    unread_char(lookahead, input);
    token->type = TOKEN_OPERATOR;
    token->value[0] = '/';
    token->value[1] = '\0';
    return 0;
}

// Lex an identifier or keyword starting with current_char
static Token lex_identifier_or_keyword(ParserContext *ctx, int first_char)
{
    Token token = {0};
    token.line = ctx->current_line;
    int truncated = 0;
    int value_idx = 0;

    token.value[value_idx++] = (char)first_char;

    int ch = 0;
    while ((ch = fgetc(ctx->input)) != EOF && (isalnum(ch) || ch == '_')) {
        if (value_idx < MAX_TOKEN_VALUE_LEN) {
            token.value[value_idx++] = (char)ch;
        } else {
            truncated = 1;
        }
    }
    token.value[value_idx] = '\0';

    if (truncated) {
        log_error(ERROR_CATEGORY_LEXER, ctx->current_line,
                  "Identifier too long (truncated to %d characters)", MAX_TOKEN_VALUE_LEN);
    }

    unread_char(ch, ctx->input);
    token.type = is_keyword(token.value) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
    return token;
}

// Lex a numeric literal (integer or floating point)
static Token lex_number(ParserContext *ctx, int first_char)
{
    Token token = {0};
    token.line = ctx->current_line;
    int value_idx = 0;

    token.value[value_idx++] = (char)first_char;

    int ch = 0;
    while ((ch = fgetc(ctx->input)) != EOF && (isdigit(ch) || ch == '.')) {
        if (value_idx < MAX_TOKEN_VALUE_LEN) {
            token.value[value_idx++] = (char)ch;
        }
    }
    token.value[value_idx] = '\0';

    unread_char(ch, ctx->input);
    token.type = TOKEN_NUMBER;
    return token;
}

// Try to lex a single-character punctuation token.
// Returns the TokenType, or -1 if not a punctuation character.
static int classify_punctuation(int ch)
{
    switch (ch) {
        case ';': return TOKEN_SEMICOLON;
        case '(': return TOKEN_PARENTHESIS_OPEN;
        case ')': return TOKEN_PARENTHESIS_CLOSE;
        case '{': return TOKEN_BRACE_OPEN;
        case '}': return TOKEN_BRACE_CLOSE;
        case '[': return TOKEN_BRACKET_OPEN;
        case ']': return TOKEN_BRACKET_CLOSE;
        case ',': return TOKEN_COMMA;
        default:  return -1;
    }
}

// Build a two-character operator token, or fall back to single-char
static void build_operator_token(Token *token, int first, int second, FILE *input)
{
    // Each pair: { first_char, second_char } that forms a two-char operator
    static const char pairs[][2] = {
        {'=','='}, {'!','='}, {'<','='}, {'>','='},
        {'<','<'}, {'>','>'}, {'&','&'}, {'|','|'},
        {'+','+'}, {'-','-'},
    };

    for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); ++i) {
        if (first == pairs[i][0] && second == pairs[i][1]) {
            token->value[0] = (char)first;
            token->value[1] = (char)second;
            token->value[2] = '\0';
            return;
        }
    }

    // Single-character operator — put the lookahead back
    unread_char(second, input);
    token->value[0] = (char)first;
    token->value[1] = '\0';
}

// Main lexer: reads the next token from input
Token get_next_token(ParserContext *ctx)
{
    FILE *input = ctx->input;
    Token token = {0};

    int current_char = skip_whitespace(ctx);
    token.line = ctx->current_line;

    if (current_char == EOF) {
        token.type = TOKEN_EOF;
        return token;
    }

    // Comments or division operator
    if (current_char == '/') {
        int lookahead = fgetc(input);
        if (skip_comment_or_division(ctx, lookahead, &token)) {
            return get_next_token(ctx);
        }
        return token;
    }

    if (isalpha(current_char) || current_char == '_') {
        return lex_identifier_or_keyword(ctx, current_char);
    }

    if (isdigit(current_char)) {
        return lex_number(ctx, current_char);
    }

    // Punctuation (single-char delimiters)
    int punct_type = classify_punctuation(current_char);
    if (punct_type >= 0) {
        token.type = (TokenType)punct_type;
        token.value[0] = (char)current_char;
        token.value[1] = '\0';
        return token;
    }

    // Operators (single or multi-character)
    int lookahead = fgetc(input);
    token.type = TOKEN_OPERATOR;
    build_operator_token(&token, current_char, lookahead, input);
    return token;
}

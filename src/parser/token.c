#include "token.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Current token and functions to manage the token stream
Token current_token;
int current_line = 1; // Track current line number

// List of C keywords
const char *keywords[] = {
    "if", "else", "while", "for", "return", "break", "continue",
    "struct",
    "int", "float", "char", "double", "void",
    NULL
};

// Get the next token and update current_token
void advance(FILE *input)
{
    current_token = get_next_token(input);
}


// Check if current token matches expected type
int match(TokenType type)
{
    return current_token.type == type;
}


// Consume the current token if it matches expected type
int consume(FILE *input, TokenType type)
{

    if (match(type)) {
        advance(input);
        return 1;
    }
    return 0;
}

// Check if a string is a keyword
int is_keyword(const char *str)
{

    int keyword_idx = 0;
    for (keyword_idx = 0; keywords[keyword_idx] != NULL; keyword_idx++) {
        if (strcmp(str, keywords[keyword_idx]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Get the next token from input
Token get_next_token(FILE *input)
{
    
    Token token = {0};
    int current_char = 0;        // Current character being read
    int lookahead_char = 0;      // Lookahead character for multi-char tokens
    int value_idx = 0;           // Index for building token value string
    int prev = 0;

    token.line = current_line; // Track line at start of token

    // Skip whitespace
    while ((current_char = fgetc(input)) != EOF && isspace(current_char)) {
        if (current_char == '\n') {
            current_line++; // Increment line on newline
        }
    }
    if (current_char == EOF) {
        token.type = TOKEN_EOF;
        token.value[0] = '\0';
        return token;
    }

    // Handle comments starting with // or /* */
    if (current_char == '/') {
        lookahead_char = fgetc(input);
        if (lookahead_char == '/') {
            // Line comment
            while ((current_char = fgetc(input)) != EOF && current_char != '\n');
            if (current_char == '\n') {
                current_line++; // Increment line on comment newline
            }
            return get_next_token(input);
        } else if (lookahead_char == '*') {
            // Block comment
            prev = 0;
            while ((current_char = fgetc(input)) != EOF) {
                if (current_char == '\n') {
                    current_line++;
                }
                if (prev == '*' && current_char == '/') {
                    break;
                }
                prev = current_char;
            }
            return get_next_token(input);
        } else {
            // It's just the division operator
            if (lookahead_char != EOF) {
                ungetc(lookahead_char, input);
            }
            token.type = TOKEN_OPERATOR;
            token.value[0] = '/';
            token.value[1] = '\0';
            return token;
        }
    }

    // Identifier or keyword
    if (isalpha(current_char) || current_char == '_') {
        value_idx = 0;
        token.value[value_idx++] = current_char;
        while ((current_char = fgetc(input)) != EOF && (isalnum(current_char) || current_char == '_')) {
            if (value_idx < 255) {
                token.value[value_idx++] = current_char;
            }
        }
        token.value[value_idx] = '\0';
        if (current_char != EOF) {
            ungetc(current_char, input);
        }
        token.type = is_keyword(token.value) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
        return token;
    } 
    // Number
    else if (isdigit(current_char)) {
        value_idx = 0;
        token.value[value_idx++] = current_char;
        while ((current_char = fgetc(input)) != EOF && (isdigit(current_char) || current_char == '.')) {
            if (value_idx < 255) {
                token.value[value_idx++] = current_char;
            }
        }
        token.value[value_idx] = '\0';
        if (current_char != EOF) {
            ungetc(current_char, input);
        }
        token.type = TOKEN_NUMBER;
        return token;
    } 
    // Operators and punctuation (including multi-char ops)
    else {
        lookahead_char = fgetc(input);
        token.value[0] = (char)current_char;
        token.value[1] = '\0';
        token.value[2] = '\0';
        // Punctuation
        switch (current_char) {
            case ';':
                token.type = TOKEN_SEMICOLON;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            case '(':
                token.type = TOKEN_PARENTHESIS_OPEN;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            case ')':
                token.type = TOKEN_PARENTHESIS_CLOSE;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            case '{':
                token.type = TOKEN_BRACE_OPEN;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            case '}':
                token.type = TOKEN_BRACE_CLOSE;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            case '[':
                token.type = TOKEN_BRACKET_OPEN;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            case ']':
                token.type = TOKEN_BRACKET_CLOSE;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            case ',':
                token.type = TOKEN_COMMA;
                if (lookahead_char != EOF) {
                    ungetc(lookahead_char, input);
                }
                return token;
            default:
                break;
        }
        // Multi-character operators: ==, !=, <=, >=, <<, >>, &&, ||, ++, --
        token.type = TOKEN_OPERATOR;
        if (current_char == '=' && lookahead_char == '=') {
            token.value[0] = '=';
            token.value[1] = '=';
            token.value[2] = '\0';
        } else if (current_char == '!' && lookahead_char == '=') {
            token.value[0] = '!';
            token.value[1] = '=';
            token.value[2] = '\0';
        } else if (current_char == '<' && lookahead_char == '=') {
            token.value[0] = '<';
            token.value[1] = '=';
            token.value[2] = '\0';
        } else if (current_char == '>' && lookahead_char == '=') {
            token.value[0] = '>';
            token.value[1] = '=';
            token.value[2] = '\0';
        } else if (current_char == '<' && lookahead_char == '<') {
            token.value[0] = '<';
            token.value[1] = '<';
            token.value[2] = '\0';
        } else if (current_char == '>' && lookahead_char == '>') {
            token.value[0] = '>';
            token.value[1] = '>';
            token.value[2] = '\0';
        } else if (current_char == '&' && lookahead_char == '&') {
            token.value[0] = '&';
            token.value[1] = '&';
            token.value[2] = '\0';
        } else if (current_char == '|' && lookahead_char == '|') {
            token.value[0] = '|';
            token.value[1] = '|';
            token.value[2] = '\0';
        } else if (current_char == '+' && lookahead_char == '+') {
            token.value[0] = '+';
            token.value[1] = '+';
            token.value[2] = '\0';
        } else if (current_char == '-' && lookahead_char == '-') {
            token.value[0] = '-';
            token.value[1] = '-';
            token.value[2] = '\0';
        } else {
            // Single-character operator
            if (lookahead_char != EOF) {
                ungetc(lookahead_char, input);
            }
            token.value[0] = (char)current_char;
            token.value[1] = '\0';
        }
        return token;
    }
}

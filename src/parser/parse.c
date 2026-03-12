#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "error_handler.h"
#include "tokenizer.h"
#include "parse_expression.h"
#include "parse_struct.h"
#include "parse_function.h"
#include "parse_statement.h"

// Skip tokens until we find a semicolon or EOF
static void skip_to_semicolon(ParserContext *ctx)
{
    while (!match(ctx, TOKEN_SEMICOLON) && !match(ctx, TOKEN_EOF)) {
        advance(ctx);
    }
    if (match(ctx, TOKEN_SEMICOLON)) {
        advance(ctx);
    }
}

// Skip tokens until we find a closing brace at depth 0, semicolon, or EOF
// Used for error recovery after parse failures
static void skip_to_sync_point(ParserContext *ctx)
{
    int brace_depth = 0;
    
    while (!match(ctx, TOKEN_EOF)) {
        if (match(ctx, TOKEN_BRACE_OPEN)) {
            brace_depth++;
            advance(ctx);
        } else if (match(ctx, TOKEN_BRACE_CLOSE)) {
            if (brace_depth > 0) {
                brace_depth--;
                advance(ctx);
            } else {
                advance(ctx);  // consume the closing brace
                break;
            }
        } else if (match(ctx, TOKEN_SEMICOLON) && brace_depth == 0) {
            advance(ctx);
            break;
        } else {
            advance(ctx);
        }
    }
}

// Parse struct declaration or function returning struct
static ASTNode* parse_struct_declaration(ParserContext *ctx, ASTNode *program_node)
{
    advance(ctx); // consume 'struct'
    
    if (!match(ctx, TOKEN_IDENTIFIER)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                    "'struct' keyword without name - expected identifier");
        skip_to_sync_point(ctx);
        return NULL;
    }
    
    Token struct_name_token = ctx->current_token;
    advance(ctx);
    
    // struct definition: struct Name { ... };
    if (match(ctx, TOKEN_BRACE_OPEN)) {
        ASTNode *struct_node = parse_struct(ctx, struct_name_token);
        if (struct_node) {
            add_child(program_node, struct_node);
        }
        return struct_node;
    }
    
    // Function returning struct: struct Name funcname(...) { ... }
    if (match(ctx, TOKEN_IDENTIFIER)) {
        Token function_name = ctx->current_token;
        advance(ctx);
        
        if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
            ASTNode *function_node = parse_function(ctx, struct_name_token, function_name);
            if (function_node) {
                add_child(program_node, function_node);
            }
            return function_node;
        } else {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                        "Expected '(' after function name '%s' for struct return function",
                        function_name.value);
            skip_to_sync_point(ctx);
        }
    } else {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                    "'struct %s' not followed by function name or '{'",
                    struct_name_token.value);
        skip_to_sync_point(ctx);
    }
    
    return NULL;
}

// Parse function declaration with primitive return type
static ASTNode* parse_function_declaration(ParserContext *ctx, Token return_type, ASTNode *program_node)
{
    if (!match(ctx, TOKEN_IDENTIFIER)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                    "Expected identifier after type '%s'", return_type.value);
        skip_to_sync_point(ctx);
        return NULL;
    }
    
    Token function_name = ctx->current_token;
    advance(ctx);
    
    if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
        ASTNode *function_node = parse_function(ctx, return_type, function_name);
        if (function_node) {
            add_child(program_node, function_node);
        }
        return function_node;
    } else {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                    "Global variable declarations not supported (found '%s %s' without '(')",
                    return_type.value, function_name.value);
        skip_to_semicolon(ctx);
        return NULL;
    }
}

// Parse the entire program: delegates to specialized modules
ASTNode* parse_program(FILE *input)
{
    ParserContext ctx;
    parser_context_init(&ctx, input);

    ASTNode *program_node = create_node(NODE_PROGRAM);

    advance(&ctx); // prime tokenizer

    while (!match(&ctx, TOKEN_EOF) && !has_errors()) {
        #ifdef DEBUG
        printf("Parsing token: type=%d, value='%s'\n", 
               ctx.current_token.type, 
               ctx.current_token.value);
        #endif
        
        if (match(&ctx, TOKEN_KEYWORD)) {
            if (strcmp(ctx.current_token.value, "struct") == 0) {
                parse_struct_declaration(&ctx, program_node);
                continue;
            }
            
            // Primitive or known type function
            Token return_type = ctx.current_token;
            advance(&ctx);
            parse_function_declaration(&ctx, return_type, program_node);
        } else {
            log_error(ERROR_CATEGORY_PARSER, ctx.current_token.line,
                      "Unexpected token '%s' at top level",
                      ctx.current_token.value);
            advance(&ctx);
        }
    }
    
    return program_node;
}
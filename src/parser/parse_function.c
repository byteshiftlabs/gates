#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_function.h"
#include "parse_statement.h"
#include "symbol_arrays.h"
#include "tokenizer.h"
#include "error_handler.h"
#include "utils.h"

// Constants

// Helper: Parse a single function parameter (type and name)
static ASTNode* parse_single_parameter(ParserContext *ctx, Token *parameter_type)
{
    // Handle struct types
    if (strcmp(ctx->current_token.value, "struct") == 0) {
        advance(ctx);
        if (!match(ctx, TOKEN_IDENTIFIER)) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected struct name in parameter list");
            return NULL;
        }
        *parameter_type = ctx->current_token;
        advance(ctx);
    } else {
        *parameter_type = ctx->current_token;
        advance(ctx);
    }
    
    // Get parameter name
    if (!match(ctx, TOKEN_IDENTIFIER)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected parameter name");
        return NULL;
    }
    
    Token parameter_name = ctx->current_token;
    advance(ctx);
    
    // Create parameter node
    ASTNode *parameter_node = create_node(NODE_VAR_DECL);
    parameter_node->token = *parameter_type;
    parameter_node->value = safe_strdup(parameter_name.value);
    
    return parameter_node;
}

// Helper: Parse all function parameters
static void parse_function_parameters(ParserContext *ctx, ASTNode *function_node)
{
    if (!consume(ctx, TOKEN_PARENTHESIS_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '(' after function name");
        return;
    }
    
    while (!match(ctx, TOKEN_PARENTHESIS_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
        if (match(ctx, TOKEN_KEYWORD)) {
            Token parameter_type = (Token){0};
            ASTNode *parameter_node = parse_single_parameter(ctx, &parameter_type);
            if (!parameter_node) {
                break;
            }
            
            add_child(function_node, parameter_node);
            
            // Handle comma between parameters
            if (match(ctx, TOKEN_COMMA)) {
                advance(ctx);
            }
        } else {
            advance(ctx);
        }
    }
    
    if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ')' after parameter list");
        return;
    }
}

// Helper: Parse function body (statements within braces)
// Control flow statements (if/while/for) consume their own braces internally,
// so we only need to watch for the function's closing brace here.
static void parse_function_body(ParserContext *ctx, ASTNode *function_node)
{
    if (!consume(ctx, TOKEN_BRACE_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '{' to start function body");
        return;
    }
    
    while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
        ASTNode *statement_node = parse_statement(ctx);
        if (statement_node) {
            add_child(function_node, statement_node);
        }
    }
    
    if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '}' after function body");
    }
}

ASTNode* parse_function(ParserContext *ctx, Token return_type, Token function_name)
{
    // Initialize function node
    ASTNode *function_node = create_node(NODE_FUNCTION_DECL);
    function_node->token = return_type;
    function_node->value = safe_strdup(function_name.value);
    
    // Reset global array count for this function scope
    reset_array_table();
    
    // Parse function parameters
    parse_function_parameters(ctx, function_node);
    if (has_errors()) {
        free_node(function_node);
        return NULL;
    }
    
    // Parse function body
    parse_function_body(ctx, function_node);
    if (has_errors()) {
        free_node(function_node);
        return NULL;
    }
    
    return function_node;
}

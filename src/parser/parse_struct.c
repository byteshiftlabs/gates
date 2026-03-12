#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_struct.h"
#include "symbol_structs.h"
#include "error_handler.h"
#include "tokenizer.h"
#include "utils.h"


// Parse a single struct field: type name;
static ASTNode* parse_struct_field(ParserContext *ctx, int struct_index)
{
    if (!match(ctx, TOKEN_KEYWORD)) {
        return NULL;
    }
    
    Token field_type = ctx->current_token;
    advance(ctx);
    
    if (!match(ctx, TOKEN_IDENTIFIER)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected field name in struct");
        return NULL;
    }
    
    Token field_name = ctx->current_token;
    advance(ctx);
    
    ASTNode *field_node = create_node(NODE_VAR_DECL);
    if (!field_node) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Failed to allocate field node");
        return NULL;
    }
    field_node->token = field_type;
    field_node->value = safe_strdup(field_name.value);
    
    register_struct_field(struct_index, field_type.value, field_name.value);
    
    if (!consume(ctx, TOKEN_SEMICOLON)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after struct field");
        free_node(field_node);  // Clean up on error
        return NULL;
    }
    
    return field_node;
}

// Parse struct definition: struct Name { type field; ... };
ASTNode* parse_struct(ParserContext *ctx, Token struct_name_token)
{
    if (!consume(ctx, TOKEN_BRACE_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '{' after struct name");
        return NULL;
    }
    
    ASTNode *struct_node = create_node(NODE_STRUCT_DECL);
    struct_node->value = safe_strdup(struct_name_token.value);
    
    int struct_index = register_struct(struct_name_token.value);
    if (struct_index < 0) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Struct table full or invalid name");
        free_node(struct_node);  // Clean up on error
        return NULL;
    }
    
    // Parse all fields
    while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
        ASTNode *field_node = parse_struct_field(ctx, struct_index);
        if (field_node) {
            add_child(struct_node, field_node);
        } else {
            // Skip unknown tokens
            advance(ctx);
        }
    }
    
    if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '}' after struct body");
    }
    if (!consume(ctx, TOKEN_SEMICOLON)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after struct declaration");
    }
    
    return struct_node;
}

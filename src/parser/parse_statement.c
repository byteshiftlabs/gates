#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_statement.h"
#include "parse_expression.h"
#include "parse_control_flow.h"
#include "parse_for.h"
#include "symbol_arrays.h"
#include "utils.h"
#include "error_handler.h"
#include "tokenizer.h"

// Constants
#define ARRAY_SIZE_BUFFER_SIZE 256
#define LHS_BUFFER_SIZE 1024
#define INDEX_BUFFER_SIZE 512
#define BASE_NAME_BUFFER_SIZE 128
#define GENERAL_BUFFER_SIZE 1024
#define INITIAL_PARENTHESIS_DEPTH 0

// Helper: Parse array or struct initializer list
static ASTNode* parse_initializer_list(ParserContext *ctx, int is_array)
{
    advance(ctx);
    ASTNode *init_list = create_node(NODE_EXPRESSION);
    init_list->value = safe_strdup(is_array ? "array_init" : "struct_init");
    
    while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
        if (match(ctx, TOKEN_NUMBER) || match(ctx, TOKEN_IDENTIFIER)) {
            ASTNode *elem = create_node(NODE_EXPRESSION);
            elem->value = safe_strdup(ctx->current_token.value);
            add_child(init_list, elem);
        }
        advance(ctx);
    }
    
    if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '}' after %s initializer",
                  is_array ? "array" : "struct");
        free_node(init_list);
        return NULL;
    }
    
    return init_list;
}

// Helper: Parse variable declaration with optional initialization
static ASTNode* parse_variable_declaration(ParserContext *ctx, Token type_token)
{
    int is_struct = 0;
    int is_array = 0;
    
    // Check if it's a struct type
    if (strcmp(type_token.value, "struct") == 0) {
        if (!match(ctx, TOKEN_IDENTIFIER)) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected struct name after 'struct'");
            return NULL;
        }
        type_token = ctx->current_token;
        advance(ctx);
        is_struct = 1;
    }
    
    // Get variable name
    if (!match(ctx, TOKEN_IDENTIFIER)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected variable name after type");
        return NULL;
    }
    
    Token name_token = ctx->current_token;
    advance(ctx);
    
    ASTNode *var_decl_node = create_node(NODE_VAR_DECL);
    var_decl_node->token = type_token;
    var_decl_node->value = safe_strdup(name_token.value);
    
    // Handle array declaration
    if (match(ctx, TOKEN_BRACKET_OPEN)) {
        is_array = 1;
        advance(ctx);
        
        if (!match(ctx, TOKEN_NUMBER)) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected array size after '['");
            free_node(var_decl_node);
            return NULL;
        }
        
        char arr_size_buf[ARRAY_SIZE_BUFFER_SIZE] = {0};
        char buf[GENERAL_BUFFER_SIZE] = {0};
        snprintf(arr_size_buf, sizeof(arr_size_buf), "%s", ctx->current_token.value);
        snprintf(buf, sizeof(buf), "%s[%s]", name_token.value, ctx->current_token.value);
        free(var_decl_node->value);
        var_decl_node->value = safe_strdup(buf);
        advance(ctx);
        
        {
            int arr_size_val = 0;
            if (!safe_strtoi(arr_size_buf, &arr_size_val)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Invalid array size '%s'", arr_size_buf);
                free_node(var_decl_node);
                return NULL;
            }
            register_array(name_token.value, arr_size_val);
        }
        
        if (!consume(ctx, TOKEN_BRACKET_CLOSE)) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected ']' after array size");
            free_node(var_decl_node);
            return NULL;
        }
    }
    
    // Handle initialization
    if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "=") == 0) {
        advance(ctx);
        
        if (is_array && match(ctx, TOKEN_BRACE_OPEN)) {
            ASTNode *init_list = parse_initializer_list(ctx, 1);
            add_child(var_decl_node, init_list);
        } else if (is_struct && match(ctx, TOKEN_BRACE_OPEN)) {
            ASTNode *init_list = parse_initializer_list(ctx, 0);
            add_child(var_decl_node, init_list);
        } else {
            ASTNode *init_expr = parse_expression(ctx);
            if (init_expr) {
                add_child(var_decl_node, init_expr);
            }
            while (!match(ctx, TOKEN_SEMICOLON) && !match(ctx, TOKEN_EOF)) {
                advance(ctx);
            }
        }
    }
    
    if (!consume(ctx, TOKEN_SEMICOLON)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after variable declaration");
        free_node(var_decl_node);
        return NULL;
    }
    
    return var_decl_node;
}

// Helper: Parse left-hand side expression (identifier with optional field access and array indexing)
static void parse_lhs_expression(ParserContext *ctx, Token lhs_token, char *lhs_buf, size_t lhs_buf_size,
                                  char *idx_buf, size_t idx_buf_size, char *base_name, size_t base_name_size)
{
    snprintf(lhs_buf, lhs_buf_size, "%s", lhs_token.value);
    
    // Handle field access (struct.field)
    while (match(ctx, TOKEN_OPERATOR) && ctx->current_token.value[0] != '\0' && strcmp(ctx->current_token.value, ".") == 0) {
        advance(ctx);
        if (!match(ctx, TOKEN_IDENTIFIER)) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected field name after '.' in assignment");
            return;
        }
        safe_append(lhs_buf, lhs_buf_size, "__");
        safe_append(lhs_buf, lhs_buf_size, ctx->current_token.value);
        advance(ctx);
    }
    
    // Handle array indexing
    if (match(ctx, TOKEN_BRACKET_OPEN)) {
        advance(ctx);
        memset(idx_buf, 0, idx_buf_size);
        int paren_depth = INITIAL_PARENTHESIS_DEPTH;
        
        while (!match(ctx, TOKEN_EOF)) {
            if (match(ctx, TOKEN_BRACKET_CLOSE) && paren_depth == 0) {
                break;
            }
            if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
                safe_append(idx_buf, idx_buf_size, "(");
                advance(ctx);
                paren_depth++;
                continue;
            }
            if (match(ctx, TOKEN_PARENTHESIS_CLOSE)) {
                safe_append(idx_buf, idx_buf_size, ")");
                advance(ctx);
                if (paren_depth > 0) {
                    paren_depth--;
                }
                continue;
            }
            if (ctx->current_token.value[0] != '\0') {
                safe_append(idx_buf, idx_buf_size, ctx->current_token.value);
            }
            advance(ctx);
        }
        
        if (!consume(ctx, TOKEN_BRACKET_CLOSE)) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected ']' after array index in assignment");
            return;
        }
        
        safe_append(lhs_buf, lhs_buf_size, "[");
        safe_append(lhs_buf, lhs_buf_size, idx_buf);
        safe_append(lhs_buf, lhs_buf_size, "]");
        
        // Validate array bounds
        if (is_number_str(idx_buf)) {
            memset(base_name, 0, base_name_size);
            const char *delim = strstr(lhs_buf, "__");
            size_t len = delim ? (size_t)(delim - lhs_buf) : strcspn(lhs_buf, "[");
            if (len >= base_name_size) {
                len = base_name_size - 1;
            }
            safe_copy(base_name, base_name_size, lhs_buf, len);
            int arr_size = find_array_size(base_name);
            if (arr_size > 0) {
                int idx_val = 0;
                if (!safe_strtoi(idx_buf, &idx_val)) {
                    return;
                }
                if (idx_val < 0 || idx_val >= arr_size) {
                    log_error(ERROR_CATEGORY_SEMANTIC, ctx->current_token.line,
                              "Array index %d out of bounds for '%s' with size %d",
                              idx_val, base_name, arr_size);
                    return;
                }
            }
        }
    }
}

// Helper: Parse function call as statement (standalone call that doesn't use return value)
static ASTNode* parse_standalone_function_call(ParserContext *ctx, const char *function_name)
{
    // Use shared helper to parse function call arguments
    ASTNode *func_call_node = parse_function_call_args(ctx, function_name);
    
    // Expect semicolon after statement
    if (!consume(ctx, TOKEN_SEMICOLON))
    {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after function call");
        return NULL;
    }
    
    return func_call_node;
}

static ASTNode* parse_assignment_or_expression(ParserContext *ctx)
{
    Token lhs_token = ctx->current_token;
    char lhs_buf[LHS_BUFFER_SIZE] = {0};
    char idx_buf[INDEX_BUFFER_SIZE] = {0};
    char base_name[BASE_NAME_BUFFER_SIZE] = {0};
    
    advance(ctx);
    
    // Check for function call: identifier(...)
    if (match(ctx, TOKEN_PARENTHESIS_OPEN))
    {
        return parse_standalone_function_call(ctx, lhs_token.value);
    }
    
    parse_lhs_expression(ctx, lhs_token, lhs_buf, sizeof(lhs_buf),
                        idx_buf, sizeof(idx_buf), base_name, sizeof(base_name));
    if (has_errors()) {
        return NULL;
    }
    
    ASTNode *lhs_expr = create_node(NODE_EXPRESSION);
    lhs_expr->value = safe_strdup(lhs_buf);
    
    if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "=") == 0)
    {
        advance(ctx);
        ASTNode *assign_node = create_node(NODE_ASSIGNMENT);
        add_child(assign_node, lhs_expr);
        
        ASTNode *rhs_node = parse_expression(ctx);
        if (rhs_node)
        {
            add_child(assign_node, rhs_node);
        }
        
        if (!consume(ctx, TOKEN_SEMICOLON))
        {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected ';' after assignment");
            return NULL;
        }
        
        return assign_node;
    }
    else
    {
        // Not an assignment, skip to semicolon
        while (!match(ctx, TOKEN_SEMICOLON) && !match(ctx, TOKEN_EOF))
        {
            advance(ctx);
        }
        if (match(ctx, TOKEN_SEMICOLON))
        {
            advance(ctx);
        }
        free_node(lhs_expr);
        return NULL;
    }
}

// Helper: Parse return statement
static ASTNode* parse_return_statement(ParserContext *ctx)
{
    ASTNode *stmt_node = create_node(NODE_STATEMENT);
    stmt_node->token = ctx->current_token;
    advance(ctx);
    
    ASTNode *return_expr = parse_expression(ctx);
    if (return_expr) {
        add_child(stmt_node, return_expr);
    }
    
    if (!consume(ctx, TOKEN_SEMICOLON)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after return statement");
        return NULL;
    }
    
    return stmt_node;
}

ASTNode* parse_statement(ParserContext *ctx)
{
    ASTNode *stmt_node = create_node(NODE_STATEMENT);
    
    // Variable declaration
    if (match(ctx, TOKEN_KEYWORD) && (is_type_keyword(ctx->current_token.value) ||
                                  strcmp(ctx->current_token.value, "struct") == 0)) {
        Token type_token = ctx->current_token;
        advance(ctx);
        ASTNode *sub_statement = parse_variable_declaration(ctx, type_token);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Assignment or expression statement
    if (match(ctx, TOKEN_IDENTIFIER)) {
        ASTNode *sub_statement = parse_assignment_or_expression(ctx);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Return statement
    if (match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "return") == 0) {
        free_node(stmt_node);
        return parse_return_statement(ctx);
    }
    
    // If statement
    if (match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "if") == 0) {
        ASTNode *sub_statement = parse_if_statement(ctx);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // While statement
    if (match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "while") == 0) {
        ASTNode *sub_statement = parse_while_statement(ctx);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // For statement
    if (match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "for") == 0) {
        ASTNode *sub_statement = parse_for_statement(ctx);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Break statement
    if ((match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "break") == 0) ||
        (match(ctx, TOKEN_IDENTIFIER) && strcmp(ctx->current_token.value, "break") == 0)) {
        ASTNode *sub_statement = parse_break_statement(ctx);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Continue statement
    if ((match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "continue") == 0) ||
        (match(ctx, TOKEN_IDENTIFIER) && strcmp(ctx->current_token.value, "continue") == 0)) {
        ASTNode *sub_statement = parse_continue_statement(ctx);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Unknown/empty statement - skip to semicolon
    while (!match(ctx, TOKEN_SEMICOLON) && !match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF)) {
        advance(ctx);
    }
    if (match(ctx, TOKEN_SEMICOLON)) {
        advance(ctx);
    }
    
    return stmt_node;
}

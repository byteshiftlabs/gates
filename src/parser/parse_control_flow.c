#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_control_flow.h"
#include "parse_expression.h"
#include "parse_statement.h"
#include "astnode.h"
#include "error_handler.h"
#include "tokenizer.h"

static int s_loop_depth = 0;

void loop_depth_inc(void) { s_loop_depth++; }
void loop_depth_dec(void) { s_loop_depth--; }

// Helper: Parse else-if or else block
static void parse_else_blocks(ParserContext *ctx, ASTNode *if_node)
{
    while (match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "else") == 0 && !has_errors()) {
        advance(ctx);
        
        if (match(ctx, TOKEN_KEYWORD) && strcmp(ctx->current_token.value, "if") == 0) {
            // else if
            advance(ctx);
            if (!consume(ctx, TOKEN_PARENTHESIS_OPEN)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Expected '(' after 'else if'");
                return;
            }
            
            ASTNode *else_if_cond = parse_expression(ctx);
            
            if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Expected ')' after else if condition");
                return;
            }
            if (!consume(ctx, TOKEN_BRACE_OPEN)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Expected '{' after else if condition");
                return;
            }
            
            ASTNode *elseif_node = create_node(NODE_ELSE_IF_STATEMENT);
            if (else_if_cond) {
                add_child(elseif_node, else_if_cond);
            }
            
            while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
                ASTNode *inner_stmt = parse_statement(ctx);
                if (inner_stmt) {
                    add_child(elseif_node, inner_stmt);
                }
            }
            
            if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Expected '}' after else if block");
                return;
            }
            
            add_child(if_node, elseif_node);
        } else {
            // else
            if (!consume(ctx, TOKEN_BRACE_OPEN)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Expected '{' after else");
                return;
            }
            
            ASTNode *else_node = create_node(NODE_ELSE_STATEMENT);
            
            while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
                ASTNode *inner_stmt = parse_statement(ctx);
                if (inner_stmt) {
                    add_child(else_node, inner_stmt);
                }
            }
            
            if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Expected '}' after else block");
                return;
            }
            
            add_child(if_node, else_node);
            break;
        }
    }
}

// Parse if statement with optional else-if/else chains
ASTNode* parse_if_statement(ParserContext *ctx)
{
    advance(ctx);
    
    if (!consume(ctx, TOKEN_PARENTHESIS_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '(' after 'if'");
        return NULL;
    }
    
    ASTNode *cond_expr = parse_expression(ctx);
    
    if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ')' after if condition");
        if (cond_expr) free_node(cond_expr);
        return NULL;
    }
    if (!consume(ctx, TOKEN_BRACE_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '{' after if condition");
        if (cond_expr) free_node(cond_expr);
        return NULL;
    }
    
    ASTNode *if_node = create_node(NODE_IF_STATEMENT);
    if (cond_expr) {
        add_child(if_node, cond_expr);
    }
    
    while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
        ASTNode *inner_stmt = parse_statement(ctx);
        if (inner_stmt) {
            add_child(if_node, inner_stmt);
        }
    }
    
    if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '}' after if block");
        free_node(if_node);
        return NULL;
    }
    
    parse_else_blocks(ctx, if_node);
    
    return if_node;
}

// Parse while statement with loop depth tracking
ASTNode* parse_while_statement(ParserContext *ctx)
{
    advance(ctx);
    
    if (!consume(ctx, TOKEN_PARENTHESIS_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '(' after 'while'");
        return NULL;
    }
    
    ASTNode *cond_expr = parse_expression(ctx);
    
    if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ')' after while condition");
        if (cond_expr) free_node(cond_expr);
        return NULL;
    }
    if (!consume(ctx, TOKEN_BRACE_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '{' after while condition");
        if (cond_expr) free_node(cond_expr);
        return NULL;
    }
    
    ASTNode *while_node = create_node(NODE_WHILE_STATEMENT);
    if (cond_expr) {
        add_child(while_node, cond_expr);
    }
    
    s_loop_depth++;
    while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
        ASTNode *inner_stmt = parse_statement(ctx);
        if (inner_stmt) {
            add_child(while_node, inner_stmt);
        }
    }
    s_loop_depth--;
    
    if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '}' after while block");
        return NULL;
    }
    
    return while_node;
}

// Parse break statement with loop depth validation
ASTNode* parse_break_statement(ParserContext *ctx)
{
    if (s_loop_depth <= 0) {
        log_error(ERROR_CATEGORY_SEMANTIC, ctx->current_token.line,
                  "'break' not within a loop");
        return NULL;
    }
    
    advance(ctx);
    
    if (!consume(ctx, TOKEN_SEMICOLON)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after 'break'");
        return NULL;
    }
    
    ASTNode *break_node = create_node(NODE_BREAK_STATEMENT);
    return break_node;
}

// Parse continue statement with loop depth validation
ASTNode* parse_continue_statement(ParserContext *ctx)
{
    if (s_loop_depth <= 0) {
        log_error(ERROR_CATEGORY_SEMANTIC, ctx->current_token.line,
                  "'continue' not within a loop");
        return NULL;
    }
    
    advance(ctx);
    
    if (!consume(ctx, TOKEN_SEMICOLON)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after 'continue'");
        return NULL;
    }
    
    ASTNode *continue_node = create_node(NODE_CONTINUE_STATEMENT);
    return continue_node;
}

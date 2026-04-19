// For-loop parser — init, condition, increment, and body.
// Separated from parse_control_flow.c to keep each file focused on
// one family of control structures (if/while vs for).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_for.h"
#include "parse_control_flow.h"
#include "parse_expression.h"
#include "parse_statement.h"
#include "astnode.h"
#include "error_handler.h"
#include "utils.h"
#include "tokenizer.h"

// For-init can be a declaration (int i = 0;) or assignment (i = 0;).
// We try declaration first, then assignment, then empty.
static ASTNode* parse_for_init(ParserContext *ctx)
{
    ASTNode *init_node = NULL;
    
    if (match(ctx, TOKEN_SEMICOLON)) {
        return NULL;
    }
    
    long saved_pos = ftell(ctx->input);
    if (saved_pos == -1L) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line, "Failed to save file position");
        return NULL;
    }
    Token saved_token = ctx->current_token;
    
    if (match(ctx, TOKEN_KEYWORD) && is_type_keyword(ctx->current_token.value)) {
        ASTNode *init_stmt = parse_statement(ctx);
        if (init_stmt && init_stmt->num_children > 0) {
            ASTNode *child0 = init_stmt->children[0];
            if (child0->type == NODE_VAR_DECL || child0->type == NODE_ASSIGNMENT) {
                init_node = child0;
            }
            // Detach child from wrapper and free the wrapper to avoid leak
            init_stmt->children[0] = NULL;
            init_stmt->num_children = 0;
            free_node(init_stmt);
        }
    } else if (match(ctx, TOKEN_IDENTIFIER)) {
        Token temp_lhs = ctx->current_token;
        advance(ctx);
        if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "=") == 0) {
            advance(ctx);
            ASTNode *assign_tmp = create_node(NODE_ASSIGNMENT);
            ASTNode *lhs_expr_tmp = create_node(NODE_EXPRESSION);
            lhs_expr_tmp->value = safe_strdup(temp_lhs.value);
            add_child(assign_tmp, lhs_expr_tmp);
            
            ASTNode *rhs_expr = parse_expression(ctx);
            if (rhs_expr) {
                add_child(assign_tmp, rhs_expr);
            }
            
            if (!consume(ctx, TOKEN_SEMICOLON)) {
                log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                          "Expected ';' after for-init assignment");
                free_node(assign_tmp);
                return NULL;
            }
            init_node = assign_tmp;
        } else {
            fseek(ctx->input, saved_pos, SEEK_SET);
            ctx->current_token = saved_token;
        }
    }
    
    return init_node;
}

// C's i++/i-- are syntactic sugar for i = i + 1 / i = i - 1.
// We desugar here so the AST and codegen only handle assignments.
static ASTNode* parse_for_increment(ParserContext *ctx)
{
    ASTNode *incr_expr = NULL;
    
    if (match(ctx, TOKEN_PARENTHESIS_CLOSE)) {
        return NULL;
    }
    
    if (match(ctx, TOKEN_IDENTIFIER)) {
        Token inc_lhs = ctx->current_token;
        advance(ctx);
        
        if (match(ctx, TOKEN_OPERATOR) && (strcmp(ctx->current_token.value, "++") == 0 ||
                                       strcmp(ctx->current_token.value, "--") == 0)) {
            incr_expr = create_node(NODE_ASSIGNMENT);
            ASTNode *lhs = create_node(NODE_EXPRESSION);
            lhs->value = safe_strdup(inc_lhs.value);
            add_child(incr_expr, lhs);
            
            ASTNode *rhs = create_node(NODE_BINARY_EXPR);
            rhs->value = safe_strdup(strcmp(ctx->current_token.value, "++") == 0 ? "+" : "-");
            ASTNode *op_l = create_node(NODE_EXPRESSION);
            op_l->value = safe_strdup(inc_lhs.value);
            ASTNode *op_r = create_node(NODE_EXPRESSION);
            op_r->value = safe_strdup("1");
            add_child(rhs, op_l);
            add_child(rhs, op_r);
            add_child(incr_expr, rhs);
            advance(ctx);
        } else if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "=") == 0) {
            advance(ctx);
            incr_expr = create_node(NODE_ASSIGNMENT);
            ASTNode *lhs = create_node(NODE_EXPRESSION);
            lhs->value = safe_strdup(inc_lhs.value);
            add_child(incr_expr, lhs);
            
            ASTNode *rhs = parse_expression(ctx);
            if (rhs) {
                add_child(incr_expr, rhs);
            }
        }
    }
    
    return incr_expr;
}

// Parse for(init; cond; incr) { body }.
// Loop depth tracking enables break/continue validation in parse_control_flow.
ASTNode* parse_for_statement(ParserContext *ctx)
{
    advance(ctx);
    
    if (!consume(ctx, TOKEN_PARENTHESIS_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '(' after 'for'");
        return NULL;
    }
    
    ASTNode *init_node = parse_for_init(ctx);
    
    if (match(ctx, TOKEN_SEMICOLON)) {
        advance(ctx);
    }
    
    ASTNode *cond_expr = NULL;
    if (!match(ctx, TOKEN_SEMICOLON)) {
        cond_expr = parse_expression(ctx);
    }
    
    if (!consume(ctx, TOKEN_SEMICOLON)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ';' after for condition");
        if (init_node) free_node(init_node);
        if (cond_expr) free_node(cond_expr);
        return NULL;
    }
    
    ASTNode *incr_expr = parse_for_increment(ctx);
    
    if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ')' after for header");
        if (init_node) free_node(init_node);
        if (cond_expr) free_node(cond_expr);
        if (incr_expr) free_node(incr_expr);
        return NULL;
    }
    if (!consume(ctx, TOKEN_BRACE_OPEN)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '{' after for header");
        if (init_node) free_node(init_node);
        if (cond_expr) free_node(cond_expr);
        if (incr_expr) free_node(incr_expr);
        return NULL;
    }
    
    ASTNode *for_node = create_node(NODE_FOR_STATEMENT);
    
    if (init_node) {
        add_child(for_node, init_node);
    }
    
    if (cond_expr) {
        add_child(for_node, cond_expr);
    } else {
        ASTNode *true_expr = create_node(NODE_EXPRESSION);
        true_expr->value = safe_strdup("1");
        add_child(for_node, true_expr);
    }
    
    loop_depth_inc();
    while (!match(ctx, TOKEN_BRACE_CLOSE) && !match(ctx, TOKEN_EOF) && !has_errors()) {
        ASTNode *inner = parse_statement(ctx);
        if (inner) {
            add_child(for_node, inner);
        }
    }
    loop_depth_dec();
    
    if (!consume(ctx, TOKEN_BRACE_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected '}' after for body");
        if (incr_expr) free_node(incr_expr);
        free_node(for_node);
        return NULL;
    }
    
    if (incr_expr) {
        add_child(for_node, incr_expr);
    }
    
    return for_node;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_control_flow.h"
#include "parse_expression.h"
#include "parse_statement.h"
#include "symbol_arrays.h"
#include "astnode.h"
#include "utils.h"
#include "token.h"

extern Token current_token;

static int s_loop_depth = 0;

// Forward declarations for internal helpers
static void parse_else_blocks(FILE *input, ASTNode *if_node);
static ASTNode* parse_for_init(FILE *input);
static ASTNode* parse_for_increment(FILE *input);

// Helper: Parse else-if or else block
static void parse_else_blocks(FILE *input, ASTNode *if_node)
{
    ASTNode *elseif_cond = NULL;
    ASTNode *elseif_node = NULL;
    ASTNode *else_node = NULL;
    ASTNode *inner_stmt = NULL;
    
    while (match(TOKEN_KEYWORD) && strcmp(current_token.value, "else") == 0) {
        advance(input);
        
        if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "if") == 0) {
            // else if
            advance(input);
            if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
                printf("Error (line %d): Expected '(' after 'else if'\n", current_token.line);
                exit(EXIT_FAILURE);
            }
            
            elseif_cond = parse_expression(input);
            
            if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
                printf("Error (line %d): Expected ')' after else if condition\n", current_token.line);
                exit(EXIT_FAILURE);
            }
            if (!consume(input, TOKEN_BRACE_OPEN)) {
                printf("Error (line %d): Expected '{' after else if condition\n", current_token.line);
                exit(EXIT_FAILURE);
            }
            
            elseif_node = create_node(NODE_ELSE_IF_STATEMENT);
            if (elseif_cond) {
                add_child(elseif_node, elseif_cond);
            }
            
            while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
                inner_stmt = parse_statement(input);
                if (inner_stmt) {
                    add_child(elseif_node, inner_stmt);
                }
            }
            
            if (!consume(input, TOKEN_BRACE_CLOSE)) {
                printf("Error (line %d): Expected '}' after else if block\n", current_token.line);
                exit(EXIT_FAILURE);
            }
            
            add_child(if_node, elseif_node);
        } else {
            // else
            if (!consume(input, TOKEN_BRACE_OPEN)) {
                printf("Error (line %d): Expected '{' after else\n", current_token.line);
                exit(EXIT_FAILURE);
            }
            
            else_node = create_node(NODE_ELSE_STATEMENT);
            
            while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
                inner_stmt = parse_statement(input);
                if (inner_stmt) {
                    add_child(else_node, inner_stmt);
                }
            }
            
            if (!consume(input, TOKEN_BRACE_CLOSE)) {
                printf("Error (line %d): Expected '}' after else block\n", current_token.line);
                exit(EXIT_FAILURE);
            }
            
            add_child(if_node, else_node);
            break;
        }
    }
}

// Parse if statement with optional else-if/else chains
ASTNode* parse_if_statement(FILE *input)
{
    ASTNode *cond_expr = NULL;
    ASTNode *if_node = NULL;
    ASTNode *inner_stmt = NULL;
    
    advance(input);
    
    if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
        printf("Error (line %d): Expected '(' after 'if'\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    cond_expr = parse_expression(input);
    
    if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
        printf("Error (line %d): Expected ')' after if condition\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    if (!consume(input, TOKEN_BRACE_OPEN)) {
        printf("Error (line %d): Expected '{' after if condition\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    if_node = create_node(NODE_IF_STATEMENT);
    if (cond_expr) {
        add_child(if_node, cond_expr);
    }
    
    while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
        inner_stmt = parse_statement(input);
        if (inner_stmt) {
            add_child(if_node, inner_stmt);
        }
    }
    
    if (!consume(input, TOKEN_BRACE_CLOSE)) {
        printf("Error (line %d): Expected '}' after if block\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    parse_else_blocks(input, if_node);
    
    return if_node;
}

// Parse while statement with loop depth tracking
ASTNode* parse_while_statement(FILE *input)
{
    ASTNode *cond_expr = NULL;
    ASTNode *while_node = NULL;
    ASTNode *inner_stmt = NULL;
    
    advance(input);
    
    if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
        printf("Error (line %d): Expected '(' after 'while'\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    cond_expr = parse_expression(input);
    
    if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
        printf("Error (line %d): Expected ')' after while condition\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    if (!consume(input, TOKEN_BRACE_OPEN)) {
        printf("Error (line %d): Expected '{' after while condition\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    while_node = create_node(NODE_WHILE_STATEMENT);
    if (cond_expr) {
        add_child(while_node, cond_expr);
    }
    
    s_loop_depth++;
    while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
        inner_stmt = parse_statement(input);
        if (inner_stmt) {
            add_child(while_node, inner_stmt);
        }
    }
    s_loop_depth--;
    
    if (!consume(input, TOKEN_BRACE_CLOSE)) {
        printf("Error (line %d): Expected '}' after while block\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    return while_node;
}

// Parse for-loop initialization (declaration or assignment)
static ASTNode* parse_for_init(FILE *input)
{
    ASTNode *init_node = NULL;
    ASTNode *init_stmt = NULL;
    ASTNode *child0 = NULL;
    ASTNode *assign_tmp = NULL;
    ASTNode *lhs_expr_tmp = NULL;
    ASTNode *rhs_expr = NULL;
    Token temp_lhs = {0};
    long saved_pos = 0;
    Token saved_token = {0};
    
    if (match(TOKEN_SEMICOLON)) {
        return NULL;
    }
    
    saved_pos = ftell(input);
    saved_token = current_token;
    
    if (match(TOKEN_KEYWORD) && (strcmp(current_token.value, "int") == 0 ||
                                  strcmp(current_token.value, "float") == 0 ||
                                  strcmp(current_token.value, "char") == 0 ||
                                  strcmp(current_token.value, "double") == 0)) {
        init_stmt = parse_statement(input);
        if (init_stmt && init_stmt->num_children > 0) {
            child0 = init_stmt->children[0];
            if (child0->type == NODE_VAR_DECL || child0->type == NODE_ASSIGNMENT) {
                init_node = child0;
            }
            // Detach child from wrapper and free the wrapper to avoid leak
            init_stmt->children[0] = NULL;
            init_stmt->num_children = 0;
            free_node(init_stmt);
        }
    } else if (match(TOKEN_IDENTIFIER)) {
        temp_lhs = current_token;
        advance(input);
        if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
            advance(input);
            assign_tmp = create_node(NODE_ASSIGNMENT);
            lhs_expr_tmp = create_node(NODE_EXPRESSION);
            lhs_expr_tmp->value = strdup(temp_lhs.value);
            add_child(assign_tmp, lhs_expr_tmp);
            
            rhs_expr = parse_expression(input);
            if (rhs_expr) {
                add_child(assign_tmp, rhs_expr);
            }
            
            if (!consume(input, TOKEN_SEMICOLON)) {
                printf("Error (line %d): Expected ';' after for-init assignment\n", current_token.line);
                exit(EXIT_FAILURE);
            }
            init_node = assign_tmp;
        } else {
            fseek(input, saved_pos, SEEK_SET);
            current_token = saved_token;
        }
    }
    
    return init_node;
}

// Parse for-loop increment (i++, i--, or i = expr)
static ASTNode* parse_for_increment(FILE *input)
{
    ASTNode *incr_expr = NULL;
    ASTNode *lhs = NULL;
    ASTNode *rhs = NULL;
    ASTNode *op_l = NULL;
    ASTNode *op_r = NULL;
    Token inc_lhs = {0};
    
    if (match(TOKEN_PARENTHESIS_CLOSE)) {
        return NULL;
    }
    
    if (match(TOKEN_IDENTIFIER)) {
        inc_lhs = current_token;
        advance(input);
        
        if (match(TOKEN_OPERATOR) && (strcmp(current_token.value, "++") == 0 ||
                                       strcmp(current_token.value, "--") == 0)) {
            incr_expr = create_node(NODE_ASSIGNMENT);
            lhs = create_node(NODE_EXPRESSION);
            lhs->value = strdup(inc_lhs.value);
            add_child(incr_expr, lhs);
            
            rhs = create_node(NODE_BINARY_EXPR);
            rhs->value = strdup(strcmp(current_token.value, "++") == 0 ? "+" : "-");
            op_l = create_node(NODE_EXPRESSION);
            op_l->value = strdup(inc_lhs.value);
            op_r = create_node(NODE_EXPRESSION);
            op_r->value = strdup("1");
            add_child(rhs, op_l);
            add_child(rhs, op_r);
            add_child(incr_expr, rhs);
            advance(input);
        } else if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
            advance(input);
            incr_expr = create_node(NODE_ASSIGNMENT);
            lhs = create_node(NODE_EXPRESSION);
            lhs->value = strdup(inc_lhs.value);
            add_child(incr_expr, lhs);
            
            rhs = parse_expression(input);
            if (rhs) {
                add_child(incr_expr, rhs);
            }
        }
    }
    
    return incr_expr;
}

// Parse for statement with init, condition, increment, and body
ASTNode* parse_for_statement(FILE *input)
{
    ASTNode *init_node = NULL;
    ASTNode *cond_expr = NULL;
    ASTNode *incr_expr = NULL;
    ASTNode *for_node = NULL;
    ASTNode *true_expr = NULL;
    ASTNode *inner = NULL;
    
    advance(input);
    
    if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
        printf("Error (line %d): Expected '(' after 'for'\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    // Parse initialization
    init_node = parse_for_init(input);
    
    if (match(TOKEN_SEMICOLON)) {
        advance(input);
    }
    
    // Parse condition
    if (!match(TOKEN_SEMICOLON)) {
        cond_expr = parse_expression(input);
    }
    
    if (!consume(input, TOKEN_SEMICOLON)) {
        printf("Error (line %d): Expected ';' after for condition\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    // Parse increment
    incr_expr = parse_for_increment(input);
    
    if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
        printf("Error (line %d): Expected ')' after for header\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    if (!consume(input, TOKEN_BRACE_OPEN)) {
        printf("Error (line %d): Expected '{' after for header\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    // Build for node
    for_node = create_node(NODE_FOR_STATEMENT);
    
    if (init_node) {
        add_child(for_node, init_node);
    }
    
    if (cond_expr) {
        add_child(for_node, cond_expr);
    } else {
        true_expr = create_node(NODE_EXPRESSION);
        true_expr->value = strdup("1");
        add_child(for_node, true_expr);
    }
    
    // Parse body
    s_loop_depth++;
    while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
        inner = parse_statement(input);
        if (inner) {
            add_child(for_node, inner);
        }
    }
    s_loop_depth--;
    
    if (!consume(input, TOKEN_BRACE_CLOSE)) {
        printf("Error (line %d): Expected '}' after for body\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    if (incr_expr) {
        add_child(for_node, incr_expr);
    }
    
    return for_node;
}

// Parse break statement with loop depth validation
ASTNode* parse_break_statement(FILE *input)
{
    ASTNode *break_node = NULL;
    
    if (s_loop_depth <= 0) {
        printf("Error (line %d): 'break' not within a loop\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    advance(input);
    
    if (!consume(input, TOKEN_SEMICOLON)) {
        printf("Error (line %d): Expected ';' after 'break'\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    break_node = create_node(NODE_BREAK_STATEMENT);
    return break_node;
}

// Parse continue statement with loop depth validation
ASTNode* parse_continue_statement(FILE *input)
{
    ASTNode *continue_node = NULL;
    
    if (s_loop_depth <= 0) {
        printf("Error (line %d): 'continue' not within a loop\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    advance(input);
    
    if (!consume(input, TOKEN_SEMICOLON)) {
        printf("Error (line %d): Expected ';' after 'continue'\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    continue_node = create_node(NODE_CONTINUE_STATEMENT);
    return continue_node;
}

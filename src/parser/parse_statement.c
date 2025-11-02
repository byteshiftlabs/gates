#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parse_statement.h"
#include "parse_expression.h"
#include "symbol_structs.h"
#include "symbol_arrays.h"
#include "utils.h"
#include "parse.h" // create_node/add_child
#include "token.h"

// Constants
#define MAX_ARRAYS 128
#define ARRAY_SIZE_BUFFER_SIZE 256
#define LHS_BUFFER_SIZE 1024
#define INDEX_BUFFER_SIZE 512
#define BASE_NAME_BUFFER_SIZE 128
#define GENERAL_BUFFER_SIZE 1024
#define INITIAL_PARENTHESIS_DEPTH 0

static inline void safe_append(char *dst, size_t dst_size, const char *src)
{
    size_t used = strlen(dst);
    if (used >= dst_size - 1) {
        return;
    }
    size_t available_space = dst_size - 1 - used;
    size_t copy_length = strlen(src);
    if (copy_length > available_space) {
        copy_length = available_space;
    }
    memcpy(dst + used, src, copy_length);
    dst[used + copy_length] = '\0';
}

static inline void safe_copy(char *dst, size_t dst_size, const char *src, size_t limit)
{
    if (!dst_size) {
        return;
    }
    size_t source_length = strlen(src);
    if (source_length > limit) {
        source_length = limit;
    }
    if (source_length >= dst_size) {
        source_length = dst_size - 1;
    }
    memcpy(dst, src, source_length);
    dst[source_length] = '\0';
}

extern Token current_token;
extern int g_array_count;
extern ArrayInfo g_arrays[MAX_ARRAYS];

static int s_loop_depth = 0;

// Forward declarations
static ASTNode* parse_variable_declaration(FILE *input, Token type_token);
static ASTNode* parse_assignment_or_expression(FILE *input);
static ASTNode* parse_return_statement(FILE *input);
static ASTNode* parse_if_statement(FILE *input);
static ASTNode* parse_while_statement(FILE *input);
static ASTNode* parse_for_statement(FILE *input);
static ASTNode* parse_break_statement(FILE *input);
static ASTNode* parse_continue_statement(FILE *input);

// Helper: Parse array or struct initializer list
static ASTNode* parse_initializer_list(FILE *input, int is_array)
{
    ASTNode *init_list = NULL;
    ASTNode *elem = NULL;
    
    advance(input);
    init_list = create_node(NODE_EXPRESSION);
    init_list->value = strdup(is_array ? "array_init" : "struct_init");
    
    while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
        if (match(TOKEN_NUMBER) || match(TOKEN_IDENTIFIER)) {
            elem = create_node(NODE_EXPRESSION);
            elem->value = strdup(current_token.value);
            add_child(init_list, elem);
            advance(input);
        } else if (match(TOKEN_COMMA)) {
            advance(input);
        } else {
            advance(input);
        }
    }
    
    if (!consume(input, TOKEN_BRACE_CLOSE)) {
        printf("Error (line %d): Expected '}' after %s initializer\n", 
               current_token.line, is_array ? "array" : "struct");
        exit(EXIT_FAILURE);
    }
    
    return init_list;
}

// Helper: Parse variable declaration with optional initialization
static ASTNode* parse_variable_declaration(FILE *input, Token type_token)
{
    Token name_token = {0};
    ASTNode *var_decl_node = NULL;
    ASTNode *init_expr = NULL;
    ASTNode *init_list = NULL;
    int is_struct = 0;
    int is_array = 0;
    char arr_size_buf[ARRAY_SIZE_BUFFER_SIZE] = {0};
    char buf[GENERAL_BUFFER_SIZE] = {0};
    
    // Check if it's a struct type
    if (strcmp(type_token.value, "struct") == 0) {
        if (!match(TOKEN_IDENTIFIER)) {
            printf("Error (line %d): Expected struct name after 'struct'\n", current_token.line);
            exit(EXIT_FAILURE);
        }
        type_token = current_token;
        advance(input);
        is_struct = 1;
    }
    
    // Get variable name
    if (!match(TOKEN_IDENTIFIER)) {
        printf("Error (line %d): Expected variable name after type\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    name_token = current_token;
    advance(input);
    
    var_decl_node = create_node(NODE_VAR_DECL);
    var_decl_node->token = type_token;
    var_decl_node->value = strdup(name_token.value);
    
    // Handle array declaration
    if (match(TOKEN_BRACKET_OPEN)) {
        is_array = 1;
        advance(input);
        
        if (!match(TOKEN_NUMBER)) {
            printf("Error (line %d): Expected array size after '['\n", current_token.line);
            exit(EXIT_FAILURE);
        }
        
        snprintf(arr_size_buf, sizeof(arr_size_buf), "%s", current_token.value);
        snprintf(buf, sizeof(buf), "%s[%s]", name_token.value, current_token.value);
        free(var_decl_node->value);
        var_decl_node->value = strdup(buf);
        advance(input);
        
        register_array(name_token.value, atoi(arr_size_buf));
        
        if (!consume(input, TOKEN_BRACKET_CLOSE)) {
            printf("Error (line %d): Expected ']' after array size\n", current_token.line);
            exit(EXIT_FAILURE);
        }
    }
    
    // Handle initialization
    if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
        advance(input);
        
        if (is_array && match(TOKEN_BRACE_OPEN)) {
            init_list = parse_initializer_list(input, 1);
            add_child(var_decl_node, init_list);
        } else if (is_struct && match(TOKEN_BRACE_OPEN)) {
            init_list = parse_initializer_list(input, 0);
            add_child(var_decl_node, init_list);
        } else {
            init_expr = parse_expression(input);
            if (init_expr) {
                add_child(var_decl_node, init_expr);
            }
            while (!match(TOKEN_SEMICOLON) && !match(TOKEN_EOF)) {
                advance(input);
            }
        }
    }
    
    if (!consume(input, TOKEN_SEMICOLON)) {
        printf("Error (line %d): Expected ';' after variable declaration\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    return var_decl_node;
}

// Helper: Parse left-hand side expression (identifier with optional field access and array indexing)
static void parse_lhs_expression(FILE *input, Token lhs_token, char *lhs_buf, size_t lhs_buf_size,
                                  char *idx_buf, size_t idx_buf_size, char *base_name, size_t base_name_size)
{
    int paren_depth = INITIAL_PARENTHESIS_DEPTH;
    int arr_size = 0;
    int idx_val = 0;
    size_t len = 0;
    const char *delim = NULL;
    
    snprintf(lhs_buf, lhs_buf_size, "%s", lhs_token.value);
    
    // Handle field access (struct.field)
    while (match(TOKEN_OPERATOR) && current_token.value && strcmp(current_token.value, ".") == 0) {
        advance(input);
        if (!match(TOKEN_IDENTIFIER)) {
            printf("Error (line %d): Expected field name after '.' in assignment\n", current_token.line);
            exit(EXIT_FAILURE);
        }
        safe_append(lhs_buf, lhs_buf_size, "__");
        safe_append(lhs_buf, lhs_buf_size, current_token.value);
        advance(input);
    }
    
    // Handle array indexing
    if (match(TOKEN_BRACKET_OPEN)) {
        advance(input);
        memset(idx_buf, 0, idx_buf_size);
        
        while (!match(TOKEN_EOF)) {
            if (match(TOKEN_BRACKET_CLOSE) && paren_depth == 0) {
                break;
            }
            if (match(TOKEN_PARENTHESIS_OPEN)) {
                safe_append(idx_buf, idx_buf_size, "(");
                advance(input);
                paren_depth++;
                continue;
            }
            if (match(TOKEN_PARENTHESIS_CLOSE)) {
                safe_append(idx_buf, idx_buf_size, ")");
                advance(input);
                if (paren_depth > 0) {
                    paren_depth--;
                }
                continue;
            }
            if (current_token.value) {
                safe_append(idx_buf, idx_buf_size, current_token.value);
            }
            advance(input);
        }
        
        if (!consume(input, TOKEN_BRACKET_CLOSE)) {
            printf("Error (line %d): Expected ']' after array index in assignment\n", current_token.line);
            exit(EXIT_FAILURE);
        }
        
        safe_append(lhs_buf, lhs_buf_size, "[");
        safe_append(lhs_buf, lhs_buf_size, idx_buf);
        safe_append(lhs_buf, lhs_buf_size, "]");
        
        // Validate array bounds
        if (is_number_str(idx_buf)) {
            memset(base_name, 0, base_name_size);
            delim = strstr(lhs_buf, "__");
            len = delim ? (size_t)(delim - lhs_buf) : strcspn(lhs_buf, "[");
            if (len >= base_name_size) {
                len = base_name_size - 1;
            }
            safe_copy(base_name, base_name_size, lhs_buf, len);
            arr_size = find_array_size(base_name);
            if (arr_size > 0) {
                idx_val = atoi(idx_buf);
                if (idx_val < 0 || idx_val >= arr_size) {
                    printf("Error (line %d): Array index %d out of bounds for '%s' with size %d\n",
                           current_token.line, idx_val, base_name, arr_size);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

// Helper: Parse assignment statement or expression statement
static ASTNode* parse_assignment_or_expression(FILE *input)
{
    Token lhs_token = current_token;
    ASTNode *assign_node = NULL;
    ASTNode *lhs_expr = NULL;
    ASTNode *rhs_node = NULL;
    char lhs_buf[LHS_BUFFER_SIZE] = {0};
    char idx_buf[INDEX_BUFFER_SIZE] = {0};
    char base_name[BASE_NAME_BUFFER_SIZE] = {0};
    
    advance(input);
    
    parse_lhs_expression(input, lhs_token, lhs_buf, sizeof(lhs_buf),
                        idx_buf, sizeof(idx_buf), base_name, sizeof(base_name));
    
    lhs_expr = create_node(NODE_EXPRESSION);
    lhs_expr->value = strdup(lhs_buf);
    
    if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0) {
        advance(input);
        assign_node = create_node(NODE_ASSIGNMENT);
        add_child(assign_node, lhs_expr);
        
        rhs_node = parse_expression(input);
        if (rhs_node) {
            add_child(assign_node, rhs_node);
        }
        
        if (!consume(input, TOKEN_SEMICOLON)) {
            printf("Error (line %d): Expected ';' after assignment\n", current_token.line);
            exit(EXIT_FAILURE);
        }
        
        return assign_node;
    } else {
        // Not an assignment, skip to semicolon
        while (!match(TOKEN_SEMICOLON) && !match(TOKEN_EOF)) {
            advance(input);
        }
        if (match(TOKEN_SEMICOLON)) {
            advance(input);
        }
        free_node(lhs_expr);
        return NULL;
    }
}

// Helper: Parse return statement
static ASTNode* parse_return_statement(FILE *input)
{
    ASTNode *stmt_node = create_node(NODE_STATEMENT);
    ASTNode *return_expr = NULL;
    
    stmt_node->token = current_token;
    advance(input);
    
    return_expr = parse_expression(input);
    if (return_expr) {
        add_child(stmt_node, return_expr);
    }
    
    if (!consume(input, TOKEN_SEMICOLON)) {
        printf("Error (line %d): Expected ';' after return statement\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    return stmt_node;
}

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

// Helper: Parse if statement
static ASTNode* parse_if_statement(FILE *input)
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

// Helper: Parse while statement
static ASTNode* parse_while_statement(FILE *input)
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

// Helper: Parse for-loop initialization
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

// Helper: Parse for-loop increment
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

// Helper: Parse for statement
static ASTNode* parse_for_statement(FILE *input)
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

// Helper: Parse break statement
static ASTNode* parse_break_statement(FILE *input)
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

// Helper: Parse continue statement
static ASTNode* parse_continue_statement(FILE *input)
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

ASTNode* parse_statement(FILE *input)
{
    ASTNode *stmt_node = NULL;
    ASTNode *sub_statement = NULL;
    
    stmt_node = create_node(NODE_STATEMENT);
    
    // Variable declaration
    if (match(TOKEN_KEYWORD) && (strcmp(current_token.value, "int") == 0 ||
                                  strcmp(current_token.value, "float") == 0 ||
                                  strcmp(current_token.value, "char") == 0 ||
                                  strcmp(current_token.value, "double") == 0 ||
                                  strcmp(current_token.value, "struct") == 0)) {
        Token type_token = current_token;
        advance(input);
        sub_statement = parse_variable_declaration(input, type_token);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Assignment or expression statement
    if (match(TOKEN_IDENTIFIER)) {
        sub_statement = parse_assignment_or_expression(input);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Return statement
    if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "return") == 0) {
        return parse_return_statement(input);
    }
    
    // If statement
    if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "if") == 0) {
        sub_statement = parse_if_statement(input);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // While statement
    if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "while") == 0) {
        sub_statement = parse_while_statement(input);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // For statement
    if (match(TOKEN_KEYWORD) && strcmp(current_token.value, "for") == 0) {
        sub_statement = parse_for_statement(input);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Break statement
    if ((match(TOKEN_KEYWORD) && strcmp(current_token.value, "break") == 0) ||
        (match(TOKEN_IDENTIFIER) && strcmp(current_token.value, "break") == 0)) {
        sub_statement = parse_break_statement(input);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Continue statement
    if ((match(TOKEN_KEYWORD) && strcmp(current_token.value, "continue") == 0) ||
        (match(TOKEN_IDENTIFIER) && strcmp(current_token.value, "continue") == 0)) {
        sub_statement = parse_continue_statement(input);
        if (sub_statement) {
            add_child(stmt_node, sub_statement);
        }
        return stmt_node;
    }
    
    // Unknown/empty statement - skip to semicolon
    while (!match(TOKEN_SEMICOLON) && !match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
        advance(input);
    }
    if (match(TOKEN_SEMICOLON)) {
        advance(input);
    }
    
    return stmt_node;
}

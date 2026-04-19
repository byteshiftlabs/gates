#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parse_statement.h"
#include "parse_expression.h"
#include "parse_control_flow.h"
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

extern Token current_token;
extern int g_array_count;
extern ArrayInfo g_arrays[MAX_ARRAYS];

// Forward declarations
static ASTNode* parse_variable_declaration(FILE *input, Token type_token);
static ASTNode* parse_assignment_or_expression(FILE *input);
static ASTNode* parse_return_statement(FILE *input);

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
    while (match(TOKEN_OPERATOR) && current_token.value[0] != '\0' && strcmp(current_token.value, ".") == 0) {
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
            if (current_token.value[0] != '\0') {
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
// Helper: Parse function call as statement (standalone call that doesn't use return value)
static ASTNode* parse_standalone_function_call(FILE *input, const char *function_name)
{
    ASTNode *func_call_node = NULL;
    
    // Use shared helper to parse function call arguments
    func_call_node = parse_function_call_args(input, function_name);
    
    // Expect semicolon after statement
    if (!consume(input, TOKEN_SEMICOLON))
    {
        printf("Error (line %d): Expected ';' after function call\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    return func_call_node;
}

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
    
    // Check for function call: identifier(...)
    if (match(TOKEN_PARENTHESIS_OPEN))
    {
        return parse_standalone_function_call(input, lhs_token.value);
    }
    
    parse_lhs_expression(input, lhs_token, lhs_buf, sizeof(lhs_buf),
                        idx_buf, sizeof(idx_buf), base_name, sizeof(base_name));
    
    lhs_expr = create_node(NODE_EXPRESSION);
    lhs_expr->value = strdup(lhs_buf);
    
    if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "=") == 0)
    {
        advance(input);
        assign_node = create_node(NODE_ASSIGNMENT);
        add_child(assign_node, lhs_expr);
        
        rhs_node = parse_expression(input);
        if (rhs_node)
        {
            add_child(assign_node, rhs_node);
        }
        
        if (!consume(input, TOKEN_SEMICOLON))
        {
            printf("Error (line %d): Expected ';' after assignment\n", current_token.line);
            exit(EXIT_FAILURE);
        }
        
        return assign_node;
    }
    else
    {
        // Not an assignment, skip to semicolon
        while (!match(TOKEN_SEMICOLON) && !match(TOKEN_EOF))
        {
            advance(input);
        }
        if (match(TOKEN_SEMICOLON))
        {
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

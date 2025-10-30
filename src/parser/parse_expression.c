#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "token.h"
#include "utils.h"
#include "parse_expression.h"
#include "symbol_arrays.h"
#include "symbol_structs.h"

// Buffer size constants
#define IDENTIFIER_BUFFER_SIZE 128
#define NEGATED_VALUE_BUFFER_SIZE 128
#define INDEX_EXPRESSION_BUFFER_SIZE 512
// Full expression = identifier[index] + brackets + null terminator
#define FULL_EXPRESSION_BUFFER_SIZE (IDENTIFIER_BUFFER_SIZE + INDEX_EXPRESSION_BUFFER_SIZE + 3)
#define OPERATOR_COPY_BUFFER_SIZE 8

// Precedence constants for expression parsing
// Parenthesized expressions start at precedence 1 (above bitwise OR at 0)
#define PARENTHESIZED_EXPR_MIN_PRECEDENCE 1
// Top-level expressions start at -2 (includes all operators including logical OR at -2)
#define TOP_LEVEL_EXPR_MIN_PRECEDENCE -2

// Forward declarations for helper functions (mutual recursion with parse_primary)
static ASTNode* parse_logical_not(FILE *input);
static ASTNode* parse_bitwise_not(FILE *input);
static ASTNode* parse_unary_minus(FILE *input);

// Safe append helper to avoid strncat truncation warnings
static inline void safe_append(char *dst, size_t dst_size, const char *src)
{
    size_t used = strlen(dst);
    if (used >= dst_size - 1) {
        return;
    }
    size_t available_space = dst_size - 1 - used;
    size_t copy = strlen(src);
    if (copy > available_space) {
        copy = available_space;
    }
    memcpy(dst + used, src, copy);
    dst[used + copy] = '\0';
}

static inline void safe_copy(char *dst, size_t dst_size, const char *src)
{
    if (!dst_size) {
        return;
    }
    size_t n = strlen(src);
    if (n >= dst_size) {
        n = dst_size - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

// Helper: Parse logical NOT operator (!)
static ASTNode* parse_logical_not(FILE *input)
{
    ASTNode *operand = NULL;
    ASTNode *not_node = NULL;
    
    advance(input);
    operand = parse_primary(input);
    if (!operand) {
        return NULL;
    }
    
    not_node = create_node(NODE_BINARY_OP);
    not_node->value = strdup("!");
    add_child(not_node, operand);
    return not_node;
}

// Helper: Parse bitwise NOT operator (~)
static ASTNode* parse_bitwise_not(FILE *input)
{
    ASTNode *operand = NULL;
    ASTNode *not_node = NULL;
    
    advance(input);
    operand = parse_primary(input);
    if (!operand) {
        return NULL;
    }
    
    not_node = create_node(NODE_BINARY_OP);
    not_node->value = strdup("~");
    add_child(not_node, operand);
    return not_node;
}

// Helper: Parse unary minus operator (-)
static ASTNode* parse_unary_minus(FILE *input)
{
    ASTNode *operand = NULL;
    ASTNode *result_node = NULL;
    ASTNode *zero_node = NULL;
    ASTNode *binary_expr = NULL;
    char negated_value[NEGATED_VALUE_BUFFER_SIZE] = {0};
    
    advance(input);
    operand = parse_primary(input);
    if (!operand) {
        return NULL;
    }
    
    if (operand->type == NODE_EXPRESSION && operand->value) {
        snprintf(negated_value, sizeof(negated_value), "-%s", operand->value);
        result_node = create_node(NODE_EXPRESSION);
        result_node->value = strdup(negated_value);
        free_node(operand);
        return result_node;
    }
    
    zero_node = create_node(NODE_EXPRESSION);
    zero_node->value = strdup("0");
    binary_expr = create_node(NODE_BINARY_EXPR);
    binary_expr->value = strdup("-");
    add_child(binary_expr, zero_node);
    add_child(binary_expr, operand);
    return binary_expr;
}

// Helper: Parse parenthesized expression
static ASTNode* parse_parenthesized_expr(FILE *input)
{
    ASTNode *expr_node = NULL;
    
    advance(input);
    expr_node = parse_expression_prec(input, PARENTHESIZED_EXPR_MIN_PRECEDENCE);
    
    if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
        printf("Error (line %d): Expected ')' after expression\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    return expr_node;
}

// Helper: Parse field access (e.g., struct.field.subfield)
static void parse_field_access(FILE *input, char *identifier_buffer, size_t buffer_size)
{
    while (match(TOKEN_OPERATOR) && strcmp(current_token.value, ".") == 0) {
        advance(input);
        
        if (!match(TOKEN_IDENTIFIER)) {
            printf("Error (line %d): Expected field name after '.'\n", current_token.line);
            exit(EXIT_FAILURE);
        }
        
        safe_append(identifier_buffer, buffer_size, "__");
        safe_append(identifier_buffer, buffer_size, current_token.value);
        advance(input);
    }
}

// Helper: Parse array index expression
static void parse_array_index(FILE *input, char *index_buffer, size_t buffer_size)
{
    int parenthesis_depth = 0;
    
    advance(input);
    
    while (!match(TOKEN_EOF)) {
        if (match(TOKEN_BRACKET_CLOSE) && parenthesis_depth == 0) {
            break;
        }
        
        if (match(TOKEN_PARENTHESIS_OPEN)) {
            safe_append(index_buffer, buffer_size, "(");
            advance(input);
            parenthesis_depth++;
            continue;
        }
        
        if (match(TOKEN_PARENTHESIS_CLOSE)) {
            safe_append(index_buffer, buffer_size, ")");
            advance(input);
            if (parenthesis_depth > 0) {
                parenthesis_depth--;
            }
            continue;
        }
        
        if (current_token.value) {
            safe_append(index_buffer, buffer_size, current_token.value);
        }
        advance(input);
    }
    
    if (!consume(input, TOKEN_BRACKET_CLOSE)) {
        printf("Error (line %d): Expected ']' after array index in expression\n", current_token.line);
        exit(EXIT_FAILURE);
    }
}

// Helper: Validate array bounds if index is a constant number
static void validate_array_bounds(const char *identifier_name, const char *index_expr)
{
    int index_value = 0;
    int array_size = 0;
    
    if (!is_number_str(index_expr)) {
        return;
    }
    
    index_value = atoi(index_expr);
    array_size = find_array_size(identifier_name);
    
    if (array_size > 0 && (index_value < 0 || index_value >= array_size)) {
        printf("Error (line %d): Array index %d out of bounds for '%s' with size %d\n",
               current_token.line, index_value, identifier_name, array_size);
        exit(EXIT_FAILURE);
    }
}

// Helper: Parse identifier with optional field access and array indexing
static ASTNode* parse_identifier(FILE *input)
{
    ASTNode *identifier_node = NULL;
    char identifier_name[IDENTIFIER_BUFFER_SIZE] = {0};
    char index_expression[INDEX_EXPRESSION_BUFFER_SIZE] = {0};
    char full_expression[FULL_EXPRESSION_BUFFER_SIZE] = {0};
    
    safe_copy(identifier_name, sizeof(identifier_name), current_token.value);
    advance(input);
    
    // Handle field access (e.g., struct.field)
    parse_field_access(input, identifier_name, sizeof(identifier_name));
    
    // Handle array indexing
    if (match(TOKEN_BRACKET_OPEN)) {
        parse_array_index(input, index_expression, sizeof(index_expression));
        
        identifier_node = create_node(NODE_EXPRESSION);
        snprintf(full_expression, sizeof(full_expression), "%s[%s]", identifier_name, index_expression);
        identifier_node->value = strdup(full_expression);
        
        validate_array_bounds(identifier_name, index_expression);
        return identifier_node;
    }
    
    // Simple identifier
    identifier_node = create_node(NODE_EXPRESSION);
    identifier_node->value = strdup(identifier_name);
    return identifier_node;
}

// Helper: Parse number literal
static ASTNode* parse_number(FILE *input)
{
    ASTNode *number_node = create_node(NODE_EXPRESSION);
    number_node->value = strdup(current_token.value);
    advance(input);
    return number_node;
}

// Primary: identifiers, numbers, unary minus, logical/bitwise NOT, parentheses, field & array access
ASTNode* parse_primary(FILE *input)
{
    // Logical NOT operator
    if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "!") == 0) {
        return parse_logical_not(input);
    }
    
    // Bitwise NOT operator
    if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "~") == 0) {
        return parse_bitwise_not(input);
    }
    
    // Unary minus operator
    if (match(TOKEN_OPERATOR) && strcmp(current_token.value, "-") == 0) {
        return parse_unary_minus(input);
    }
    
    // Parenthesized expression
    if (match(TOKEN_PARENTHESIS_OPEN)) {
        return parse_parenthesized_expr(input);
    }
    
    // Identifier (with optional field access and array indexing)
    if (match(TOKEN_IDENTIFIER)) {
        return parse_identifier(input);
    }
    
    // Number literal
    if (match(TOKEN_NUMBER)) {
        return parse_number(input);
    }
    
    return NULL;
}

ASTNode* parse_expression_prec(FILE *input, int min_prec)
{
    ASTNode *left_operand = NULL;
    ASTNode *right_operand = NULL;
    ASTNode *binary_expr = NULL;
    const char *operator = NULL;
    int operator_precedence = 0;
    char operator_copy[OPERATOR_COPY_BUFFER_SIZE] = {0};

    left_operand = parse_primary(input);
    if (!left_operand) {
        return NULL;
    }
    while (match(TOKEN_OPERATOR)) {
        operator = current_token.value;
        operator_precedence = get_precedence(operator);
        if (operator_precedence < min_prec) {
            break;
        }
        safe_copy(operator_copy, sizeof(operator_copy), operator);
        advance(input);
        right_operand = parse_expression_prec(input, operator_precedence + 1);
        if (!right_operand) {
            printf("Error (line %d): Expected right operand after operator '%s'\n", current_token.line, operator_copy);
            exit(EXIT_FAILURE);
        }
        binary_expr = create_node(NODE_BINARY_EXPR);
        binary_expr->value = strdup(operator_copy);
        add_child(binary_expr, left_operand);
        add_child(binary_expr, right_operand);
        left_operand = binary_expr;
    }
    return left_operand;
}

ASTNode* parse_expression(FILE *input)
{ 
    return parse_expression_prec(input, TOP_LEVEL_EXPR_MIN_PRECEDENCE);
}

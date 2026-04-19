#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "error_handler.h"
#include "utils.h"
#include "parse_expression.h"
#include "symbol_arrays.h"

// Buffer size constants
#define IDENTIFIER_BUFFER_SIZE 128
#define NEGATED_VALUE_BUFFER_SIZE 128
#define INDEX_EXPRESSION_BUFFER_SIZE 512
// Full expression buffer = identifier + '[' + index + ']' + '\0'
#define FULL_EXPRESSION_BUFFER_SIZE (IDENTIFIER_BUFFER_SIZE + INDEX_EXPRESSION_BUFFER_SIZE + 3)
#define OPERATOR_COPY_BUFFER_SIZE 8

// Helper: Parse logical NOT operator (!)
static ASTNode* parse_logical_not(ParserContext *ctx)
{
    advance(ctx);
    ASTNode *operand = parse_primary(ctx);
    if (!operand) {
        return NULL;
    }
    
    ASTNode *not_node = create_node(NODE_BINARY_OP);
    not_node->value = safe_strdup("!");
    add_child(not_node, operand);
    return not_node;
}

// Helper: Parse bitwise NOT operator (~)
static ASTNode* parse_bitwise_not(ParserContext *ctx)
{
    advance(ctx);
    ASTNode *operand = parse_primary(ctx);
    if (!operand) {
        return NULL;
    }
    
    ASTNode *not_node = create_node(NODE_BINARY_OP);
    not_node->value = safe_strdup("~");
    add_child(not_node, operand);
    return not_node;
}

// Helper: Parse unary minus operator (-)
static ASTNode* parse_unary_minus(ParserContext *ctx)
{
    advance(ctx);
    ASTNode *operand = parse_primary(ctx);
    if (!operand) {
        return NULL;
    }
    
    if (operand->type == NODE_EXPRESSION && operand->value) {
        char negated_value[NEGATED_VALUE_BUFFER_SIZE] = {0};
        snprintf(negated_value, sizeof(negated_value), "-%s", operand->value);
        ASTNode *result_node = create_node(NODE_EXPRESSION);
        result_node->value = safe_strdup(negated_value);
        free_node(operand);
        return result_node;
    }
    
    ASTNode *zero_node = create_node(NODE_EXPRESSION);
    zero_node->value = safe_strdup("0");
    ASTNode *binary_expr = create_node(NODE_BINARY_EXPR);
    binary_expr->value = safe_strdup("-");
    add_child(binary_expr, zero_node);
    add_child(binary_expr, operand);
    return binary_expr;
}

// Helper: Parse parenthesized expression
static ASTNode* parse_parenthesized_expr(ParserContext *ctx)
{
    advance(ctx);
    ASTNode *expr_node = parse_expression_prec(ctx, PREC_PARENTHESIZED_MIN);
    
    if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ')' after expression");
        if (expr_node) {
            free_node(expr_node);  // Clean up on error
        }
        return NULL;
    }
    
    return expr_node;
}

// Helper: Parse field access (e.g., struct.field.subfield)
static void parse_field_access(ParserContext *ctx, char *identifier_buffer, size_t buffer_size)
{
    while (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, ".") == 0) {
        advance(ctx);
        
        if (!match(ctx, TOKEN_IDENTIFIER)) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected field name after '.'");
            return;
        }
        
        safe_append(identifier_buffer, buffer_size, "__");
        safe_append(identifier_buffer, buffer_size, ctx->current_token.value);
        advance(ctx);
    }
}

// Helper: Parse array index expression
static void parse_array_index(ParserContext *ctx, char *index_buffer, size_t buffer_size)
{
    int parenthesis_depth = 0;
    
    advance(ctx);
    
    while (!match(ctx, TOKEN_EOF)) {
        if (match(ctx, TOKEN_BRACKET_CLOSE) && parenthesis_depth == 0) {
            break;
        }
        
        if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
            safe_append(index_buffer, buffer_size, "(");
            advance(ctx);
            parenthesis_depth++;
            continue;
        }
        
        if (match(ctx, TOKEN_PARENTHESIS_CLOSE)) {
            safe_append(index_buffer, buffer_size, ")");
            advance(ctx);
            if (parenthesis_depth > 0) {
                parenthesis_depth--;
            }
            continue;
        }
        
        if (ctx->current_token.value[0] != '\0') {
            safe_append(index_buffer, buffer_size, ctx->current_token.value);
        }
        advance(ctx);
    }
    
    if (!consume(ctx, TOKEN_BRACKET_CLOSE)) {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ']' after array index in expression");
        return;
    }
}

// Helper: Validate array bounds if index is a constant number
static void validate_array_bounds(const ParserContext *ctx, const char *identifier_name, const char *index_expr)
{
    int index_value = 0;
    int array_size = 0;
    
    if (!is_number_str(index_expr)) {
        return;
    }
    
    if (!safe_strtoi(index_expr, &index_value)) {
        return;
    }
    array_size = find_array_size(identifier_name);
    
    if (array_size > 0 && (index_value < 0 || index_value >= array_size)) {
        log_error(ERROR_CATEGORY_SEMANTIC, ctx->current_token.line,
                  "Array index %d out of bounds for '%s' with size %d",
                  index_value, identifier_name, array_size);
        return;
    }
}

// Helper: Parse function call arguments
/**
 * Parses function call arguments (shared helper for expressions and statements).
 * 
 * Syntax: (arg1, arg2, ..., argN)
 * 
 * The opening parenthesis should already be matched before calling this function.
 * This function consumes the opening parenthesis, parses comma-separated arguments,
 * and expects a closing parenthesis.
 * 
 * @param ctx           Parser context
 * @param function_name Name of the function being called (for error messages)
 * @return              NODE_FUNC_CALL node with arguments as children
 */
ASTNode* parse_function_call_args(ParserContext *ctx, const char *function_name)
{
    ASTNode *call_node = NULL;
    ASTNode *arg_node = NULL;
    
    call_node = create_node(NODE_FUNC_CALL);
    call_node->value = safe_strdup(function_name);
    
    // Consume opening parenthesis
    advance(ctx);
    
    // Parse zero or more comma-separated arguments
    while (!match(ctx, TOKEN_PARENTHESIS_CLOSE) && !match(ctx, TOKEN_EOF))
    {
        arg_node = parse_expression_prec(ctx, PREC_TOP_LEVEL_MIN);
        if (arg_node)
        {
            add_child(call_node, arg_node);
        }
        
        if (match(ctx, TOKEN_COMMA))
        {
            advance(ctx);
            continue;
        }
        else
        {
            break;
        }
    }
    
    if (!consume(ctx, TOKEN_PARENTHESIS_CLOSE))
    {
        log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                  "Expected ')' after function call arguments for '%s'",
                  function_name);
        free_node(call_node);
        return NULL;
    }
    
    return call_node;
}

/**
 * Parses a function call with its arguments.
 */
static ASTNode* parse_function_call(ParserContext *ctx, const char *function_name)
{
    return parse_function_call_args(ctx, function_name);
}

// Build an array-access expression node: identifier[index].
// Validates bounds at parse time when the index is a compile-time constant.
static ASTNode* parse_array_access(ParserContext *ctx, const char *identifier_name)
{
    char index_expression[INDEX_EXPRESSION_BUFFER_SIZE] = {0};
    char full_expression[FULL_EXPRESSION_BUFFER_SIZE] = {0};
    
    parse_array_index(ctx, index_expression, sizeof(index_expression));
    if (has_errors()) {
        return NULL;
    }
    
    ASTNode *node = create_node(NODE_EXPRESSION);
    snprintf(full_expression, sizeof(full_expression), "%s[%s]", identifier_name, index_expression);
    node->value = safe_strdup(full_expression);
    
    validate_array_bounds(ctx, identifier_name, index_expression);
    if (has_errors()) {
        free_node(node);
        return NULL;
    }
    return node;
}

// Identifier dispatch: function call, struct field access, array indexing, or plain variable.
// Each access pattern is delegated to a dedicated helper so this stays a flat dispatcher
// and new access patterns (e.g. pointer dereference) can be added without nesting.
static ASTNode* parse_identifier(ParserContext *ctx)
{
    char identifier_name[IDENTIFIER_BUFFER_SIZE] = {0};
    
    safe_copy(identifier_name, sizeof(identifier_name), ctx->current_token.value, sizeof(identifier_name) - 1);
    advance(ctx);
    
    if (match(ctx, TOKEN_PARENTHESIS_OPEN))
    {
        return parse_function_call(ctx, identifier_name);
    }
    
    parse_field_access(ctx, identifier_name, sizeof(identifier_name));
    if (has_errors()) {
        return NULL;
    }
    
    if (match(ctx, TOKEN_BRACKET_OPEN))
    {
        return parse_array_access(ctx, identifier_name);
    }
    
    ASTNode *identifier_node = create_node(NODE_EXPRESSION);
    identifier_node->value = safe_strdup(identifier_name);
    return identifier_node;
}

// Helper: Parse number literal
static ASTNode* parse_number(ParserContext *ctx)
{
    ASTNode *number_node = create_node(NODE_EXPRESSION);
    number_node->value = safe_strdup(ctx->current_token.value);
    advance(ctx);
    return number_node;
}

// Primary: identifiers, numbers, unary minus, logical/bitwise NOT, parentheses, field & array access
ASTNode* parse_primary(ParserContext *ctx)
{
    // Logical NOT operator
    if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "!") == 0) {
        return parse_logical_not(ctx);
    }
    
    // Bitwise NOT operator
    if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "~") == 0) {
        return parse_bitwise_not(ctx);
    }
    
    // Unary minus operator
    if (match(ctx, TOKEN_OPERATOR) && strcmp(ctx->current_token.value, "-") == 0) {
        return parse_unary_minus(ctx);
    }
    
    // Parenthesized expression
    if (match(ctx, TOKEN_PARENTHESIS_OPEN)) {
        return parse_parenthesized_expr(ctx);
    }
    
    // Identifier (with optional field access and array indexing)
    if (match(ctx, TOKEN_IDENTIFIER)) {
        return parse_identifier(ctx);
    }
    
    // Number literal
    if (match(ctx, TOKEN_NUMBER)) {
        return parse_number(ctx);
    }
    
    return NULL;
}

ASTNode* parse_expression_prec(ParserContext *ctx, int min_prec)
{
    ASTNode *left_operand = parse_primary(ctx);
    if (!left_operand) {
        return NULL;
    }
    while (match(ctx, TOKEN_OPERATOR)) {
        const char *op = ctx->current_token.value;
        int operator_precedence = get_precedence(op);
        if (operator_precedence < min_prec) {
            break;
        }
        char operator_copy[OPERATOR_COPY_BUFFER_SIZE] = {0};
        safe_copy(operator_copy, sizeof(operator_copy), op, sizeof(operator_copy) - 1);
        advance(ctx);
        ASTNode *right_operand = parse_expression_prec(ctx, operator_precedence + 1);
        if (!right_operand) {
            log_error(ERROR_CATEGORY_PARSER, ctx->current_token.line,
                      "Expected right operand after operator '%s'", operator_copy);
            free_node(left_operand);
            return NULL;
        }
        ASTNode *binary_expr = create_node(NODE_BINARY_EXPR);
        binary_expr->value = safe_strdup(operator_copy);
        add_child(binary_expr, left_operand);
        add_child(binary_expr, right_operand);
        left_operand = binary_expr;
    }
    return left_operand;
}

ASTNode* parse_expression(ParserContext *ctx)
{ 
    return parse_expression_prec(ctx, PREC_TOP_LEVEL_MIN);
}

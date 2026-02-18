#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_function.h"
#include "parse_statement.h"
#include "symbol_arrays.h"
#include "parse.h" // create_node/add_child
#include "token.h"

// Constants
#define MAX_ARRAYS 128
#define INITIAL_BRACE_DEPTH 1

extern Token current_token;
extern ArrayInfo g_arrays[MAX_ARRAYS];
extern int g_array_count;

static void parse_function_parameters(FILE *input, ASTNode *function_node);
static void parse_function_body(FILE *input, ASTNode *function_node);

// Helper: Parse a single function parameter (type and name)
static ASTNode* parse_single_parameter(FILE *input, Token *parameter_type)
{
    Token parameter_name = (Token){0};
    ASTNode *parameter_node = NULL;
    
    // Handle struct types
    if (strcmp(current_token.value, "struct") == 0) {
        advance(input);
        if (!match(TOKEN_IDENTIFIER)) {
            printf("Error (line %d): Expected struct name in parameter list\n", current_token.line);
            return NULL;
        }
        *parameter_type = current_token;
        advance(input);
    } else {
        *parameter_type = current_token;
        advance(input);
    }
    
    // Get parameter name
    if (!match(TOKEN_IDENTIFIER)) {
        printf("Error (line %d): Expected parameter name\n", current_token.line);
        return NULL;
    }
    
    parameter_name = current_token;
    advance(input);
    
    // Create parameter node
    parameter_node = create_node(NODE_VAR_DECL);
    parameter_node->token = *parameter_type;
    parameter_node->value = strdup(parameter_name.value);
    
    return parameter_node;
}

// Helper: Parse all function parameters
static void parse_function_parameters(FILE *input, ASTNode *function_node)
{
    Token parameter_type = (Token){0};
    ASTNode *parameter_node = NULL;
    
    if (!consume(input, TOKEN_PARENTHESIS_OPEN)) {
        printf("Error (line %d): Expected '(' after function name\n", current_token.line);
        free_node(function_node);
        exit(EXIT_FAILURE);
    }
    
    while (!match(TOKEN_PARENTHESIS_CLOSE) && !match(TOKEN_EOF)) {
        if (match(TOKEN_KEYWORD)) {
            parameter_node = parse_single_parameter(input, &parameter_type);
            if (!parameter_node) {
                break;
            }
            
            add_child(function_node, parameter_node);
            
            // Handle comma between parameters
            if (match(TOKEN_COMMA)) {
                advance(input);
            }
        } else {
            advance(input);
        }
    }
    
    if (!consume(input, TOKEN_PARENTHESIS_CLOSE)) {
        printf("Error (line %d): Expected ')' after parameter list\n", current_token.line);
        free_node(function_node);
        exit(EXIT_FAILURE);
    }
}

// Helper: Parse function body (statements within braces)
static void parse_function_body(FILE *input, ASTNode *function_node)
{
    int brace_depth = INITIAL_BRACE_DEPTH;
    ASTNode *statement_node = NULL;
    
    if (!consume(input, TOKEN_BRACE_OPEN)) {
        printf("Error (line %d): Expected '{' to start function body\n", current_token.line);
        free_node(function_node);
        exit(EXIT_FAILURE);
    }
    
    while (brace_depth > 0 && !match(TOKEN_EOF)) {
        if (match(TOKEN_BRACE_OPEN)) {
            brace_depth++;
            advance(input);
        } else if (match(TOKEN_BRACE_CLOSE)) {
            brace_depth--;
            advance(input);
        } else {
            statement_node = parse_statement(input);
            if (statement_node) {
                add_child(function_node, statement_node);
            }
        }
    }
}

ASTNode* parse_function(FILE *input, Token return_type, Token function_name)
{
    ASTNode *function_node = NULL;
    
    // Initialize function node
    function_node = create_node(NODE_FUNCTION_DECL);
    function_node->token = return_type;
    function_node->value = strdup(function_name.value);
    
    // Reset global array count for this function scope
    g_array_count = 0;
    
    // Parse function parameters
    parse_function_parameters(input, function_node);
    
    // Parse function body
    parse_function_body(input, function_node);
    
    return function_node;
}

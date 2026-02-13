#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "utils.h"
#include "token.h"
#include "symbol_structs.h"
#include "symbol_arrays.h"
#include "parse_expression.h"
#include "parse_struct.h"
#include "parse_function.h"
#include "parse_statement.h"
#include "codegen_vhdl.h"
#include <ctype.h>

// Current token shared by parsing modules
extern Token current_token;

// Forward declarations
static ASTNode* parse_struct_declaration(FILE *input, ASTNode *program_node);
static ASTNode* parse_function_declaration(FILE *input, Token return_type, ASTNode *program_node);
static void skip_to_semicolon(FILE *input);

// Skip tokens until we find a semicolon or EOF
static void skip_to_semicolon(FILE *input)
{
    while (!match(TOKEN_SEMICOLON) && !match(TOKEN_EOF)) {
        advance(input);
    }
    if (match(TOKEN_SEMICOLON)) {
        advance(input);
    }
}

// Parse struct declaration or function returning struct
static ASTNode* parse_struct_declaration(FILE *input, ASTNode *program_node)
{
    advance(input); // consume 'struct'
    
    if (!match(TOKEN_IDENTIFIER)) {
        printf("Warning: 'struct' without name at line %d\n", current_token.line);
        return NULL;
    }
    
    Token struct_name_token = current_token;
    advance(input);
    
    // struct definition: struct Name { ... };
    if (match(TOKEN_BRACE_OPEN)) {
        ASTNode *struct_node = parse_struct(input, struct_name_token);
        if (struct_node) {
            add_child(program_node, struct_node);
        }
        return struct_node;
    }
    
    // Function returning struct: struct Name funcname(...) { ... }
    if (match(TOKEN_IDENTIFIER)) {
        Token function_name = current_token;
        advance(input);
        
        if (match(TOKEN_PARENTHESIS_OPEN)) {
            ASTNode *function_node = parse_function(input, struct_name_token, function_name);
            if (function_node) {
                add_child(program_node, function_node);
            }
            return function_node;
        } else {
            printf("Warning: Expected '(' after function name for struct return function '%s'\n", 
                   function_name.value);
        }
    } else {
        printf("Warning: 'struct %s' not followed by function name or '{'\n", 
               struct_name_token.value);
    }
    
    return NULL;
}

// Parse function declaration with primitive return type
static ASTNode* parse_function_declaration(FILE *input, Token return_type, ASTNode *program_node)
{
    if (!match(TOKEN_IDENTIFIER)) {
        printf("Warning: Expected identifier after type at line %d\n", current_token.line);
        advance(input);
        return NULL;
    }
    
    Token function_name = current_token;
    advance(input);
    
    if (match(TOKEN_PARENTHESIS_OPEN)) {
        ASTNode *function_node = parse_function(input, return_type, function_name);
        if (function_node) {
            add_child(program_node, function_node);
        }
        return function_node;
    } else {
        printf("Warning: Global variable declarations not yet implemented\n");
        skip_to_semicolon(input);
        return NULL;
    }
}

// Parse the entire program: delegates to specialized modules
ASTNode* parse_program(FILE *input)
{
    ASTNode *program_node = create_node(NODE_PROGRAM);

    advance(input); // prime tokenizer

    while (!match(TOKEN_EOF)) {
        #ifdef DEBUG
        printf("Parsing token: type=%d, value='%s'\n", 
               current_token.type, 
               current_token.value ? current_token.value : "");
        #endif
        
        if (match(TOKEN_KEYWORD)) {
            if (strcmp(current_token.value, "struct") == 0) {
                parse_struct_declaration(input, program_node);
                continue;
            }
            
            // Primitive or known type function
            Token return_type = current_token;
            advance(input);
            parse_function_declaration(input, return_type, program_node);
        } else {
            advance(input); // Skip unknown token
        }
    }
    
    return program_node;
}
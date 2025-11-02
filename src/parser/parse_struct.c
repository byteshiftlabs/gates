#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_struct.h"
#include "symbol_structs.h"
#include "parse.h"
#include "token.h"

#define MAX_STRUCT_FIELDS 32

static inline void safe_copy(char *dst, size_t dst_size, const char *src)
{
    if (!dst_size) {
        return;
    }
    size_t copy_length = strlen(src);
    if (copy_length >= dst_size) {
        copy_length = dst_size - 1;
    }
    memcpy(dst, src, copy_length);
    dst[copy_length] = '\0';
}

extern Token current_token;

// Forward declarations
static void register_struct_in_table(int struct_index, Token struct_name_token);
static ASTNode* parse_struct_field(FILE *input, int struct_index);
static void register_field_in_struct(int struct_index, Token field_type, Token field_name);


// Register struct name in global struct table
static void register_struct_in_table(int struct_index, Token struct_name_token)
{
    if (g_struct_count < (int)(sizeof(g_structs)/sizeof(g_structs[0]))) {
        safe_copy(g_structs[struct_index].name, 
                  sizeof(g_structs[struct_index].name), 
                  struct_name_token.value);
        g_structs[struct_index].field_count = 0;
    }
}

// Register a field in the struct's field table
static void register_field_in_struct(int struct_index, Token field_type, Token field_name)
{
    if (struct_index != g_struct_count || 
        g_struct_count >= (int)(sizeof(g_structs)/sizeof(g_structs[0]))) {
        return;
    }
    
    StructInfo *struct_info = &g_structs[struct_index];
    if (struct_info->field_count >= MAX_STRUCT_FIELDS) {
        return;
    }
    
    int field_index = struct_info->field_count;
    safe_copy(struct_info->fields[field_index].field_name, 
              sizeof(struct_info->fields[field_index].field_name), 
              field_name.value);
    safe_copy(struct_info->fields[field_index].field_type, 
              sizeof(struct_info->fields[field_index].field_type), 
              field_type.value);
    struct_info->field_count++;
}

// Parse a single struct field: type name;
static ASTNode* parse_struct_field(FILE *input, int struct_index)
{
    if (!match(TOKEN_KEYWORD)) {
        return NULL;
    }
    
    Token field_type = current_token;
    advance(input);
    
    if (!match(TOKEN_IDENTIFIER)) {
        printf("Error (line %d): Expected field name in struct\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    Token field_name = current_token;
    advance(input);
    
    ASTNode *field_node = create_node(NODE_VAR_DECL);
    field_node->token = field_type;
    field_node->value = strdup(field_name.value);
    
    register_field_in_struct(struct_index, field_type, field_name);
    
    if (!consume(input, TOKEN_SEMICOLON)) {
        printf("Error (line %d): Expected ';' after struct field\n", current_token.line);
        exit(EXIT_FAILURE);
    }
    
    return field_node;
}

// Parse struct definition: struct Name { type field; ... };
ASTNode* parse_struct(FILE *input, Token struct_name_token)
{
    if (!consume(input, TOKEN_BRACE_OPEN)) {
        printf("Error (line %d): Expected '{' after struct name\n", current_token.line);
        return NULL;
    }
    
    ASTNode *struct_node = create_node(NODE_STRUCT_DECL);
    struct_node->value = strdup(struct_name_token.value);
    
    int struct_index = g_struct_count;
    register_struct_in_table(struct_index, struct_name_token);
    
    // Parse all fields
    while (!match(TOKEN_BRACE_CLOSE) && !match(TOKEN_EOF)) {
        ASTNode *field_node = parse_struct_field(input, struct_index);
        if (field_node) {
            add_child(struct_node, field_node);
        } else {
            // Skip unknown tokens
            advance(input);
        }
    }
    
    if (!consume(input, TOKEN_BRACE_CLOSE)) {
        printf("Error (line %d): Expected '}' after struct body\n", current_token.line);
    }
    if (!consume(input, TOKEN_SEMICOLON)) {
        printf("Error (line %d): Expected ';' after struct declaration\n", current_token.line);
    }
    
    if (struct_index == g_struct_count) {
        g_struct_count++;
    }
    
    return struct_node;
}

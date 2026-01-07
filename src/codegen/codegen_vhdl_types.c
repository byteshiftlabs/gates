// VHDL Code Generator - Type and Signal Declarations Implementation
// -------------------------------------------------------------

#include "codegen_vhdl_types.h"
#include "codegen_vhdl_constants.h"
#include "symbol_structs.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

// -------------------------------------------------------------
// Emit all struct type declarations as VHDL records
// -------------------------------------------------------------
void emit_all_struct_declarations(FILE *output_file)
{
    int struct_idx = 0;
    int field_index = 0;
    
    for (struct_idx = 0; struct_idx < g_struct_count; ++struct_idx)
    {
        StructInfo *struct_info = &g_structs[struct_idx];
        
        fprintf(output_file, "-- Struct %s as VHDL record\n", struct_info->name);
        fprintf(output_file, "type %s_t is record\n", struct_info->name);
        
        for (field_index = 0; field_index < struct_info->field_count; ++field_index)
        {
            fprintf(output_file, "  %s : %s;\n", 
                    struct_info->fields[field_index].field_name, 
                    ctype_to_vhdl(struct_info->fields[field_index].field_type));
        }
        
        fprintf(output_file, "end record;\n\n");
    }
}

// -------------------------------------------------------------
// Helper: Emit struct type signal declaration
// -------------------------------------------------------------
void emit_struct_signal_declaration(ASTNode *var_decl, FILE *output_file)
{
    fprintf(output_file, "  signal %s : %s_t;\n", 
            var_decl->value, var_decl->token.value);
}

// -------------------------------------------------------------
// Helper: Parse array dimensions from variable name
// -------------------------------------------------------------
int parse_array_dimensions(const char *var_name, char *array_name, 
                          char *array_size, int name_buf_size, int size_buf_size)
{
    const char *left_bracket = NULL;
    const char *size_start = NULL;
    const char *size_end = NULL;
    int name_length = 0;
    int size_length = 0;
    
    if (var_name == NULL || array_name == NULL || array_size == NULL)
    {
        return 0;
    }
    
    left_bracket = strchr(var_name, '[');
    if (left_bracket == NULL)
    {
        return 0;
    }
    
    name_length = (int)(left_bracket - var_name);
    if (name_length >= name_buf_size)
    {
        return 0;
    }
    
    strncpy(array_name, var_name, name_length);
    array_name[name_length] = '\0';
    
    size_start = left_bracket + 1;
    size_end = strchr(size_start, ']');
    
    if (size_end == NULL || size_end <= size_start)
    {
        return 0;
    }
    
    size_length = (int)(size_end - size_start);
    if (size_length >= size_buf_size)
    {
        return 0;
    }
    
    strncpy(array_size, size_start, size_length);
    array_size[size_length] = '\0';
    
    return 1;
}

// -------------------------------------------------------------
// Helper: Emit array initializer constant
// -------------------------------------------------------------
void emit_array_initializer_constant(ASTNode *var_decl, ASTNode *init_list, 
                                     const char *array_name, FILE *output_file)
{
    int element_index = 0;
    const char *element_value = NULL;
    
    fprintf(output_file, "  -- Array initialization\n");
    fprintf(output_file, "  constant %s_init : %s_type := (", array_name, array_name);
    
    for (element_index = 0; element_index < init_list->num_children; ++element_index)
    {
        element_value = init_list->children[element_index]->value;
        int is_last_element = (element_index == init_list->num_children - 1);
        const char *separator = is_last_element ? "" : ", ";
        
        if (strcmp(var_decl->token.value, C_TYPE_INT) == 0)
        {
            char bit_string[BITSTRING_BUFFER_SIZE] = {0};
            int numeric_value = atoi(element_value);
            int bit_position = 0;
            
            for (bit_position = VHDL_BIT_WIDTH - 1; bit_position >= 0; --bit_position)
            {
                int bit_index = (VHDL_BIT_WIDTH - 1) - bit_position;
                bit_string[bit_index] = ((numeric_value >> bit_position) & 1) ? '1' : '0';
            }
            bit_string[VHDL_BIT_WIDTH] = '\0';
            
            fprintf(output_file, "\"%s\"%s", bit_string, separator);
        }
        else if (strcmp(var_decl->token.value, C_TYPE_FLOAT) == 0 || 
                 strcmp(var_decl->token.value, C_TYPE_DOUBLE) == 0)
        {
            fprintf(output_file, "%s%s", element_value, separator);
        }
        else if (strcmp(var_decl->token.value, C_TYPE_CHAR) == 0)
        {
            fprintf(output_file, "'%s'%s", element_value, separator);
        }
        else
        {
            fprintf(output_file, "%s%s", element_value, separator);
        }
    }
    
    fprintf(output_file, ");\n");
    fprintf(output_file, "  signal %s : %s_type := %s_init;\n", 
            array_name, array_name, array_name);
}

// -------------------------------------------------------------
// Helper: Emit array signal declaration
// -------------------------------------------------------------
void emit_array_signal_declaration(ASTNode *var_decl, FILE *output_file)
{
    char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
    char array_size[ARRAY_SIZE_BUFFER_SIZE] = {0};
    int array_element_count = 0;
    const char *vhdl_element_type = NULL;
    int has_initializer = 0;
    ASTNode *initializer_list = NULL;
    
    if (!parse_array_dimensions(var_decl->value, array_name, array_size, 
                                ARRAY_NAME_BUFFER_SIZE, ARRAY_SIZE_BUFFER_SIZE))
    {
        return;
    }
    
    array_element_count = atoi(array_size) - 1;
    vhdl_element_type = ctype_to_vhdl(var_decl->token.value);
    
    fprintf(output_file, "  type %s_type is array (0 to %d) of %s;\n", 
            array_name, array_element_count, vhdl_element_type);
    fprintf(output_file, "  signal %s : %s_type;\n", array_name, array_name);
    
    // Check for array initializer
    has_initializer = (var_decl->num_children > 0 && 
                      var_decl->children[FIRST_CHILD_INDEX]->value != NULL &&
                      strcmp(var_decl->children[FIRST_CHILD_INDEX]->value, ARRAY_INIT_MARKER) == 0);
    
    if (has_initializer)
    {
        initializer_list = var_decl->children[FIRST_CHILD_INDEX];
        emit_array_initializer_constant(var_decl, initializer_list, array_name, output_file);
    }
}

// -------------------------------------------------------------
// Helper: Emit simple (non-array, non-struct) signal declaration
// -------------------------------------------------------------
void emit_simple_signal_declaration(ASTNode *var_decl, FILE *output_file)
{
    int is_result_variable = (strcmp(var_decl->value, RESERVED_PORT_NAME_RESULT) == 0);
    
    if (is_result_variable)
    {
        fprintf(output_file, "  signal ");
        fprintf(output_file, "%s%s", var_decl->value, SIGNAL_SUFFIX_LOCAL);
        fprintf(output_file, " : %s;\n", ctype_to_vhdl(var_decl->token.value));
    }
    else
    {
        fprintf(output_file, "  signal %s : %s;\n", 
                var_decl->value, ctype_to_vhdl(var_decl->token.value));
    }
}

// -------------------------------------------------------------
// Helper: Process a single variable declaration for signal emission
// -------------------------------------------------------------
void process_variable_declaration_for_signals(ASTNode *var_decl, FILE *output_file)
{
    int struct_index = 0;
    char *array_bracket = NULL;
    int is_struct_type = 0;
    int is_array_type = 0;
    
    struct_index = find_struct_index(var_decl->token.value);
    is_struct_type = (struct_index >= 0);
    
    if (is_struct_type)
    {
        emit_struct_signal_declaration(var_decl, output_file);
        return;
    }
    
    array_bracket = (var_decl->value != NULL) ? strchr(var_decl->value, '[') : NULL;
    is_array_type = (array_bracket != NULL);
    
    if (is_array_type)
    {
        emit_array_signal_declaration(var_decl, output_file);
    }
    else
    {
        emit_simple_signal_declaration(var_decl, output_file);
    }
}

// -------------------------------------------------------------
// Helper: Process variable declarations inside for loops
// -------------------------------------------------------------
void process_for_loop_declarations(ASTNode *for_statement, FILE *output_file)
{
    int child_index = 0;
    ASTNode *for_child = NULL;
    char *array_bracket = NULL;
    char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
    char array_size[ARRAY_SIZE_BUFFER_SIZE] = {0};
    
    for (child_index = 0; child_index < for_statement->num_children; ++child_index)
    {
        for_child = for_statement->children[child_index];
        
        if (for_child->type != NODE_VAR_DECL)
        {
            continue;
        }
        
        array_bracket = (for_child->value != NULL) ? strchr(for_child->value, '[') : NULL;
        
        if (array_bracket != NULL)
        {
            if (parse_array_dimensions(for_child->value, array_name, array_size,
                                      ARRAY_NAME_BUFFER_SIZE, ARRAY_SIZE_BUFFER_SIZE))
            {
                int array_element_count = atoi(array_size) - 1;
                const char *vhdl_element_type = ctype_to_vhdl(for_child->token.value);
                
                fprintf(output_file, "  type %s_type is array (0 to %d) of %s;\n", 
                        array_name, array_element_count, vhdl_element_type);
                fprintf(output_file, "  signal %s : %s_type;\n", array_name, array_name);
            }
        }
        else
        {
            fprintf(output_file, "  signal %s : %s;\n", 
                    for_child->value, ctype_to_vhdl(for_child->token.value));
        }
    }
}

// -------------------------------------------------------------
// Emit local signal declarations for a function
// -------------------------------------------------------------
void emit_function_local_signals(ASTNode *function_declaration, FILE *output_file)
{
    int child_index = 0;
    int statement_child_index = 0;
    ASTNode *child = NULL;
    ASTNode *statement_child = NULL;
    
    // Iterate through function children to find statement blocks
    for (child_index = 0; child_index < function_declaration->num_children; ++child_index)
    {
        child = function_declaration->children[child_index];
        
        if (child->type != NODE_STATEMENT)
        {
            continue;
        }
        
        // Process each child within the statement block
        for (statement_child_index = 0; 
             statement_child_index < child->num_children; 
             ++statement_child_index)
        {
            statement_child = child->children[statement_child_index];
            
            if (statement_child->type == NODE_VAR_DECL)
            {
                process_variable_declaration_for_signals(statement_child, output_file);
            }
            else if (statement_child->type == NODE_FOR_STATEMENT)
            {
                process_for_loop_declarations(statement_child, output_file);
            }
        }
    }
}

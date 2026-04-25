// VHDL Code Generator - Type and Signal Declarations Implementation
// -------------------------------------------------------------

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "codegen_vhdl_types.h"
#include "codegen_vhdl_constants.h"
#include "codegen_vhdl_emit.h"
#include "codegen_vhdl_helpers.h"
#include "error_handler.h"
#include "symbol_structs.h"
#include "utils.h"

// Generate VHDL record type definitions so C struct types exist
// as first-class types in the generated hardware description.
void emit_all_struct_declarations(void)
{
    for (int struct_idx = 0; struct_idx < get_struct_count(); ++struct_idx)
    {
        const StructInfo *struct_info = get_struct_info(struct_idx);
        if (!struct_info) {
            continue;
        }
        
        emit_raw("-- Struct %s as VHDL record\n", struct_info->name);
        emit_raw("type %s_t is record\n", struct_info->name);
        
        for (int field_index = 0; field_index < struct_info->field_count; ++field_index)
        {
            emit_raw("  %s : %s;\n", 
                    struct_info->fields[field_index].field_name, 
                    ctype_to_vhdl(struct_info->fields[field_index].field_type));
        }
        
        emit_raw("end record;\n\n");
    }
}

// Struct variables become typed signals in VHDL architecture declarations
static void emit_struct_signal_declaration(const ASTNode *var_decl)
{
    emit_raw("  signal %s : %s_t;\n", 
            var_decl->value, var_decl->token.value);
}

// Extract array name and size from C declaration syntax (e.g. "arr[10]")
static int parse_array_dimensions(const char *var_name, char *array_name, 
                          char *array_size, int name_buf_size, int size_buf_size)
{
    if (var_name == NULL || array_name == NULL || array_size == NULL)
    {
        return 0;
    }
    
    const char *left_bracket = strchr(var_name, '[');
    if (left_bracket == NULL)
    {
        return 0;
    }
    
    int name_length = (int)(left_bracket - var_name);
    if (name_length >= name_buf_size)
    {
        return 0;
    }
    
    strncpy(array_name, var_name, (size_t)name_length);
    array_name[name_length] = '\0';
    
    const char *size_start = left_bracket + 1;
    const char *size_end = strchr(size_start, ']');
    
    if (size_end == NULL || size_end <= size_start)
    {
        return 0;
    }
    
    int size_length = (int)(size_end - size_start);
    if (size_length >= size_buf_size)
    {
        return 0;
    }
    
    strncpy(array_size, size_start, (size_t)size_length);
    array_size[size_length] = '\0';
    
    return 1;
}

// -------------------------------------------------------------
// Helper: Emit array initializer constant
// -------------------------------------------------------------
static void emit_array_initializer_constant(const ASTNode *var_decl, ASTNode *init_list,
                                     const char *array_name)
{
    emit_raw("  -- Array initialization\n");
    emit_raw("  constant %s_init : %s_type := (", array_name, array_name);
    
    for (int element_index = 0; element_index < init_list->num_children; ++element_index)
    {
        const char *element_value = init_list->children[element_index]->value;
        int is_last_element = (element_index == init_list->num_children - 1);
        const char *separator = is_last_element ? "" : ", ";
        
        if (strcmp(var_decl->token.value, C_TYPE_INT) == 0)
        {
            char bit_string[BITSTRING_BUFFER_SIZE] = {0};
            int numeric_value = 0;
            int bit_position = 0;
            int source_bit_width = (int)(sizeof(unsigned int) * CHAR_BIT);
            unsigned int raw_bits = 0U;
            if (!safe_strtoi(element_value, &numeric_value))
            {
                log_error(ERROR_CATEGORY_CODEGEN, 0,
                          "Invalid numeric value '%s' in array initializer",
                          element_value);
                continue;
            }

            raw_bits = (unsigned int)numeric_value;
            
            for (bit_position = VHDL_BIT_WIDTH - 1; bit_position >= 0; --bit_position)
            {
                int bit_index = (VHDL_BIT_WIDTH - 1) - bit_position;
                int bit_value = 0;

                if (bit_position >= source_bit_width)
                {
                    bit_value = (numeric_value < 0) ? 1 : 0;
                }
                else
                {
                    bit_value = ((raw_bits >> bit_position) & 1U) ? 1 : 0;
                }

                bit_string[bit_index] = bit_value ? '1' : '0';
            }
            bit_string[VHDL_BIT_WIDTH] = '\0';
            
            emit_raw("\"%s\"%s", bit_string, separator);
        }
        else if (strcmp(var_decl->token.value, C_TYPE_FLOAT) == 0 || 
                 strcmp(var_decl->token.value, C_TYPE_DOUBLE) == 0)
        {
            emit_raw("%s%s", element_value, separator);
        }
        else if (strcmp(var_decl->token.value, C_TYPE_CHAR) == 0)
        {
            emit_raw("'%s'%s", element_value, separator);
        }
        else
        {
            emit_raw("%s%s", element_value, separator);
        }
    }
    
    emit_raw(");\n");
    emit_raw("  signal %s : %s_type := %s_init;\n", 
            array_name, array_name, array_name);
}

// -------------------------------------------------------------
// Helper: Emit array signal declaration
// -------------------------------------------------------------
static void emit_array_signal_declaration(ASTNode *var_decl)
{
    char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
    char array_size[ARRAY_SIZE_BUFFER_SIZE] = {0};
    
    if (!parse_array_dimensions(var_decl->value, array_name, array_size, 
                                ARRAY_NAME_BUFFER_SIZE, ARRAY_SIZE_BUFFER_SIZE))
    {
        return;
    }
    
    int array_element_count = 0;
    if (!safe_strtoi(array_size, &array_element_count))
    {
        log_error(ERROR_CATEGORY_CODEGEN, 0,
                  "Invalid array size '%s'", array_size);
        return;
    }
    array_element_count -= 1;
    const char *vhdl_element_type = ctype_to_vhdl(var_decl->token.value);
    
    emit_raw("  type %s_type is array (0 to %d) of %s;\n", 
            array_name, array_element_count, vhdl_element_type);
    emit_raw("  signal %s : %s_type;\n", array_name, array_name);
    
    // Check for array initializer
    int has_initializer = (var_decl->num_children > 0 && 
                      var_decl->children[FIRST_CHILD_INDEX]->value != NULL &&
                      strcmp(var_decl->children[FIRST_CHILD_INDEX]->value, ARRAY_INIT_MARKER) == 0);
    
    if (has_initializer)
    {
        ASTNode *initializer_list = var_decl->children[FIRST_CHILD_INDEX];
        emit_array_initializer_constant(var_decl, initializer_list, array_name);
    }
}

// -------------------------------------------------------------
// Helper: Emit simple (non-array, non-struct) signal declaration
// -------------------------------------------------------------
static void emit_simple_signal_declaration(const ASTNode *var_decl)
{
    int is_result_variable = (strcmp(var_decl->value, RESERVED_PORT_NAME_RESULT) == 0);
    
    if (is_result_variable)
    {
        emit_raw("  signal ");
        emit_raw("%s%s", var_decl->value, SIGNAL_SUFFIX_LOCAL);
        emit_raw(" : %s;\n", ctype_to_vhdl(var_decl->token.value));
    }
    else
    {
        emit_raw("  signal %s : %s;\n", 
                var_decl->value, ctype_to_vhdl(var_decl->token.value));
    }
}

// -------------------------------------------------------------
// Helper: Process a single variable declaration for signal emission
// -------------------------------------------------------------
static void process_variable_declaration_for_signals(ASTNode *var_decl)
{
    int struct_index = find_struct_index(var_decl->token.value);
    
    if (struct_index >= 0)
    {
        emit_struct_signal_declaration(var_decl);
        return;
    }
    
    const char *array_bracket = (var_decl->value != NULL) ? strchr(var_decl->value, '[') : NULL;
    
    if (array_bracket != NULL)
    {
        emit_array_signal_declaration(var_decl);
    }
    else
    {
        emit_simple_signal_declaration(var_decl);
    }
}

// -------------------------------------------------------------
// Helper: Process variable declarations inside for loops
// -------------------------------------------------------------
static void process_for_loop_declarations(ASTNode *for_statement)
{
    for (int child_index = 0; child_index < for_statement->num_children; ++child_index)
    {
        const ASTNode *for_child = for_statement->children[child_index];
        
        if (for_child->type != NODE_VAR_DECL)
        {
            continue;
        }
        
        const char *array_bracket = (for_child->value != NULL) ? strchr(for_child->value, '[') : NULL;
        
        if (array_bracket != NULL)
        {
            char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
            char array_size[ARRAY_SIZE_BUFFER_SIZE] = {0};
            if (parse_array_dimensions(for_child->value, array_name, array_size,
                                      ARRAY_NAME_BUFFER_SIZE, ARRAY_SIZE_BUFFER_SIZE))
            {
                int array_element_count = 0;
                const char *vhdl_element_type = NULL;
                if (!safe_strtoi(array_size, &array_element_count))
                {
                    continue;
                }
                array_element_count -= 1;
                vhdl_element_type = ctype_to_vhdl(for_child->token.value);
                
                emit_raw("  type %s_type is array (0 to %d) of %s;\n", 
                        array_name, array_element_count, vhdl_element_type);
                emit_raw("  signal %s : %s_type;\n", array_name, array_name);
            }
        }
        else
        {
            emit_raw("  signal %s : %s;\n", 
                    for_child->value, ctype_to_vhdl(for_child->token.value));
        }
    }
}

// Walk the function body to declare VHDL signals for every C local variable.
// Structs become record-typed signals, arrays become array-typed signals,
// and for-loop variables need declarations hoisted to architecture scope.
void emit_function_local_signals(ASTNode *function_declaration)
{
    // Iterate through function children to find statement blocks
    for (int child_index = 0; child_index < function_declaration->num_children; ++child_index)
    {
        ASTNode *child = function_declaration->children[child_index];
        
        if (child->type != NODE_STATEMENT)
        {
            continue;
        }
        
        // Process each child within the statement block
        for (int statement_child_index = 0; 
             statement_child_index < child->num_children; 
             ++statement_child_index)
        {
            ASTNode *statement_child = child->children[statement_child_index];
            
            if (statement_child->type == NODE_VAR_DECL)
            {
                process_variable_declaration_for_signals(statement_child);
            }
            else if (statement_child->type == NODE_FOR_STATEMENT)
            {
                process_for_loop_declarations(statement_child);
            }
        }
    }
}

// All supported C types map to std_logic_vector, so the same default applies
static const char* get_vhdl_default_value(const char *c_type)
{
    (void)c_type;
    return "(others => '0')";
}

// Reset each local signal to zero on hardware reset; arrays, structs, and
// simple signals each need different VHDL reset syntax.
// Uses the emitter's indentation rather than a manual indent string.
static void emit_reset_assignment(const ASTNode *var_decl)
{
    const char *var_name = var_decl->value;
    if (var_name == NULL) {
        return;
    }

    // Array reset: use VHDL aggregate "(others => '0')"
    const char *bracket = strchr(var_name, '[');
    if (bracket != NULL) {
        char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
        size_t name_len = (size_t)(bracket - var_name);
        if (name_len >= ARRAY_NAME_BUFFER_SIZE) {
            name_len = ARRAY_NAME_BUFFER_SIZE - 1;
        }
        strncpy(array_name, var_name, name_len);
        array_name[name_len] = '\0';
        emit_line("%s <= (others => %s);", array_name,
                  get_vhdl_default_value(var_decl->token.value));
        return;
    }

    // Struct reset: each field must be zeroed individually
    int struct_index = find_struct_index(var_decl->token.value);
    if (struct_index >= 0) {
        const StructInfo *info = get_struct_info(struct_index);
        if (info != NULL) {
            emit_line("-- Reset struct %s", var_name);
            for (int i = 0; i < info->field_count; ++i) {
                emit_line("%s.%s <= %s;", var_name,
                          info->fields[i].field_name,
                          get_vhdl_default_value(info->fields[i].field_type));
            }
        }
        return;
    }

    // Simple signal reset (with reserved name remapping)
    if (strcmp(var_name, RESERVED_PORT_NAME_RESULT) == 0) {
        emit_line("%s%s <= %s;", var_name, SIGNAL_SUFFIX_LOCAL,
                  get_vhdl_default_value(var_decl->token.value));
    } else {
        emit_line("%s <= %s;", var_name,
                  get_vhdl_default_value(var_decl->token.value));
    }
}

// Emit reset assignments for variable declarations inside a for loop
static void emit_for_loop_reset_assignments(ASTNode *for_statement)
{
    for (int i = 0; i < for_statement->num_children; ++i) {
        const ASTNode *child = for_statement->children[i];
        if (child->type == NODE_VAR_DECL) {
            emit_reset_assignment(child);
        }
    }
}

// Walk the function body to emit reset assignments for every declared signal.
// Must mirror emit_function_local_signals() to ensure all signals are reset.
void emit_function_reset_logic(ASTNode *function_declaration)
{
    for (int child_index = 0; child_index < function_declaration->num_children; ++child_index)
    {
        ASTNode *child = function_declaration->children[child_index];
        if (child->type != NODE_STATEMENT) {
            continue;
        }

        for (int i = 0; i < child->num_children; ++i)
        {
            ASTNode *statement_child = child->children[i];
            if (statement_child->type == NODE_VAR_DECL) {
                emit_reset_assignment(statement_child);
            } else if (statement_child->type == NODE_FOR_STATEMENT) {
                emit_for_loop_reset_assignments(statement_child);
            }
        }
    }
}

// VHDL Code Generator - Helper Utilities Implementation
// -------------------------------------------------------------

#include "codegen_vhdl_helpers.h"
#include "codegen_vhdl_constants.h"
#include "symbol_structs.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// -------------------------------------------------------------
// Helper function to check if a variable name needs remapping
// Returns 1 if the variable name conflicts with reserved VHDL port names
// -------------------------------------------------------------
int is_signal_name_reserved(const char *variable_name)
{
    if (variable_name == NULL)
    {
        return 0;
    }
    
    return (strcmp(variable_name, RESERVED_PORT_NAME_RESULT) == 0);
}

// -------------------------------------------------------------
// Helper function to emit a potentially remapped signal name
// Writes the signal name to output, adding '_local' suffix if needed
// -------------------------------------------------------------
void emit_mapped_signal_name(const char *variable_name, FILE *output_file)
{
    if (is_signal_name_reserved(variable_name))
    {
        fprintf(output_file, "%s%s", variable_name, SIGNAL_SUFFIX_LOCAL);
    }
    else
    {
        const char *name_to_emit = (variable_name != NULL) ? variable_name : UNKNOWN_IDENTIFIER;
        fprintf(output_file, "%s", name_to_emit);
    }
}

// -------------------------------------------------------------
// Check if operator is a boolean comparison operator
// -------------------------------------------------------------
int is_boolean_comparison_operator(const char *operator)
{
    if (operator == NULL)
    {
        return 0;
    }

    return (strcmp(operator, OP_EQUAL) == 0 ||
            strcmp(operator, OP_NOT_EQUAL) == 0 ||
            strcmp(operator, OP_LESS_THAN) == 0 ||
            strcmp(operator, OP_LESS_EQUAL) == 0 ||
            strcmp(operator, OP_GREATER_THAN) == 0 ||
            strcmp(operator, OP_GREATER_EQUAL) == 0 ||
            strcmp(operator, OP_LOGICAL_AND) == 0 ||
            strcmp(operator, OP_LOGICAL_OR) == 0);
}

// -------------------------------------------------------------
// Check if AST node is a boolean expression
// -------------------------------------------------------------
int is_node_boolean_expression(ASTNode *node)
{
    if (node == NULL)
    {
        return 0;
    }
    
    if (node->type == NODE_BINARY_EXPR && node->value != NULL)
    {
        return is_boolean_comparison_operator(node->value);
    }
    
    if (node->type == NODE_BINARY_OP && 
        node->value != NULL && 
        strcmp(node->value, OP_LOGICAL_NOT) == 0)
    {
        return 1;
    }
    
    return 0;
}

// -------------------------------------------------------------
// Check if expression is a plain identifier (no array/struct access)
// -------------------------------------------------------------
int is_plain_identifier(const char *expression_value)
{
    const char *character_ptr = NULL;
    
    if (expression_value == NULL)
    {
        return 0;
    }
    
    // Check for array access brackets or struct field dots
    for (character_ptr = expression_value; *character_ptr != '\0'; ++character_ptr)
    {
        if (*character_ptr == '[' || *character_ptr == ']' || *character_ptr == '.')
        {
            return 0;
        }
    }
    
    // Check for double underscore (struct field encoding)
    if (strstr(expression_value, "__") != NULL)
    {
        return 0;
    }
    
    return 1;
}

// -------------------------------------------------------------
// Numeric literal detection utilities
// -------------------------------------------------------------

/**
 * Check if a string value represents a numeric literal
 * @param value String to check
 * @return 1 if numeric (digits and optional decimal point), 0 otherwise
 */
int is_numeric_literal(const char *value)
{
    const char *char_ptr = value;
    
    if (value == NULL || *value == '\0')
    {
        return 0;
    }
    
    // Check each character
    while (*char_ptr != '\0')
    {
        if (!isdigit(*char_ptr) && *char_ptr != '.')
        {
            return 0;
        }
        ++char_ptr;
    }
    
    return 1;
}

/**
 * Check if a string value represents a negative numeric literal
 * @param value String to check
 * @return 1 if it starts with '-' followed by digits, 0 otherwise
 */
int is_negative_numeric_literal(const char *value)
{
    if (value == NULL || value[0] != '-' || value[1] == '\0')
    {
        return 0;
    }
    
    // Check if what follows '-' is numeric
    return is_numeric_literal(value + 1);
}

// -------------------------------------------------------------
// Type conversion utilities
// -------------------------------------------------------------

/**
 * Emit VHDL unsigned cast for a literal value
 * @param value The value to cast (must be numeric)
 * @param output_file Output stream
 */
void emit_unsigned_cast(const char *value, FILE *output_file)
{
    fprintf(output_file, "to_unsigned(%s, %d)", value, VHDL_BIT_WIDTH);
}

/**
 * Emit VHDL signed cast for a literal value
 * @param value The value to cast (must be numeric)
 * @param output_file Output stream
 */
void emit_signed_cast(const char *value, FILE *output_file)
{
    fprintf(output_file, "to_signed(%s, %d)", value, VHDL_BIT_WIDTH);
}

/**
 * Emit a typed operand with appropriate VHDL casting
 * Handles both literal values and complex expressions
 * @param operand AST node representing the operand
 * @param output_file Output stream
 * @param is_signed 1 if operand should be treated as signed, 0 for unsigned
 * @param node_generator Function pointer to generate nested nodes
 */
void emit_typed_operand(ASTNode *operand, FILE *output_file, int is_signed, void (*node_generator)(ASTNode*, FILE*))
{
    if (operand == NULL)
    {
        fprintf(output_file, "0");
        return;
    }
    
    // Handle expression nodes with literal values
    if (operand->type == NODE_EXPRESSION && operand->value != NULL)
    {
        if (is_negative_numeric_literal(operand->value))
        {
            emit_signed_cast(operand->value, output_file);
        }
        else if (is_numeric_literal(operand->value))
        {
            if (is_signed)
            {
                emit_signed_cast(operand->value, output_file);
            }
            else
            {
                emit_unsigned_cast(operand->value, output_file);
            }
        }
        else
        {
            // Variable or expression - wrap in unsigned/signed cast
            fprintf(output_file, "%s(", is_signed ? "signed" : "unsigned");
            fprintf(output_file, "%s", operand->value);
            fprintf(output_file, ")");
        }
    }
    else
    {
        // Complex expression - recursively generate and wrap
        fprintf(output_file, "%s(", is_signed ? "signed" : "unsigned");
        if (node_generator != NULL)
        {
            node_generator(operand, output_file);
        }
        fprintf(output_file, ")");
    }
}

// -------------------------------------------------------------
// Array utilities
// -------------------------------------------------------------

/**
 * Parse array declaration to extract name and size
 * @param value String like "arr[10]"
 * @param name_out Buffer to store extracted name
 * @param name_size Size of name_out buffer
 * @param size_out Pointer to store extracted size
 * @return 1 if successfully parsed, 0 otherwise
 */
int parse_array_declaration(const char *value, char *name_out, int name_size, int *size_out)
{
    const char *bracket_pos = NULL;
    const char *size_start = NULL;
    const char *size_end = NULL;
    int name_length = 0;
    char size_buffer[ARRAY_SIZE_BUFFER_SIZE] = {0};
    
    if (value == NULL || name_out == NULL || size_out == NULL)
    {
        return 0;
    }
    
    bracket_pos = strchr(value, '[');
    if (bracket_pos == NULL)
    {
        return 0;
    }
    
    // Extract array name
    name_length = (int)(bracket_pos - value);
    if (name_length >= name_size)
    {
        name_length = name_size - 1;
    }
    strncpy(name_out, value, name_length);
    name_out[name_length] = '\0';
    
    // Extract array size
    size_start = bracket_pos + 1;
    size_end = strchr(size_start, ']');
    
    if (size_end == NULL || size_end <= size_start)
    {
        return 0;
    }
    
    strncpy(size_buffer, size_start, size_end - size_start);
    size_buffer[size_end - size_start] = '\0';
    
    *size_out = atoi(size_buffer);
    
    return (*size_out > 0) ? 1 : 0;
}

/**
 * Emit VHDL array type and signal declaration
 * @param name Array name
 * @param type VHDL element type
 * @param size Array size
 * @param output_file Output stream
 */
void emit_array_type_and_signal(const char *name, const char *type, int size, FILE *output_file)
{
    fprintf(output_file, "  type %s_type is array (0 to %d) of %s;\n", name, size - 1, type);
    fprintf(output_file, "  signal %s : %s_type;\n", name, name);
}

// -------------------------------------------------------------
// Statement utilities
// -------------------------------------------------------------

/**
 * Unwrap single-child statement nodes
 * Returns the inner node if it's a NODE_STATEMENT with one child of type
 * NODE_VAR_DECL or NODE_ASSIGNMENT, otherwise returns the original node
 * @param node Node to unwrap
 * @return Unwrapped node or original node
 */
ASTNode* unwrap_statement_node(ASTNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    
    if (node->type == NODE_STATEMENT && 
        node->num_children == 1 &&
        (node->children[0]->type == NODE_VAR_DECL || 
         node->children[0]->type == NODE_ASSIGNMENT))
    {
        return node->children[0];
    }
    
    return node;
}

// -------------------------------------------------------------
// Struct field utilities
// -------------------------------------------------------------

/**
 * Emit field-by-field assignments between two struct instances
 * @param struct_index Index in g_structs array
 * @param target_name Name of target struct variable
 * @param source_name Name of source struct variable
 * @param output_file Output stream
 * @param indentation Indentation string for each line
 */
void emit_struct_field_assignments(int struct_index, const char *target_name, 
                                   const char *source_name, FILE *output_file, 
                                   const char *indentation)
{
    int field_index = 0;
    
    if (struct_index < 0 || target_name == NULL || source_name == NULL)
    {
        return;
    }
    
    for (field_index = 0; field_index < g_structs[struct_index].field_count; ++field_index)
    {
        fprintf(output_file, "%s%s.%s <= %s.%s;\n", 
                indentation,
                target_name,
                g_structs[struct_index].fields[field_index].field_name,
                source_name,
                g_structs[struct_index].fields[field_index].field_name);
    }
}



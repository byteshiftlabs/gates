// VHDL Code Generator - Expression Generation Implementation
// -------------------------------------------------------------

#include "codegen_vhdl_expressions.h"
#include "codegen_vhdl_constants.h"
#include "codegen_vhdl_helpers.h"
#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Forward declarations for mutual recursion
static void generate_node(ASTNode *node, FILE *output_file);

// -------------------------------------------------------------
// Binary expression (both arithmetic and comparison)
// -------------------------------------------------------------
void generate_binary_expression(ASTNode *node, FILE *output_file)
{
    const char *operator = node->value;
    const char *normalized_operator = operator;
    ASTNode *left_operand  = node->children[FIRST_CHILD_INDEX];
    ASTNode *right_operand = node->children[FIRST_CHILD_INDEX + 1];

    // Normalize operator for VHDL
    if (strcmp(operator, OP_EQUAL) == 0)
    {
        normalized_operator = VHDL_OP_EQUAL;
    }
    else if (strcmp(operator, OP_NOT_EQUAL) == 0)
    {
        normalized_operator = VHDL_OP_NOT_EQUAL;
    }

    // Logical short-circuit operators (&&, ||) converted to boolean expressions
    if (strcmp(node->value, OP_LOGICAL_AND) == 0 || strcmp(node->value, OP_LOGICAL_OR) == 0)
    {
        const char *vhdl_logical = (strcmp(node->value, OP_LOGICAL_AND) == 0) ? VHDL_OP_AND : VHDL_OP_OR;
        emit_boolean_gate_expression(left_operand, right_operand, vhdl_logical, output_file);
        return;
    }

    // Comparison operations produce booleans
    if (strcmp(normalized_operator, VHDL_OP_EQUAL) == 0 || 
        strcmp(normalized_operator, VHDL_OP_NOT_EQUAL) == 0 ||
        strcmp(normalized_operator, OP_LESS_THAN) == 0 ||
        strcmp(normalized_operator, OP_LESS_EQUAL) == 0 ||
        strcmp(normalized_operator, OP_GREATER_THAN) == 0 ||
        strcmp(normalized_operator, OP_GREATER_EQUAL) == 0)
    {
        // Emit left operand with type conversion
        emit_typed_operand(left_operand, output_file, 0, generate_node);
        
        fprintf(output_file, " %s ", normalized_operator);
        
        // Emit right operand with type conversion
        emit_typed_operand(right_operand, output_file, 0, generate_node);
        
        return;
    }

    // Bitwise operations
    if (strcmp(operator, OP_BITWISE_AND) == 0)
    {
        fprintf(output_file, "unsigned(");
        generate_node(left_operand, output_file);
        fprintf(output_file, ") and unsigned(");
        generate_node(right_operand, output_file);
        fprintf(output_file, ")");
        return;
    }
    
    if (strcmp(operator, OP_BITWISE_OR) == 0)
    {
        fprintf(output_file, "unsigned(");
        generate_node(left_operand, output_file);
        fprintf(output_file, ") or unsigned(");
        generate_node(right_operand, output_file);
        fprintf(output_file, ")");
        return;
    }
    
    if (strcmp(operator, OP_BITWISE_XOR) == 0)
    {
        fprintf(output_file, "unsigned(");
        generate_node(left_operand, output_file);
        fprintf(output_file, ") xor unsigned(");
        generate_node(right_operand, output_file);
        fprintf(output_file, ")");
        return;
    }
    
    if (strcmp(operator, OP_SHIFT_LEFT) == 0)
    {
        fprintf(output_file, "shift_left(unsigned(");
        generate_node(left_operand, output_file);
        fprintf(output_file, "), to_integer(unsigned(");
        generate_node(right_operand, output_file);
        fprintf(output_file, "))))");
        return;
    }
    
    if (strcmp(operator, OP_SHIFT_RIGHT) == 0)
    {
        fprintf(output_file, "shift_right(unsigned(");
        generate_node(left_operand, output_file);
        fprintf(output_file, "), to_integer(unsigned(");
        generate_node(right_operand, output_file);
        fprintf(output_file, "))))");
        return;
    }

    // Fallback: arithmetic or unknown operators
    generate_node(left_operand, output_file);
    fprintf(output_file, " %s ", operator);
    generate_node(right_operand, output_file);
}

// -------------------------------------------------------------
// Expression (identifier / literal / array access / struct field)
// -------------------------------------------------------------
void generate_expression(ASTNode *node, FILE *output_file)
{
    if (node->value == NULL)
    {
        fprintf(output_file, "%s", UNKNOWN_IDENTIFIER);
        return;
    }

    // Array element like name[index]
    if (strchr(node->value, '[') != NULL)
    {
        emit_array_element_access(node->value, output_file);
        return;
    }

    if (is_negative_literal(node->value))
    {
        if (isalpha(node->value[1]) || node->value[1] == '_')
        {
            fprintf(output_file, "-unsigned(%s)", node->value + 1);
        }
        else
        {
            emit_signed_cast(node->value, output_file);
        }
        return;
    }

    // Struct field encoded as a__b -> a.b
    if (strstr(node->value, "__") != NULL)
    {
        char buffer[MAX_BUFFER_SIZE];
        char *char_ptr = NULL;
        
        strncpy(buffer, node->value, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        
        for (char_ptr = buffer; *char_ptr != '\0'; ++char_ptr)
        {
            if (*char_ptr == '_' && *(char_ptr + 1) == '_')
            {
                *char_ptr = '.';
                memmove(char_ptr + 1, char_ptr + 2, strlen(char_ptr + 2) + 1);
            }
        }
        
        fprintf(output_file, "%s", buffer);
        return;
    }

    // Use mapped signal name for variables
    emit_mapped_signal_name(node->value, output_file);
}

// -------------------------------------------------------------
// Unary operations (stored as NODE_BINARY_OP w/ value '!','~')
// -------------------------------------------------------------
void generate_unary_operation(ASTNode *node, FILE *output_file)
{
    ASTNode *inner_expression = NULL;

    if (node->value == NULL || node->num_children != 1)
    {
        fprintf(output_file, "-- unsupported unary op");
        return;
    }

    inner_expression = node->children[FIRST_CHILD_INDEX];

    if (strcmp(node->value, OP_LOGICAL_NOT) == 0)
    {
        if (is_node_boolean_expression(inner_expression))
        {
            fprintf(output_file, "not (");
            generate_node(inner_expression, output_file);
            fprintf(output_file, ")");
        }
        else
        {
            fprintf(output_file, "(unsigned(");
            generate_node(inner_expression, output_file);
            fprintf(output_file, ") = 0)");
        }
    }
    else if (strcmp(node->value, OP_BITWISE_NOT) == 0)
    {
        fprintf(output_file, "not unsigned(");
        generate_node(inner_expression, output_file);
        fprintf(output_file, ")");
    }
    else
    {
        fprintf(output_file, "-- unsupported unary op");
    }
}

// -------------------------------------------------------------
// Function call generation
// -------------------------------------------------------------
void generate_function_call(ASTNode *node, FILE *output_file)
{
    int argument_index = 0;
    
    if (node == NULL || node->value == NULL)
    {
        fprintf(output_file, "-- Error: unknown function call");
        return;
    }
    
    // Emit function name
    fprintf(output_file, "%s(", node->value);
    
    // Emit comma-separated arguments
    for (argument_index = 0; argument_index < node->num_children; argument_index++)
    {
        if (argument_index > 0)
        {
            fprintf(output_file, ", ");
        }
        
        generate_node(node->children[argument_index], output_file);
    }
    
    fprintf(output_file, ")");
}

// -------------------------------------------------------------
// Helper: Emit array element access
// -------------------------------------------------------------
void emit_array_element_access(const char *array_expression, FILE *output_file)
{
    char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
    char array_index[ARRAY_INDEX_BUFFER_SIZE] = {0};
    const char *left_bracket = NULL;
    const char *index_start = NULL;
    const char *index_end = NULL;
    int name_length = 0;
    int index_length = 0;

    if (array_expression == NULL)
    {
        fprintf(output_file, "-- Invalid array ref");
        return;
    }

    left_bracket = strchr(array_expression, '[');

    if (left_bracket == NULL)
    {
        fprintf(output_file, "%s", array_expression);
        return;
    }

    name_length = (int)(left_bracket - array_expression);
    strncpy(array_name, array_expression, name_length);
    array_name[name_length] = '\0';
    
    index_start = left_bracket + 1;
    index_end = strchr(index_start, ']');

    if (index_end != NULL && index_end > index_start)
    {
        index_length = (int)(index_end - index_start);
        strncpy(array_index, index_start, index_length);
        array_index[index_length] = '\0';
        
        fprintf(output_file, "%s(%s)", array_name, array_index);
    }
    else
    {
        fprintf(output_file, "-- Invalid array index");
    }
}

// -------------------------------------------------------------
// Helper: Emit conditional expression
// -------------------------------------------------------------
void emit_conditional_expression(ASTNode *condition, FILE *output_file)
{
    if (condition == NULL)
    {
        fprintf(output_file, "(%s)", VHDL_FALSE);
        return;
    }
    
    if (condition->type == NODE_BINARY_EXPR)
    {
        if (is_boolean_comparison_operator(condition->value))
        {
            generate_node(condition, output_file);
        }
        else
        {
            fprintf(output_file, "unsigned(");
            generate_node(condition, output_file);
            fprintf(output_file, ") /= 0");
        }
    }
    else if (condition->type == NODE_BINARY_OP)
    {
        generate_node(condition, output_file);
    }
    else if (condition->type == NODE_EXPRESSION && condition->value != NULL)
    {
        fprintf(output_file, "unsigned(%s) /= 0", condition->value);
    }
    else
    {
        const char *condition_value = (condition->value != NULL) ? condition->value : VHDL_FALSE;
        fprintf(output_file, "(%s)", condition_value);
    }
}

// -------------------------------------------------------------
// Helper: Emit boolean gate expression (AND/OR)
// -------------------------------------------------------------
void emit_boolean_gate_expression(ASTNode *left_operand, ASTNode *right_operand, 
                                  const char *logical_operator, FILE *output_file)
{
    fprintf(output_file, "(");
    
    if (is_node_boolean_expression(left_operand))
    {
        fprintf(output_file, "(");
        generate_node(left_operand, output_file);
        fprintf(output_file, ")");
    }
    else
    {
        fprintf(output_file, "unsigned(");
        generate_node(left_operand, output_file);
        fprintf(output_file, ") /= 0");
    }
    
    fprintf(output_file, "%s", logical_operator);
    
    if (is_node_boolean_expression(right_operand))
    {
        fprintf(output_file, "(");
        generate_node(right_operand, output_file);
        fprintf(output_file, ")");
    }
    else
    {
        fprintf(output_file, "unsigned(");
        generate_node(right_operand, output_file);
        fprintf(output_file, ") /= 0");
    }
    
    fprintf(output_file, ")");
}

// -------------------------------------------------------------
// Minimal node dispatcher for mutual recursion
// -------------------------------------------------------------
static void generate_node(ASTNode *node, FILE *output_file)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
        case NODE_BINARY_EXPR:
            generate_binary_expression(node, output_file);
            break;
            
        case NODE_BINARY_OP:
            generate_unary_operation(node, output_file);
            break;
            
        case NODE_EXPRESSION:
            generate_expression(node, output_file);
            break;
            
        case NODE_FUNC_CALL:
            generate_function_call(node, output_file);
            break;
            
        default:
            // For other node types, defer to main code generator
            break;
    }
}

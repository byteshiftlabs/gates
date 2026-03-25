// VHDL Code Generator - Expression Generation Implementation
// -------------------------------------------------------------

#include <ctype.h>
#include <string.h>

#include "codegen_vhdl_expressions.h"
#include "codegen_vhdl_constants.h"
#include "codegen_vhdl_emit.h"
#include "codegen_vhdl_helpers.h"

// File-scoped helpers (forward declarations for top-down organization)
static void emit_array_element_access(const char *array_expression);
static void emit_boolean_operand(ASTNode *operand, void (*node_generator)(ASTNode*));
static void emit_boolean_gate_expression(ASTNode *left_operand, ASTNode *right_operand,
                                         const char *logical_operator,
                                         void (*node_generator)(ASTNode*));

// Map C operators to VHDL equivalents (==  -> =, !=  -> /=) and dispatch
// bitwise/logical/comparison/arithmetic to VHDL-appropriate emission.
void generate_binary_expression(ASTNode *node, void (*node_generator)(ASTNode*))
{
    if (!node || node->num_children < 2) {
        return;
    }

    const char *op = node->value;
    ASTNode *left_operand  = node->children[FIRST_CHILD_INDEX];
    ASTNode *right_operand = node->children[FIRST_CHILD_INDEX + 1];

    // Normalize C comparison operators to VHDL syntax
    const char *normalized_operator = op;
    if (strcmp(op, OP_EQUAL) == 0) {
        normalized_operator = VHDL_OP_EQUAL;
    } else if (strcmp(op, OP_NOT_EQUAL) == 0) {
        normalized_operator = VHDL_OP_NOT_EQUAL;
    }

    // C short-circuit operators become VHDL boolean gates (no short-circuit in HDL)
    if (strcmp(node->value, OP_LOGICAL_AND) == 0 || strcmp(node->value, OP_LOGICAL_OR) == 0)
    {
        const char *vhdl_logical = (strcmp(node->value, OP_LOGICAL_AND) == 0) ? VHDL_OP_AND : VHDL_OP_OR;
        emit_boolean_gate_expression(left_operand, right_operand, vhdl_logical, node_generator);
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
        emit_typed_operand(left_operand, 0, node_generator);
        
        emit_raw(" %s ", normalized_operator);
        
        // Emit right operand with type conversion
        emit_typed_operand(right_operand, 0, node_generator);
        
        return;
    }

    // Bitwise operations — result is unsigned, wrap in std_logic_vector
    if (strcmp(op, OP_BITWISE_AND) == 0)
    {
        emit_raw("std_logic_vector(unsigned(");
        node_generator(left_operand);
        emit_raw(") and unsigned(");
        node_generator(right_operand);
        emit_raw("))");
        return;
    }
    
    if (strcmp(op, OP_BITWISE_OR) == 0)
    {
        emit_raw("std_logic_vector(unsigned(");
        node_generator(left_operand);
        emit_raw(") or unsigned(");
        node_generator(right_operand);
        emit_raw("))");
        return;
    }
    
    if (strcmp(op, OP_BITWISE_XOR) == 0)
    {
        emit_raw("std_logic_vector(unsigned(");
        node_generator(left_operand);
        emit_raw(") xor unsigned(");
        node_generator(right_operand);
        emit_raw("))");
        return;
    }
    
    if (strcmp(op, OP_SHIFT_LEFT) == 0)
    {
        emit_raw("std_logic_vector(shift_left(unsigned(");
        node_generator(left_operand);
        emit_raw("), to_integer(unsigned(");
        node_generator(right_operand);
        emit_raw(")))))");
        return;
    }
    
    if (strcmp(op, OP_SHIFT_RIGHT) == 0)
    {
        emit_raw("std_logic_vector(shift_right(unsigned(");
        node_generator(left_operand);
        emit_raw("), to_integer(unsigned(");
        node_generator(right_operand);
        emit_raw(")))))");
        return;
    }

    // Fallback: arithmetic — use typed operands and wrap in std_logic_vector
    emit_raw("std_logic_vector(");
    emit_typed_operand(left_operand, 0, node_generator);
    emit_raw(" %s ", op);
    emit_typed_operand(right_operand, 0, node_generator);
    emit_raw(")");
}

// Map C identifiers, array accesses, struct fields, and literals to VHDL signals.
// Each category requires different VHDL syntax, so dispatch by value pattern.
void generate_expression(ASTNode *node)
{
    if (node->value == NULL)
    {
        emit_raw("%s", UNKNOWN_IDENTIFIER);
        return;
    }

    // Array element like name[index]
    if (strchr(node->value, '[') != NULL)
    {
        emit_array_element_access(node->value);
        return;
    }

    if (is_negative_literal(node->value))
    {
        if (isalpha(node->value[1]) || node->value[1] == '_')
        {
            emit_raw("std_logic_vector(0 - unsigned(%s))", node->value + 1);
        }
        else
        {
            emit_raw("std_logic_vector(");
            emit_signed_cast(node->value);
            emit_raw(")");
        }
        return;
    }

    // Positive numeric literals: emit as std_logic_vector(to_unsigned(N, BIT_WIDTH))
    if (is_numeric_literal(node->value))
    {
        emit_raw("std_logic_vector(");
        emit_unsigned_cast(node->value);
        emit_raw(")");
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
        
        emit_raw("%s", buffer);
        return;
    }

    // Use mapped signal name for variables
    emit_mapped_signal_name(node->value);
}

// VHDL has no unary operator syntax like C; map ! to boolean test and ~ to bitwise not
void generate_unary_operation(ASTNode *node, void (*node_generator)(ASTNode*))
{
    if (node->value == NULL || node->num_children != 1)
    {
        emit_raw("-- unsupported unary op");
        return;
    }

    ASTNode *inner_expression = node->children[FIRST_CHILD_INDEX];

    if (strcmp(node->value, OP_LOGICAL_NOT) == 0)
    {
        if (is_node_boolean_expression(inner_expression))
        {
            emit_raw("not (");
            node_generator(inner_expression);
            emit_raw(")");
        }
        else
        {
            emit_raw("(unsigned(");
            node_generator(inner_expression);
            emit_raw(") = 0)");
        }
    }
    else if (strcmp(node->value, OP_BITWISE_NOT) == 0)
    {
        emit_raw("std_logic_vector(not unsigned(");
        node_generator(inner_expression);
        emit_raw("))");
    }
    else
    {
        emit_raw("-- unsupported unary op");
    }
}

// VHDL function calls have the same parenthesized syntax as C
void generate_function_call(ASTNode *node, void (*node_generator)(ASTNode*))
{
    if (node == NULL || node->value == NULL)
    {
        emit_raw("-- Error: unknown function call");
        return;
    }
    
    // Emit function name
    emit_raw("%s(", node->value);
    
    // Emit comma-separated arguments
    for (int argument_index = 0; argument_index < node->num_children; argument_index++)
    {
        if (argument_index > 0)
        {
            emit_raw(", ");
        }
        
        node_generator(node->children[argument_index]);
    }
    
    emit_raw(")");
}

// VHDL uses parenthesized syntax array_name(index) instead of C's array_name[index]
static void emit_array_element_access(const char *array_expression)
{
    if (array_expression == NULL)
    {
        emit_raw("-- Invalid array ref");
        return;
    }

    const char *left_bracket = strchr(array_expression, '[');

    if (left_bracket == NULL)
    {
        emit_raw("%s", array_expression);
        return;
    }

    char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
    int name_length = (int)(left_bracket - array_expression);
    strncpy(array_name, array_expression, (size_t)name_length);
    array_name[name_length] = '\0';
    
    const char *index_start = left_bracket + 1;
    const char *index_end = strchr(index_start, ']');

    if (index_end != NULL && index_end > index_start)
    {
        char array_index[ARRAY_INDEX_BUFFER_SIZE] = {0};
        int index_length = (int)(index_end - index_start);
        strncpy(array_index, index_start, (size_t)index_length);
        array_index[index_length] = '\0';
        
        emit_raw("%s(%s)", array_name, array_index);
    }
    else
    {
        emit_raw("-- Invalid array index");
    }
}

// Wrap non-boolean conditions in a VHDL boolean test (C treats any non-zero as true)
void emit_conditional_expression(ASTNode *condition, void (*node_generator)(ASTNode*))
{
    if (condition == NULL)
    {
        emit_raw("(%s)", VHDL_FALSE);
        return;
    }
    
    if (condition->type == NODE_BINARY_EXPR)
    {
        if (is_boolean_comparison_operator(condition->value))
        {
            node_generator(condition);
        }
        else
        {
            emit_raw("unsigned(");
            node_generator(condition);
            emit_raw(") /= 0");
        }
    }
    else if (condition->type == NODE_BINARY_OP)
    {
        node_generator(condition);
    }
    else if (condition->type == NODE_EXPRESSION && condition->value != NULL)
    {
        emit_raw("unsigned(%s) /= 0", condition->value);
    }
    else
    {
        const char *condition_value = (condition->value != NULL) ? condition->value : VHDL_FALSE;
        emit_raw("(%s)", condition_value);
    }
}

// Emit a single operand in boolean context: boolean expressions pass through,
// non-boolean expressions get compared against zero (C truthiness semantics).
static void emit_boolean_operand(ASTNode *operand, void (*node_generator)(ASTNode*))
{
    if (is_node_boolean_expression(operand))
    {
        emit_raw("(");
        node_generator(operand);
        emit_raw(")");
    }
    else
    {
        emit_raw("unsigned(");
        node_generator(operand);
        emit_raw(") /= 0");
    }
}

// Emit a VHDL boolean gate (AND/OR) from two C operands
static void emit_boolean_gate_expression(ASTNode *left_operand, ASTNode *right_operand, 
                                  const char *logical_operator,
                                  void (*node_generator)(ASTNode*))
{
    emit_raw("(");
    emit_boolean_operand(left_operand, node_generator);
    emit_raw("%s", logical_operator);
    emit_boolean_operand(right_operand, node_generator);
    emit_raw(")");
}
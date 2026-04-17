// VHDL Code Generator - Statement Generation Implementation
// -------------------------------------------------------------

#include "codegen_vhdl_statements.h"
#include "codegen_vhdl_constants.h"
#include "codegen_vhdl_helpers.h"
#include "codegen_vhdl_expressions.h"
#include "symbol_structs.h"
#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// -------------------------------------------------------------
// Statement block generation
// -------------------------------------------------------------
void generate_statement_block(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*))
{
    int child_index = 0;

    for (child_index = 0; child_index < node->num_children; ++child_index)
    {
        ASTNode *child = node->children[child_index];

        switch (child->type)
        {
            case NODE_VAR_DECL:
            {
                char *array_bracket = (child->value != NULL) ? strchr(child->value, '[') : NULL;
                int struct_index = find_struct_index(child->token.value);
                int is_struct = (struct_index >= 0);
                
                if (child->num_children > 0 && array_bracket == NULL && is_struct)
                {
                    ASTNode *initializer = child->children[FIRST_CHILD_INDEX];
                    emit_struct_field_initializations(child, struct_index, initializer, output_file, node_generator);
                }
                else if (child->num_children > 0 && array_bracket == NULL)
                {
                    emit_variable_initializer(child, output_file, INDENT_LEVEL_3, node_generator);
                }
                break;
            }
            
            case NODE_ASSIGNMENT:
                emit_variable_assignment(child, output_file, INDENT_LEVEL_3, node_generator);
                break;
                
            case NODE_IF_STATEMENT:
            case NODE_WHILE_STATEMENT:
            case NODE_FOR_STATEMENT:
            case NODE_BREAK_STATEMENT:
            case NODE_CONTINUE_STATEMENT:
                node_generator(child, output_file);
                break;

            case NODE_EXPRESSION:
                emit_expression_as_return(child, node, output_file, node_generator);
                break;
                
            case NODE_BINARY_EXPR:
            case NODE_BINARY_OP:
                fprintf(output_file, "%sresult <= ", INDENT_LEVEL_3);
                node_generator(child, output_file);
                fprintf(output_file, ";\n");
                break;
                
            default:
                // Intentionally ignored node types
                break;
        }
    }
}

// -------------------------------------------------------------
// While loop generation
// -------------------------------------------------------------
void generate_while_loop(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*))
{
    int statement_index = FIRST_STATEMENT_INDEX;
    ASTNode *condition = node->children[FIRST_CHILD_INDEX];
    
    fprintf(output_file, "%swhile ", INDENT_LEVEL_3);
    emit_conditional_expression(condition, output_file);
    fprintf(output_file, " loop\n");
    
    for (statement_index = FIRST_STATEMENT_INDEX; statement_index < node->num_children; ++statement_index)
    {
        node_generator(node->children[statement_index], output_file);
    }
    
    fprintf(output_file, "%send loop;\n", INDENT_LEVEL_3);
}

// -------------------------------------------------------------
// For loop generation (converted to VHDL while loop)
// -------------------------------------------------------------
void generate_for_loop(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*))
{
    int condition_index = 0;
    int statement_index = 0;
    int increment_index = -1;
    ASTNode *first_child = NULL;
    ASTNode *condition = NULL;
    ASTNode *increment = NULL;

    if (node->num_children == 0)
    {
        return;
    }

    first_child = node->children[FIRST_CHILD_INDEX];

    // Unwrap single-child statement nodes
    first_child = unwrap_statement_node(first_child);

    // Emit initialization (if present)
    if (first_child->type == NODE_ASSIGNMENT || first_child->type == NODE_VAR_DECL)
    {
        if (first_child->type == NODE_ASSIGNMENT && first_child->num_children == 2)
        {
            emit_variable_assignment(first_child, output_file, INDENT_LEVEL_3, node_generator);
        }
        else if (first_child->type == NODE_VAR_DECL && first_child->num_children > 0)
        {
            emit_variable_initializer(first_child, output_file, INDENT_LEVEL_3, node_generator);
        }
        
        condition_index = 1;
    }

    if (condition_index >= node->num_children)
    {
        return;
    }

    condition = node->children[condition_index];
    
    // Find increment statement
    increment_index = node->num_children - 1;
    
    if (node->children[increment_index]->type == NODE_ASSIGNMENT && 
        increment_index != condition_index)
    {
        increment = node->children[increment_index];
    }
    else
    {
        increment_index = -1;
    }

    // Emit while loop with condition
    fprintf(output_file, "%swhile ", INDENT_LEVEL_3);
    emit_conditional_expression(condition, output_file);
    fprintf(output_file, " loop\n");

    // Emit loop body statements (excluding increment)
    for (statement_index = condition_index + 1; 
         statement_index < node->num_children; 
         ++statement_index)
    {
        if (statement_index == increment_index)
        {
            continue; // Skip increment, emit it at the end
        }
        
        node_generator(node->children[statement_index], output_file);
    }

    // Emit increment at end of loop body
    if (increment != NULL && increment->num_children == 2)
    {
        emit_variable_assignment(increment, output_file, INDENT_LEVEL_4, node_generator);
    }

    fprintf(output_file, "%send loop;\n", INDENT_LEVEL_3);
}

// -------------------------------------------------------------
// If / ElseIf / Else statement generation
// -------------------------------------------------------------
void generate_if_statement(ASTNode *node, FILE *output_file, void (*node_generator)(ASTNode*, FILE*))
{
    int branch_index = 0;
    int statement_index = 0;
    ASTNode *condition = node->children[FIRST_CHILD_INDEX];

    fprintf(output_file, "%sif ", INDENT_LEVEL_3);
    emit_conditional_expression(condition, output_file);
    fprintf(output_file, " then\n");

    for (branch_index = FIRST_STATEMENT_INDEX; 
         branch_index < node->num_children; 
         ++branch_index)
    {
        ASTNode *branch = node->children[branch_index];
        
        if (branch->type == NODE_ELSE_IF_STATEMENT)
        {
            ASTNode *elseif_condition = branch->children[FIRST_CHILD_INDEX];
            
            fprintf(output_file, "%selsif ", INDENT_LEVEL_3);
            emit_conditional_expression(elseif_condition, output_file);
            fprintf(output_file, " then\n");
            
            for (statement_index = FIRST_STATEMENT_INDEX; 
                 statement_index < branch->num_children; 
                 ++statement_index)
            {
                node_generator(branch->children[statement_index], output_file);
            }
        }
        else if (branch->type == NODE_ELSE_STATEMENT)
        {
            fprintf(output_file, "%selse\n", INDENT_LEVEL_3);
            
            for (statement_index = 0; 
                 statement_index < branch->num_children; 
                 ++statement_index)
            {
                node_generator(branch->children[statement_index], output_file);
            }
        }
        else
        {
            node_generator(branch, output_file);
        }
    }
    
    fprintf(output_file, "%send if;\n", INDENT_LEVEL_3);
}

// -------------------------------------------------------------
// Break statement generation
// -------------------------------------------------------------
void generate_break_statement(ASTNode *node, FILE *output_file)
{
    (void)node; // Suppress unused parameter warning
    fprintf(output_file, "%sexit;\n", INDENT_LEVEL_3);
}

// -------------------------------------------------------------
// Continue statement generation
// -------------------------------------------------------------
void generate_continue_statement(ASTNode *node, FILE *output_file)
{
    (void)node; // Suppress unused parameter warning
    fprintf(output_file, "%snext;\n", INDENT_LEVEL_3);
}

// -------------------------------------------------------------
// Helper: Emit variable initializer
// -------------------------------------------------------------
void emit_variable_initializer(ASTNode *declaration, FILE *output_file, const char *indentation, void (*node_generator)(ASTNode*, FILE*))
{
    ASTNode *initializer = NULL;

    if (declaration == NULL || declaration->num_children == 0)
    {
        return;
    }

    initializer = declaration->children[FIRST_CHILD_INDEX];
    fprintf(output_file, "%s", indentation);
    emit_mapped_signal_name(declaration->value, output_file);
    fprintf(output_file, " <= ");
    node_generator(initializer, output_file);
    fprintf(output_file, ";\n");
}

// -------------------------------------------------------------
// Helper: Emit variable assignment
// -------------------------------------------------------------
void emit_variable_assignment(ASTNode *assignment, FILE *output_file, const char *indentation, void (*node_generator)(ASTNode*, FILE*))
{
    ASTNode *left_hand_side = NULL;
    ASTNode *right_hand_side = NULL;
    char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
    char array_index[ARRAY_INDEX_BUFFER_SIZE] = {0};
    const char *left_bracket = NULL;
    const char *index_start = NULL;
    const char *index_end = NULL;
    int name_length = 0;

    if (assignment == NULL || assignment->num_children != 2)
    {
        return;
    }
    
    left_hand_side = assignment->children[FIRST_CHILD_INDEX];
    right_hand_side = assignment->children[FIRST_CHILD_INDEX + 1];

    fprintf(output_file, "%s", indentation);

    if (left_hand_side->value != NULL && strchr(left_hand_side->value, '[') != NULL)
    {
        // Array element assignment
        left_bracket = strchr(left_hand_side->value, '[');
        
        if (left_bracket != NULL)
        {
            name_length = (int)(left_bracket - left_hand_side->value);
            strncpy(array_name, left_hand_side->value, name_length);
            array_name[name_length] = '\0';
            
            index_start = left_bracket + 1;
            index_end = strchr(index_start, ']');
            
            if (index_end != NULL && index_end > index_start)
            {
                int index_length = (int)(index_end - index_start);
                strncpy(array_index, index_start, index_length);
                array_index[index_length] = '\0';
                
                fprintf(output_file, "%s(%s) <= ", array_name, array_index);
                node_generator(right_hand_side, output_file);
                fprintf(output_file, ";\n");
                return;
            }
        }
        
        fprintf(output_file, "-- Invalid array index\n");
        return;
    }

    fprintf(output_file, "%s", indentation);
    emit_mapped_signal_name(left_hand_side->value, output_file);
    fprintf(output_file, " <= ");
    node_generator(right_hand_side, output_file);
    fprintf(output_file, ";\n");
}

// -------------------------------------------------------------
// Helper: Emit struct field initializations
// -------------------------------------------------------------
void emit_struct_field_initializations(ASTNode *var_decl, int struct_index, 
                                       ASTNode *initializer, FILE *output_file, void (*node_generator)(ASTNode*, FILE*))
{
    int field_index = 0;
    
    if (initializer != NULL && 
        initializer->value != NULL && 
        strcmp(initializer->value, STRUCT_INIT_MARKER) == 0)
    {
        for (field_index = 0; field_index < g_structs[struct_index].field_count; ++field_index)
        {
            const char *field_name = g_structs[struct_index].fields[field_index].field_name;
            const char *field_value = (field_index < initializer->num_children) ? 
                                      initializer->children[field_index]->value : DEFAULT_ZERO_VALUE;
            
            if (strcmp(g_structs[struct_index].fields[field_index].field_type, C_TYPE_INT) == 0)
            {
                if (is_numeric_literal(field_value) || is_negative_numeric_literal(field_value))
                {
                    fprintf(output_file, "%s%s.%s <= ", 
                            INDENT_LEVEL_3, var_decl->value, field_name);
                    emit_unsigned_cast(field_value, output_file);
                    fprintf(output_file, ";\n");
                }
                else
                {
                    fprintf(output_file, "%s%s.%s <= %s;\n", 
                            INDENT_LEVEL_3, var_decl->value, field_name, field_value);
                }
            }
            else
            {
                fprintf(output_file, "%s%s.%s <= %s;\n", 
                        INDENT_LEVEL_3, var_decl->value, field_name, field_value);
            }
        }
    }
    else
    {
        const char *var_name = (var_decl->value != NULL) ? var_decl->value : UNKNOWN_IDENTIFIER;
        fprintf(output_file, "%s%s <= ", INDENT_LEVEL_3, var_name);
        node_generator(initializer, output_file);
        fprintf(output_file, ";\n");
    }
}

// -------------------------------------------------------------
// Helper: Handle expression as function return value
// -------------------------------------------------------------
void emit_expression_as_return(ASTNode *expression, ASTNode *parent_statement, 
                               FILE *output_file, void (*node_generator)(ASTNode*, FILE*))
{
    int is_struct_return_type = 0;
    const char *struct_return_name = NULL;
    
    if (parent_statement->parent != NULL && 
        parent_statement->parent->type == NODE_FUNCTION_DECL)
    {
        struct_return_name = parent_statement->parent->token.value;
        is_struct_return_type = (struct_return_name != NULL && 
                                 find_struct_index(struct_return_name) >= 0);
    }
    
    int is_plain = is_plain_identifier(expression->value);
    
    if (is_struct_return_type && expression->value != NULL && is_plain)
    {
        emit_struct_field_copy_to_result(expression, parent_statement->parent, 
                                        output_file, INDENT_LEVEL_3);
    }
    else
    {
        fprintf(output_file, "%sresult <= ", INDENT_LEVEL_3);
        
        if (expression->value != NULL && is_negative_numeric_literal(expression->value))
        {
            if (isalpha(expression->value[1]) || expression->value[1] == '_')
            {
                fprintf(output_file, "-unsigned(%s)", expression->value + 1);
            }
            else
            {
                emit_signed_cast(expression->value, output_file);
            }
        }
        else
        {
            node_generator(expression, output_file);
        }
        
        fprintf(output_file, ";\n");
    }
}

// -------------------------------------------------------------
// Helper: Copy struct fields to result port
// -------------------------------------------------------------
void emit_struct_field_copy_to_result(ASTNode *expression, ASTNode *function_node, 
                                      FILE *output_file, const char *indentation)
{
    const char *struct_return_name = NULL;
    int struct_index = 0;

    if (function_node == NULL || expression == NULL || expression->value == NULL)
    {
        return;
    }

    struct_return_name = function_node->token.value;
    struct_index = find_struct_index(struct_return_name);

    if (struct_index < 0)
    {
        return;
    }

    emit_struct_field_assignments(struct_index, "result", expression->value, 
                                  output_file, indentation);
}

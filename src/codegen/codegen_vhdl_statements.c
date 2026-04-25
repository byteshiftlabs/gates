// VHDL Code Generator - Statement Generation Implementation
// -------------------------------------------------------------

#include <ctype.h>
#include <string.h>

#include "codegen_vhdl_statements.h"
#include "codegen_vhdl_constants.h"
#include "codegen_vhdl_emit.h"
#include "codegen_vhdl_helpers.h"
#include "codegen_vhdl_expressions.h"
#include "error_handler.h"
#include "symbol_structs.h"

// File-scoped helpers (forward declarations for top-down organization)
static void emit_variable_initializer(ASTNode *declaration,
                                      void (*node_generator)(ASTNode*));
static void emit_variable_assignment(ASTNode *assignment,
                                     void (*node_generator)(ASTNode*));
static void emit_array_element_assignment(const ASTNode *left_hand_side,
                                          ASTNode *right_hand_side,
                                          void (*node_generator)(ASTNode*));
static void emit_struct_field_initializations(ASTNode *var_decl, int struct_index,
                                              ASTNode *initializer,
                                              void (*node_generator)(ASTNode*));
static void emit_expression_as_return(ASTNode *expression, ASTNode *parent_statement,
                                      void (*node_generator)(ASTNode*));
static void emit_struct_field_copy_to_result(const ASTNode *expression, ASTNode *function_node);

// Dispatch each child statement to its VHDL equivalent.
// Struct initializations need field-by-field emission because VHDL records
// cannot be assigned from aggregate literals in the same way as C structs.
void generate_statement_block(ASTNode *node, void (*node_generator)(ASTNode*))
{
    for (int child_index = 0; child_index < node->num_children; ++child_index)
    {
        ASTNode *child = node->children[child_index];

        switch (child->type)
        {
            case NODE_VAR_DECL:
            {
                const char *array_bracket = (child->value != NULL) ? strchr(child->value, '[') : NULL;
                int struct_index = find_struct_index(child->token.value);
                int is_struct = (struct_index >= 0);
                
                if (child->num_children > 0 && array_bracket == NULL && is_struct)
                {
                    ASTNode *initializer = child->children[FIRST_CHILD_INDEX];
                    emit_struct_field_initializations(child, struct_index, initializer, node_generator);
                }
                else if (child->num_children > 0 && array_bracket == NULL)
                {
                    emit_variable_initializer(child, node_generator);
                }
                break;
            }
            
            case NODE_ASSIGNMENT:
                emit_variable_assignment(child, node_generator);
                break;
                
            case NODE_IF_STATEMENT:
            case NODE_WHILE_STATEMENT:
            case NODE_FOR_STATEMENT:
            case NODE_BREAK_STATEMENT:
            case NODE_CONTINUE_STATEMENT:
                node_generator(child);
                break;

            case NODE_EXPRESSION:
                emit_expression_as_return(child, node, node_generator);
                break;
                
            case NODE_BINARY_EXPR:
            case NODE_BINARY_OP:
                emit_expression_as_return(child, node, node_generator);
                break;
                
            default:
                // Intentionally ignored node types
                break;
        }
    }
}

// VHDL while loop maps directly from C while
void generate_while_loop(ASTNode *node, void (*node_generator)(ASTNode*))
{
    ASTNode *condition = node->children[FIRST_CHILD_INDEX];
    
    emit_indented("while ");
    emit_conditional_expression(condition, node_generator);
    emit_raw(" loop\n");
    
    emit_indent_inc();
    for (int statement_index = FIRST_STATEMENT_INDEX; statement_index < node->num_children; ++statement_index)
    {
        node_generator(node->children[statement_index]);
    }
    
    emit_indent_dec();
    emit_line("end loop;");
}

// C for loops have no direct VHDL equivalent; decompose into
// init assignment + while loop + trailing increment.
void generate_for_loop(ASTNode *node, void (*node_generator)(ASTNode*))
{
    if (node->num_children == 0)
    {
        return;
    }

    ASTNode *first_child = node->children[FIRST_CHILD_INDEX];
    int condition_index = 0;

    // Unwrap single-child statement nodes
    first_child = unwrap_statement_node(first_child);

    // Emit initialization (if present)
    if (first_child->type == NODE_ASSIGNMENT || first_child->type == NODE_VAR_DECL)
    {
        if (first_child->type == NODE_ASSIGNMENT && first_child->num_children == 2)
        {
            emit_variable_assignment(first_child, node_generator);
        }
        else if (first_child->type == NODE_VAR_DECL && first_child->num_children > 0)
        {
            emit_variable_initializer(first_child, node_generator);
        }
        
        condition_index = 1;
    }

    if (condition_index >= node->num_children)
    {
        return;
    }

    ASTNode *condition = node->children[condition_index];
    
    // Find increment statement
    int increment_index = node->num_children - 1;
    ASTNode *increment = NULL;
    
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
    emit_indented("while ");
    emit_conditional_expression(condition, node_generator);
    emit_raw(" loop\n");

    emit_indent_inc();
    
    // Emit loop body statements (excluding increment)
    for (int statement_index = condition_index + 1; 
         statement_index < node->num_children; 
         ++statement_index)
    {
        if (statement_index == increment_index)
        {
            continue; // Skip increment, emit it at the end
        }
        
        node_generator(node->children[statement_index]);
    }

    // Emit increment at end of loop body
    if (increment != NULL && increment->num_children == 2)
    {
        emit_variable_assignment(increment, node_generator);
    }

    emit_indent_dec();
    emit_line("end loop;");
}

// VHDL if/elsif/else maps closely to C, but requires 'then' and 'end if'
void generate_if_statement(ASTNode *node, void (*node_generator)(ASTNode*))
{
    ASTNode *condition = node->children[FIRST_CHILD_INDEX];

    emit_indented("if ");
    emit_conditional_expression(condition, node_generator);
    emit_raw(" then\n");

    emit_indent_inc();
    
    for (int branch_index = FIRST_STATEMENT_INDEX; 
         branch_index < node->num_children; 
         ++branch_index)
    {
        ASTNode *branch = node->children[branch_index];
        
        if (branch->type == NODE_ELSE_IF_STATEMENT)
        {
            ASTNode *elseif_condition = branch->children[FIRST_CHILD_INDEX];
            
            emit_indent_dec();
            emit_indented("elsif ");
            emit_conditional_expression(elseif_condition, node_generator);
            emit_raw(" then\n");
            emit_indent_inc();
            
            for (int statement_index = FIRST_STATEMENT_INDEX; 
                 statement_index < branch->num_children; 
                 ++statement_index)
            {
                node_generator(branch->children[statement_index]);
            }
        }
        else if (branch->type == NODE_ELSE_STATEMENT)
        {
            emit_indent_dec();
            emit_line("else");
            emit_indent_inc();
            
            for (int statement_index = 0; 
                 statement_index < branch->num_children; 
                 ++statement_index)
            {
                node_generator(branch->children[statement_index]);
            }
        }
        else
        {
            node_generator(branch);
        }
    }
    
    emit_indent_dec();
    emit_line("end if;");
}

// VHDL 'exit' is the equivalent of C 'break' inside a loop
// Parameter unused but required by the node_generator callback signature
void generate_break_statement(ASTNode *node)
{
    (void)node;
    emit_line("exit;");
}

// VHDL 'next' is the equivalent of C 'continue' inside a loop
// Parameter unused but required by the node_generator callback signature
void generate_continue_statement(ASTNode *node)
{
    (void)node;
    emit_line("next;");
}

// Simple variable init: emit signal_name <= initial_value
static void emit_variable_initializer(ASTNode *declaration,
                                      void (*node_generator)(ASTNode*))
{
    if (declaration == NULL || declaration->num_children == 0)
    {
        return;
    }

    ASTNode *initializer = declaration->children[FIRST_CHILD_INDEX];
    emit_indent();
    emit_mapped_signal_name(declaration->value);
    emit_raw(" <= ");
    node_generator(initializer);
    emit_raw(";\n");
}

// Emit VHDL array element assignment: C's a[i] = expr becomes a(i) <= expr
static void emit_array_element_assignment(const ASTNode *left_hand_side,
                                          ASTNode *right_hand_side,
                                          void (*node_generator)(ASTNode*))
{
    const char *left_bracket = strchr(left_hand_side->value, '[');
    if (left_bracket == NULL) {
        emit_raw("-- Invalid array index\n");
        return;
    }

    char array_name[ARRAY_NAME_BUFFER_SIZE] = {0};
    int name_length = (int)(left_bracket - left_hand_side->value);
    strncpy(array_name, left_hand_side->value, (size_t)name_length);
    array_name[name_length] = '\0';

    const char *index_start = left_bracket + 1;
    const char *index_end = strchr(index_start, ']');

    if (index_end != NULL && index_end > index_start) {
        char array_index[ARRAY_INDEX_BUFFER_SIZE] = {0};
        int index_length = (int)(index_end - index_start);
        strncpy(array_index, index_start, (size_t)index_length);
        array_index[index_length] = '\0';

        emit_raw("%s(%s) <= ", array_name, array_index);
        node_generator(right_hand_side);
        emit_raw(";\n");
        return;
    }

    emit_raw("-- Invalid array index\n");
}

// Emit VHDL signal assignment from C assignment node
static void emit_variable_assignment(ASTNode *assignment,
                                     void (*node_generator)(ASTNode*))
{
    if (assignment == NULL || assignment->num_children != 2) {
        return;
    }

    const ASTNode *left_hand_side = assignment->children[FIRST_CHILD_INDEX];
    ASTNode *right_hand_side = assignment->children[FIRST_CHILD_INDEX + 1];

    emit_indent();

    if (left_hand_side->value != NULL && strchr(left_hand_side->value, '[') != NULL) {
        emit_array_element_assignment(left_hand_side, right_hand_side, node_generator);
        return;
    }

    emit_mapped_signal_name(left_hand_side->value);
    emit_raw(" <= ");
    node_generator(right_hand_side);
    emit_raw(";\n");
}

// VHDL records require field-by-field assignment; C's aggregate init
// (struct_init marker) is expanded into individual field assignments.
static void emit_struct_field_initializations(ASTNode *var_decl, int struct_index, 
                                       ASTNode *initializer, void (*node_generator)(ASTNode*))
{
    if (initializer != NULL && 
        initializer->value != NULL && 
        strcmp(initializer->value, STRUCT_INIT_MARKER) == 0)
    {
        const StructInfo *struct_info = get_struct_info(struct_index);
        if (!struct_info) {
            log_error(ERROR_CATEGORY_CODEGEN, 0, "Invalid struct index %d", struct_index);
            return;
        }
        for (int field_index = 0; field_index < struct_info->field_count; ++field_index)
        {
            const char *field_name = struct_info->fields[field_index].field_name;
            const char *field_value = (field_index < initializer->num_children) ? 
                                      initializer->children[field_index]->value : DEFAULT_ZERO_VALUE;
            
            if (strcmp(struct_info->fields[field_index].field_type, C_TYPE_INT) == 0)
            {
                if (is_numeric_literal(field_value) || is_negative_numeric_literal(field_value))
                {
                    emit_indented("%s.%s <= ", var_decl->value, field_name);
                    emit_unsigned_cast(field_value);
                    emit_raw(";\n");
                }
                else
                {
                    emit_line("%s.%s <= %s;", var_decl->value, field_name, field_value);
                }
            }
            else
            {
                emit_line("%s.%s <= %s;", var_decl->value, field_name, field_value);
            }
        }
    }
    else
    {
        const char *var_name = (var_decl->value != NULL) ? var_decl->value : UNKNOWN_IDENTIFIER;
        emit_indented("%s <= ", var_name);
        node_generator(initializer);
        emit_raw(";\n");
    }
}

// In VHDL, function output is a port signal; C's implicit return-by-expression
// maps to assigning the result port, with special handling for struct returns.
static void emit_expression_as_return(ASTNode *expression, ASTNode *parent_statement, 
                               void (*node_generator)(ASTNode*))
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
        emit_struct_field_copy_to_result(expression, parent_statement->parent);
    }
    else if (is_node_boolean_expression(expression))
    {
        emit_indented("if ");
        emit_conditional_expression(expression, node_generator);
        emit_raw(" then\n");
        emit_indent_inc();
        emit_line("result <= std_logic_vector(to_unsigned(1, %d));", VHDL_BIT_WIDTH);
        emit_indent_dec();
        emit_line("else");
        emit_indent_inc();
        emit_line("result <= std_logic_vector(to_unsigned(0, %d));", VHDL_BIT_WIDTH);
        emit_indent_dec();
        emit_line("end if;");
    }
    else
    {
        emit_indented("result <= ");
        
        if (expression->value != NULL && is_negative_numeric_literal(expression->value))
        {
            if (isalpha(expression->value[1]) || expression->value[1] == '_')
            {
                emit_raw("-unsigned(%s)", expression->value + 1);
            }
            else
            {
                emit_signed_cast(expression->value);
            }
        }
        else
        {
            node_generator(expression);
        }
        
        emit_raw(";\n");
    }
}

// Struct return requires field-by-field copy because VHDL record ports
// cannot be assigned from a local record in one statement.
static void emit_struct_field_copy_to_result(const ASTNode *expression, ASTNode *function_node)
{
    if (function_node == NULL || expression == NULL || expression->value == NULL)
    {
        return;
    }

    const char *struct_return_name = function_node->token.value;
    int struct_index = find_struct_index(struct_return_name);

    if (struct_index < 0)
    {
        return;
    }

    emit_struct_field_assignments(struct_index, "result", expression->value);
}

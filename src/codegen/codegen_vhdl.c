// Readable/refactored variant of codegen_vhdl.c
// -------------------------------------------------------------
// Purpose: Provide the same VHDL generation behavior found in
//          codegen_vhdl.c while improving readability, structure,
//          and maintainability. This file intentionally keeps
//          logic equivalent (best effort) but organizes code into
//          smaller helpers, adds comments, and reduces repetition.
//
// NOTE: This introduces a new public entry point:
//          void generate_vhdl_readable(ASTNode* root, FILE* out);
//       Original symbols (generate_vhdl / generate_vhdl_node) are
//       NOT redefined here to avoid duplicate symbol conflicts if
//       both files are compiled. To swap implementations, either
//       (a) rename in build system, or (b) update the header to
//       point generate_vhdl to this implementation.
// -------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "codegen_vhdl.h"      // ASTNode definition and external helpers
#include "symbol_structs.h"     // g_structs / g_struct_count
#include "symbol_arrays.h"
#include "utils.h"              // is_negative_literal
#include "parse_expression.h"   // ctype_to_vhdl, etc.

// -------------------------------------------------------------
// Internal helper function declarations
// -------------------------------------------------------------
static void gen_node(ASTNode *node, FILE *out);
static void gen_program(ASTNode *node, FILE *out);
static void gen_function(ASTNode *node, FILE *out);
static void gen_statement(ASTNode *node, FILE *out);
static void gen_while(ASTNode *node, FILE *out);
static void gen_for(ASTNode *node, FILE *out);
static void gen_if(ASTNode *node, FILE *out);
static void gen_break(ASTNode *node, FILE *out);
static void gen_continue(ASTNode *node, FILE *out);
static void gen_binary_expr(ASTNode *node, FILE *out);
static void gen_expression(ASTNode *node, FILE *out);
static int needs_signal_remapping(const char *var_name);
static void emit_signal_name(const char *var_name, FILE *out);
static void gen_unary_op(ASTNode *node, FILE *out);
static void gen_func_call(ASTNode *node, FILE *out);

static int  is_bool_comparison(const char *op);
static int  node_is_boolean(ASTNode *node);
static void emit_initializer(ASTNode *decl, FILE *out, const char *indent);
static void emit_assignment(ASTNode *assign, FILE *out, const char *indent);
static void emit_array_element(const char *value, FILE *out);
static void emit_condition(ASTNode *cond, FILE *out);
static void emit_boolean_gate(ASTNode *left, ASTNode *right, const char *logical, FILE *out);
static void emit_struct_declarations(FILE *out);
static void emit_local_signals(ASTNode *function_decl, FILE *out);
static void emit_struct_return_copy(ASTNode *expr, ASTNode *function_stmt_node, FILE *out, const char *indent);

// -------------------------------------------------------------
// Helper function to check if a variable name needs remapping
// Returns 1 if the variable name conflicts with reserved VHDL port names
// -------------------------------------------------------------
static int needs_signal_remapping(const char *var_name)
{
    return (var_name && strcmp(var_name, "result") == 0);
}

// -------------------------------------------------------------
// Helper function to emit a potentially remapped signal name
// Writes the signal name to output, adding '_local' suffix if needed
// -------------------------------------------------------------
static void emit_signal_name(const char *var_name, FILE *out)
{
    if (needs_signal_remapping(var_name)) {
        fprintf(out, "%s_local", var_name);
    } else {
        fprintf(out, "%s", var_name ? var_name : "unknown");
    }
}

// -------------------------------------------------------------
// Public entry point
// -------------------------------------------------------------
void generate_vhdl(ASTNode *root, FILE *out)
{
    gen_node(root, out);
}

// -------------------------------------------------------------
// Dispatch
// -------------------------------------------------------------
static void gen_node(ASTNode *node, FILE *out)
{

    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:          gen_program(node, out); break;
        case NODE_FUNCTION_DECL:    gen_function(node, out); break;
        case NODE_STATEMENT:        gen_statement(node, out); break;
        case NODE_WHILE_STATEMENT:  gen_while(node, out); break;
        case NODE_FOR_STATEMENT:    gen_for(node, out); break;
        case NODE_IF_STATEMENT:     gen_if(node, out); break;
        case NODE_BREAK_STATEMENT:  gen_break(node, out); break;
        case NODE_CONTINUE_STATEMENT: gen_continue(node, out); break;
        case NODE_BINARY_EXPR:      gen_binary_expr(node, out); break;
        case NODE_BINARY_OP:        gen_unary_op(node, out); break; // unary ops live in BINARY_OP nodes in original parser
        case NODE_EXPRESSION:       gen_expression(node, out); break;
        case NODE_FUNC_CALL:        gen_func_call(node, out); break;
        default: /* intentionally ignored */ break;
    }
}

// -------------------------------------------------------------
// Program (top-level)
// -------------------------------------------------------------
static void gen_program(ASTNode *node, FILE *out)
{

    int child_idx;

    fprintf(out, "-- VHDL generated by compi (readable variant)\n\n");
    fprintf(out, "library IEEE;\n");
    fprintf(out, "use IEEE.STD_LOGIC_1164.ALL;\n");
    fprintf(out, "use IEEE.NUMERIC_STD.ALL;\n\n");

    emit_struct_declarations(out);

    for (child_idx = 0; child_idx < node->num_children; ++child_idx) {
        gen_node(node->children[child_idx], out);
    }

}

// -------------------------------------------------------------
// Function declaration -> entity + architecture
// -------------------------------------------------------------
static void gen_function(ASTNode *node, FILE *out)
{

    const char *function_name = node->value ? node->value : "anon";
    ASTNode *params[128] = {0};
    int param_count = 0;
    int child_idx = 0;

    fprintf(out, "-- Function: %s\n", function_name);
    fprintf(out, "entity %s is\n", function_name);
    fprintf(out, "  port (\n");
    fprintf(out, "    clk   : in  std_logic;\n");
    fprintf(out, "    reset : in  std_logic;\n");

    // Collect parameters (var decl children at top level)
    for (child_idx = 0; child_idx < node->num_children; ++child_idx) {
        ASTNode *child_node = node->children[child_idx];
        if (child_node->type == NODE_VAR_DECL) params[param_count++] = child_node;
    }

    for (child_idx = 0; child_idx < param_count; ++child_idx) {
        ASTNode *parameter = params[child_idx];
        int is_struct = find_struct_index(parameter->token.value) >= 0;
        if (is_struct) {
            fprintf(out, "    %s : in %s_t;\n", parameter->value, parameter->token.value);
        } else {
            fprintf(out, "    %s : in %s;\n", parameter->value, ctype_to_vhdl(parameter->token.value));
        }
    }

    // Return port
    if (node->token.value && strlen(node->token.value) > 0) {

        if (find_struct_index(node->token.value) >= 0) {
            fprintf(out, "    result : out %s_t\n", node->token.value);
        } else {
            fprintf(out, "    result : out %s\n", ctype_to_vhdl(node->token.value));
        }

    } else {

        fprintf(out, "    result : out std_logic_vector(31 downto 0)\n");

    }
    fprintf(out, "  );\nend entity;\n\n");

    // Architecture
    fprintf(out, "architecture behavioral of %s is\n", function_name);
    emit_local_signals(node, out);
    fprintf(out, "begin\n");
    fprintf(out, "  process(clk, reset)\n");
    fprintf(out, "  begin\n");
    fprintf(out, "    if reset = '1' then\n");

    fprintf(out, "      -- Reset logic (user-defined)\n");

    fprintf(out, "    elsif rising_edge(clk) then\n");

    // Body statements
    for (child_idx = 0; child_idx < node->num_children; ++child_idx) {
        ASTNode *child = node->children[child_idx];
        if (child->type == NODE_STATEMENT) gen_node(child, out);
    }

    fprintf(out, "    end if;\n");
    fprintf(out, "  end process;\n");
    fprintf(out, "end architecture;\n\n");

}

// -------------------------------------------------------------
// Statement block
// -------------------------------------------------------------
static void gen_statement(ASTNode *node, FILE *out)
{

    int child_idx = 0;
    for (child_idx = 0; child_idx < node->num_children; ++child_idx) {

        ASTNode *child = node->children[child_idx];

        switch (child->type) {

            case NODE_VAR_DECL: {
                // Handle struct init or simple init
                char *arr_bracket = child->value ? strchr(child->value, '[') : NULL;
                int struct_idx = find_struct_index(child->token.value);
                if (child->num_children > 0 && !arr_bracket && struct_idx >= 0) {
                    ASTNode *init = child->children[0];
                    if (init && init->value && strcmp(init->value, "struct_init") == 0) {
                        for (int field_index = 0; field_index < g_structs[struct_idx].field_count; ++field_index) {
                            const char *field = g_structs[struct_idx].fields[field_index].field_name;
                            const char *val = (field_index < init->num_children) ? init->children[field_index]->value : "0";
                            if (strcmp(g_structs[struct_idx].fields[field_index].field_type, "int") == 0) {
                                if (isdigit(val[0]) || (val[0] == '-' && isdigit(val[1]))) {
                                    fprintf(out, "      %s.%s <= to_unsigned(%s, 32);\n", child->value, field, val);
                                } else {
                                    fprintf(out, "      %s.%s <= %s;\n", child->value, field, val);
                                }
                            } else {
                                fprintf(out, "      %s.%s <= %s;\n", child->value, field, val);
                            }
                        }
                    } else {
                        fprintf(out, "      %s <= ", child->value ? child->value : "unknown");
                        gen_node(init, out);
                        fprintf(out, ";\n");
                    }
                } else if (child->num_children > 0 && !arr_bracket) {
                    emit_initializer(child, out, "      ");
                }
                break; }
            case NODE_ASSIGNMENT:
                emit_assignment(child, out, "      ");
                break;
            case NODE_IF_STATEMENT:
            case NODE_WHILE_STATEMENT:
            case NODE_FOR_STATEMENT:
            case NODE_BREAK_STATEMENT:
            case NODE_CONTINUE_STATEMENT:
                gen_node(child, out);
                break;

            case NODE_EXPRESSION: {
                // Expression acting as function result
                int is_struct_ret = 0;
                const char *struct_ret_name = NULL;
                if (node->parent && node->parent->type == NODE_FUNCTION_DECL) {
                    struct_ret_name = node->parent->token.value;
                    is_struct_ret = struct_ret_name && find_struct_index(struct_ret_name) >= 0;
                }
                int plain_ident = 1;
                if (child->value) {
                    for (const char *p = child->value; *p; ++p) {
                        if (*p == '[' || *p == ']' || *p == '.') { plain_ident = 0; break; }
                    }
                    if (strstr(child->value, "__")) plain_ident = 0;
                } else plain_ident = 0;

                if (is_struct_ret && child->value && plain_ident) {
                    emit_struct_return_copy(child, node->parent, out, "      ");
                } else {
                    fprintf(out, "      result <= ");
                    if (child->value && child->value[0] == '-' && strlen(child->value) > 1) {
                        if (isalpha(child->value[1]) || child->value[1] == '_') {
                            fprintf(out, "-unsigned(%s)", child->value + 1);
                        } else {
                            fprintf(out, "to_signed(%s, 32)", child->value);
                        }
                    } else {
                        gen_node(child, out);
                    }
                    fprintf(out, ";\n");
                }
                break; }
            case NODE_BINARY_EXPR:
            case NODE_BINARY_OP:
                fprintf(out, "      result <= ");
                gen_node(child, out);
                fprintf(out, ";\n");
                break;
            default:
                break; // ignore
        }
    }
}

// -------------------------------------------------------------
// While loop
// -------------------------------------------------------------
static void gen_while(ASTNode *node, FILE *out)
{

    ASTNode *cond = node->children[0];
    fprintf(out, "      while ");
    emit_condition(cond, out);
    fprintf(out, " loop\n");
    for (int stmt_idx = 1; stmt_idx < node->num_children; ++stmt_idx) {
        gen_node(node->children[stmt_idx], out);
    }
    fprintf(out, "      end loop;\n");
}

// -------------------------------------------------------------
// For loop rewritten as while (mirrors original logic)
// -------------------------------------------------------------
static void gen_for(ASTNode *node, FILE *out)
{

    if (node->num_children == 0) return;

    int cond_index = 0;
    ASTNode *first = node->children[0];

    // Legacy AST normalization
    if (first->type == NODE_STATEMENT && first->num_children == 1 &&
        (first->children[0]->type == NODE_VAR_DECL || first->children[0]->type == NODE_ASSIGNMENT)) {
        first = first->children[0];
    }

    if (first->type == NODE_ASSIGNMENT || first->type == NODE_VAR_DECL) {
        if (first->type == NODE_ASSIGNMENT && first->num_children == 2) {
            emit_assignment(first, out, "      ");
        } else if (first->type == NODE_VAR_DECL && first->num_children > 0) {
            emit_initializer(first, out, "      ");
        }
        cond_index = 1;
    }

    if (cond_index >= node->num_children) return;

    ASTNode *cond = node->children[cond_index];
    int incr_index = node->num_children - 1;
    ASTNode *incr = NULL;
    if (node->children[incr_index]->type == NODE_ASSIGNMENT && incr_index != cond_index) {
        incr = node->children[incr_index];
    } else {
        incr_index = -1;
    }

    fprintf(out, "      while ");
    emit_condition(cond, out);
    fprintf(out, " loop\n");

    for (int stmt_idx = cond_index + 1; stmt_idx < node->num_children; ++stmt_idx) {
        if (stmt_idx == incr_index) continue; // skip increment here
        gen_node(node->children[stmt_idx], out);
    }

    if (incr && incr->num_children == 2) {
        emit_assignment(incr, out, "        ");
    }

    fprintf(out, "      end loop;\n");
}

// -------------------------------------------------------------
// If / ElseIf / Else
// -------------------------------------------------------------
static void gen_if(ASTNode *node, FILE *out)
{

    ASTNode *cond = node->children[0];

    fprintf(out, "      if ");
    emit_condition(cond, out);
    fprintf(out, " then\n");

    for (int branch_idx = 1; branch_idx < node->num_children; ++branch_idx) {
        ASTNode *branch = node->children[branch_idx];
        if (branch->type == NODE_ELSE_IF_STATEMENT) {
            ASTNode *elseif_cond = branch->children[0];
            fprintf(out, "      elsif ");
            emit_condition(elseif_cond, out);
            fprintf(out, " then\n");
            for (int stmt_idx = 1; stmt_idx < branch->num_children; ++stmt_idx) {
                gen_node(branch->children[stmt_idx], out);
            }
        } else if (branch->type == NODE_ELSE_STATEMENT) {
            fprintf(out, "      else\n");
            for (int stmt_idx = 0; stmt_idx < branch->num_children; ++stmt_idx) {
                gen_node(branch->children[stmt_idx], out);
            }
        } else {
            gen_node(branch, out);
        }
    }
    fprintf(out, "      end if;\n");
}

// -------------------------------------------------------------
static void gen_break(ASTNode *node, FILE *out)
{
    (void)node;
    fprintf(out, "      exit;\n");
}

static void gen_continue(ASTNode *node, FILE *out)
{
    (void)node;
    fprintf(out, "      next;\n");
}

// -------------------------------------------------------------
// Binary expression (both arithmetic and comparison)
// -------------------------------------------------------------
static void gen_binary_expr(ASTNode *node, FILE *out)
{

    const char *op = node->value;
    ASTNode *left  = node->children[0];
    ASTNode *right = node->children[1];

    // Normalize op
    if (strcmp(op, "==") == 0) op = "=";
    else if (strcmp(op, "!=") == 0) op = "/=";

    // Logical short-circuit style (&&, ||) converted to boolean expressions
    if (strcmp(node->value, "&&") == 0 || strcmp(node->value, "||") == 0) {
        emit_boolean_gate(left, right, strcmp(node->value, "&&") == 0 ? " and " : " or ", out);
        return;
    }

    // Comparison operations produce booleans
    if (strcmp(op, "=") == 0 || strcmp(op, "/=") == 0 || strcmp(op, "<") == 0 ||
        strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        // Left
        if (left->type == NODE_EXPRESSION && left->value) {
            if (is_negative_literal(left->value)) {
                fprintf(out, "to_signed(%s, 32)", left->value);
            } else {
                int is_num = 1;
                const char *p = left->value;
                if (!*p) {
                    is_num = 0;
                }
                while (*p) {
                    if (!isdigit(*p) && *p != '.') {
                        is_num = 0;
                        break;
                    }
                    ++p;
                }
                if (is_num) {
                    fprintf(out, "to_unsigned(%s, 32)", left->value);
                } else {
                    fprintf(out, "unsigned(%s)", left->value);
                }
            }
        } else {
            fprintf(out, "unsigned("); gen_node(left, out); fprintf(out, ")");
        }
        fprintf(out, " %s ", op);
        // Right
        if (right->type == NODE_EXPRESSION && right->value) {
            if (is_negative_literal(right->value)) {
                fprintf(out, "to_signed(%s, 32)", right->value);
            } else {
                int is_num = 1;
                const char *p = right->value;
                if (!*p) {
                    is_num = 0;
                }
                while (*p) {
                    if (!isdigit(*p) && *p != '.') {
                        is_num = 0;
                        break;
                    }
                    ++p;
                }
                if (is_num) {
                    fprintf(out, "to_unsigned(%s, 32)", right->value);
                } else {
                    fprintf(out, "unsigned(%s)", right->value);
                }
            }
        } else {
            fprintf(out, "unsigned("); gen_node(right, out); fprintf(out, ")");
        }
        return;
    }

    // Bitwise
    if (strcmp(op, "&") == 0) {
        fprintf(out, "unsigned("); gen_node(left, out); fprintf(out, ") and unsigned("); gen_node(right, out); fprintf(out, ")"); return; }
    if (strcmp(op, "|") == 0) {
        fprintf(out, "unsigned("); gen_node(left, out); fprintf(out, ") or unsigned("); gen_node(right, out); fprintf(out, ")"); return; }
    if (strcmp(op, "^") == 0) {
        fprintf(out, "unsigned("); gen_node(left, out); fprintf(out, ") xor unsigned("); gen_node(right, out); fprintf(out, ")"); return; }
    if (strcmp(op, "<<") == 0) {
        fprintf(out, "shift_left(unsigned("); gen_node(left, out); fprintf(out, "), to_integer(unsigned("); gen_node(right, out); fprintf(out, "))))"); return; }
    if (strcmp(op, ">>") == 0) {
        fprintf(out, "shift_right(unsigned("); gen_node(left, out); fprintf(out, "), to_integer(unsigned("); gen_node(right, out); fprintf(out, "))))"); return; }

    // Fallback arithmetic or unknown
    gen_node(left, out);
    fprintf(out, " %s ", op);
    gen_node(right, out);
}

// -------------------------------------------------------------
// Expression (identifier / literal / array access / struct field)
// -------------------------------------------------------------
static void gen_expression(ASTNode *node, FILE *out)
{

    if (!node->value) { 
        fprintf(out, "unknown"); 
        return; 
    }

    // Array element like name[index]
    if (strchr(node->value, '[')) {
        emit_array_element(node->value, out);
        return;
    }

    if (is_negative_literal(node->value)) {
        if (isalpha(node->value[1]) || node->value[1] == '_') {
            fprintf(out, "-unsigned(%s)", node->value + 1);
        } else {
            fprintf(out, "to_signed(%s, 32)", node->value);
        }
        return;
    }

    // Struct field encoded as a__b -> a.b
    if (strstr(node->value, "__")) {
        char buf[256];
        strncpy(buf, node->value, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        for (char *p = buf; *p; ++p) {
            if (*p == '_' && *(p + 1) == '_') {
                *p = '.';
                memmove(p + 1, p + 2, strlen(p + 2) + 1);
            }
        }
        fprintf(out, "%s", buf);
        return;
    }

    // Use mapped signal name for variables
    emit_signal_name(node->value, out);
}

// -------------------------------------------------------------
// Unary operations (stored as NODE_BINARY_OP w/ value '!','~')
// -------------------------------------------------------------
static void gen_unary_op(ASTNode *node, FILE *out)
{

    if (!node->value || node->num_children != 1) { 
        fprintf(out, "-- unsupported unary op"); 
        return; 
    }

    ASTNode *inner = node->children[0];

    if (strcmp(node->value, "!") == 0) {
        if (node_is_boolean(inner)) {
            fprintf(out, "not ("); gen_node(inner, out); fprintf(out, ")");
        } else {
            fprintf(out, "(unsigned("); gen_node(inner, out); fprintf(out, ") = 0)");
        }
    } else if (strcmp(node->value, "~") == 0) {
        fprintf(out, "not unsigned("); gen_node(inner, out); fprintf(out, ")");
    } else {
        fprintf(out, "-- unsupported unary op");
    }
}

// -------------------------------------------------------------
// Helper implementations
// -------------------------------------------------------------
static int is_bool_comparison(const char *op)
{

    if (!op) return 0;

    return (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, "<") == 0 ||
            strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0 ||
            strcmp(op, "&&") == 0 || strcmp(op, "||") == 0);
}

static int node_is_boolean(ASTNode *node)
{

    if (!node) return 0;
    if (node->type == NODE_BINARY_EXPR && node->value) return is_bool_comparison(node->value);
    if (node->type == NODE_BINARY_OP && node->value && strcmp(node->value, "!") == 0) return 1;
    return 0;
}

static void emit_initializer(ASTNode *decl, FILE *out, const char *indent)
{

    if (!decl || decl->num_children == 0) return;

    ASTNode *init = decl->children[0];
    fprintf(out, "%s", indent);
    emit_signal_name(decl->value, out);
    fprintf(out, " <= ");
    gen_node(init, out);
    fprintf(out, ";\n");
}

static void emit_assignment(ASTNode *assign, FILE *out, const char *indent)
{

    if (!assign || assign->num_children != 2) return;
    ASTNode *lhs = assign->children[0];
    ASTNode *rhs = assign->children[1];

    fprintf(out, "%s", indent);

    if (lhs->value && strchr(lhs->value, '[')) {
        // Array element
        char arr_name[64] = {0};
        char arr_idx[64]  = {0};
        const char *lbr = strchr(lhs->value, '[');
        if (lbr) {
            int name_len = (int)(lbr - lhs->value);
            strncpy(arr_name, lhs->value, name_len);
            const char *idx_start = lbr + 1;
            const char *idx_end   = strchr(idx_start, ']');
            if (idx_end && idx_end > idx_start) {
                strncpy(arr_idx, idx_start, idx_end - idx_start);
                fprintf(out, "%s(%s) <= ", arr_name, arr_idx);
                gen_node(rhs, out);
                fprintf(out, ";\n");
                return;
            }
        }
        fprintf(out, "-- Invalid array index\n");
        return;
    }

    emit_signal_name(lhs->value, out);
    fprintf(out, " <= ");
    gen_node(rhs, out);
    fprintf(out, ";\n");
}

static void emit_array_element(const char *value, FILE *out)
{

    if (!value) { 
        fprintf(out, "-- Invalid array ref");
        return; 
    }

    char arr_name[64] = {0};
    char arr_idx[64]  = {0};
    const char *lbr = strchr(value, '[');

    if (!lbr) { 
        fprintf(out, "%s", value); 
        return; 
    }

    int name_len = (int)(lbr - value);
    strncpy(arr_name, value, name_len);
    const char *idx_start = lbr + 1;
    const char *idx_end   = strchr(idx_start, ']');

    if (idx_end && idx_end > idx_start) {
        strncpy(arr_idx, idx_start, idx_end - idx_start);
        fprintf(out, "%s(%s)", arr_name, arr_idx);
    } else {
        fprintf(out, "-- Invalid array index");
    }
}

static void emit_condition(ASTNode *cond, FILE *out)
{
    
    if (!cond) { fprintf(out, "(false)"); return; }
    if (cond->type == NODE_BINARY_EXPR) {
        if (is_bool_comparison(cond->value)) {
            gen_node(cond, out);
        } else {
            fprintf(out, "unsigned("); gen_node(cond, out); fprintf(out, ") /= 0");
        }
    } else if (cond->type == NODE_BINARY_OP) {
        gen_node(cond, out);
    } else if (cond->type == NODE_EXPRESSION && cond->value) {
        fprintf(out, "unsigned(%s) /= 0", cond->value);
    } else {
        fprintf(out, "(%s)", cond->value ? cond->value : "false");
    }
}

static void emit_boolean_gate(ASTNode *left, ASTNode *right, const char *logical, FILE *out)
{
    fprintf(out, "(");
    if (node_is_boolean(left)) {
        fprintf(out, "("); gen_node(left, out); fprintf(out, ")");
    } else {
        fprintf(out, "unsigned("); gen_node(left, out); fprintf(out, ") /= 0");
    }
    fprintf(out, "%s", logical);
    if (node_is_boolean(right)) {
        fprintf(out, "("); gen_node(right, out); fprintf(out, ")");
    } else {
        fprintf(out, "unsigned("); gen_node(right, out); fprintf(out, ") /= 0");
    }
    fprintf(out, ")");
}

static void emit_struct_declarations(FILE *out)
{
    int struct_idx = 0;
    for (struct_idx = 0; struct_idx < g_struct_count; ++struct_idx) {
        StructInfo *struct_info = &g_structs[struct_idx];
        fprintf(out, "-- Struct %s as VHDL record\n", struct_info->name);
        fprintf(out, "type %s_t is record\n", struct_info->name);
        for (int field_index = 0; field_index < struct_info->field_count; ++field_index) {
            fprintf(out, "  %s : %s;\n", struct_info->fields[field_index].field_name, ctype_to_vhdl(struct_info->fields[field_index].field_type));
        }
        fprintf(out, "end record;\n\n");
    }
}

static void emit_local_signals(ASTNode *function_decl, FILE *out)
{
    // Iterate through statements to discover declarations hidden inside
    int child_idx = 0;
    for (child_idx = 0; child_idx < function_decl->num_children; ++child_idx) {
        ASTNode *child = function_decl->children[child_idx];
        if (child->type != NODE_STATEMENT) continue;
        for (int stmt_idx = 0; stmt_idx < child->num_children; ++stmt_idx) {
            ASTNode *stmt_child = child->children[stmt_idx];
            if (stmt_child->type == NODE_VAR_DECL) {
                if (find_struct_index(stmt_child->token.value) >= 0) {
                    fprintf(out, "  signal %s : %s_t;\n", stmt_child->value, stmt_child->token.value);
                    continue;
                }
                char *arr_bracket = stmt_child->value ? strchr(stmt_child->value, '[') : NULL;
                if (arr_bracket) {
                    // Array declaration
                    int name_len = (int)(arr_bracket - stmt_child->value);
                    char arr_name[64] = {0}; strncpy(arr_name, stmt_child->value, name_len);
                    char arr_size[32] = {0};
                    const char *size_start = arr_bracket + 1;
                    const char *size_end = strchr(size_start, ']');
                    if (size_end && size_end > size_start) {
                        strncpy(arr_size, size_start, size_end - size_start);
                        const char *vhdl_elem_type = ctype_to_vhdl(stmt_child->token.value);
                        fprintf(out, "  type %s_type is array (0 to %d) of %s;\n", arr_name, atoi(arr_size) - 1, vhdl_elem_type);
                        fprintf(out, "  signal %s : %s_type;\n", arr_name, arr_name);
                        // Optional array initializers
                        if (stmt_child->num_children > 0 && stmt_child->children[0]->value && strcmp(stmt_child->children[0]->value, "array_init") == 0) {
                            ASTNode *init_list = stmt_child->children[0];
                            fprintf(out, "  -- Array initialization\n");
                            fprintf(out, "  constant %s_init : %s_type := (", arr_name, arr_name);
                            for (int k = 0; k < init_list->num_children; ++k) {
                                const char *val = init_list->children[k]->value;
                                if (strcmp(stmt_child->token.value, "int") == 0) {
                                    char bitstr[40] = {0};
                                    int num = atoi(val);
                                    for (int b = 31; b >= 0; --b) {
                                        bitstr[31 - b] = ((num >> b) & 1) ? '1' : '0';
                                    }
                                    bitstr[32] = '\0';
                                    fprintf(out, "\"%s\"%s", bitstr, (k < init_list->num_children - 1) ? ", " : "");
                                } else if (strcmp(stmt_child->token.value, "float") == 0 || strcmp(stmt_child->token.value, "double") == 0) {
                                    fprintf(out, "%s%s", val, (k < init_list->num_children - 1) ? ", " : "");
                                } else if (strcmp(stmt_child->token.value, "char") == 0) {
                                    fprintf(out, "'%s'%s", val, (k < init_list->num_children - 1) ? ", " : "");
                                } else {
                                    fprintf(out, "%s%s", val, (k < init_list->num_children - 1) ? ", " : "");
                                }
                            }
                            fprintf(out, ");\n");
                            fprintf(out, "  signal %s : %s_type := %s_init;\n", arr_name, arr_name, arr_name);
                        }
                    }
                } else if (strcmp(stmt_child->value, "result") == 0) {
                    fprintf(out, "  signal ");
                    emit_signal_name(stmt_child->value, out);
                    fprintf(out, " : %s;\n", ctype_to_vhdl(stmt_child->token.value));
                } else {
                    fprintf(out, "  signal %s : %s;\n", stmt_child->value, ctype_to_vhdl(stmt_child->token.value));
                }
            }
            // Decls inside for loops
            if (stmt_child->type == NODE_FOR_STATEMENT) {
                for (int f = 0; f < stmt_child->num_children; ++f) {
                    ASTNode *for_child = stmt_child->children[f];
                    if (for_child->type == NODE_VAR_DECL) {
                        char *arr_br = for_child->value ? strchr(for_child->value, '[') : NULL;
                        if (arr_br) {
                            int name_len = (int)(arr_br - for_child->value);
                            char arr_name[64] = {0}; strncpy(arr_name, for_child->value, name_len);
                            char arr_size[32] = {0};
                            const char *size_start = arr_br + 1;
                            const char *size_end = strchr(size_start, ']');
                            if (size_end && size_end > size_start) {
                                strncpy(arr_size, size_start, size_end - size_start);
                                const char *vhdl_elem_type = ctype_to_vhdl(for_child->token.value);
                                fprintf(out, "  type %s_type is array (0 to %d) of %s;\n", arr_name, atoi(arr_size) - 1, vhdl_elem_type);
                                fprintf(out, "  signal %s : %s_type;\n", arr_name, arr_name);
                            }
                        } else {
                            fprintf(out, "  signal %s : %s;\n", for_child->value, ctype_to_vhdl(for_child->token.value));
                        }
                    }
                }
            }
        }
    }
}

static void emit_struct_return_copy(ASTNode *expr, ASTNode *function_decl, FILE *out, const char *indent)
{

    if (!function_decl || !expr || !expr->value) return;

    const char *struct_ret_name = function_decl->token.value;
    int sidx = find_struct_index(struct_ret_name);

    if (sidx < 0) return;
    
    int field_index = 0;

    for (field_index = 0; field_index < g_structs[sidx].field_count; ++field_index) {
        fprintf(out, "%sresult.%s <= %s.%s;\n", indent,
                g_structs[sidx].fields[field_index].field_name,
                expr->value,
                g_structs[sidx].fields[field_index].field_name);
    }
}

// -------------------------------------------------------------
// Function call generation
// -------------------------------------------------------------

/**
 * Generates VHDL code for a function call expression.
 * 
 * Current implementation: Emits the function call as-is in VHDL syntax.
 * This works for simple expressions but doesn't handle:
 * - Component instantiation for called functions
 * - Signal routing for return values
 * - Multi-cycle function execution
 * 
 * Future enhancement: Generate proper component instantiation with
 * unique instance names and signal wiring.
 * 
 * @param node Function call AST node (NODE_FUNC_CALL)
 * @param out  Output file stream for VHDL code
 */
static void gen_func_call(ASTNode *node, FILE *out)
{
    int arg_index = 0;
    
    if (!node || !node->value)
    {
        fprintf(out, "-- Error: unknown function call");
        return;
    }
    
    // Emit function name
    fprintf(out, "%s(", node->value);
    
    // Emit comma-separated arguments
    for (arg_index = 0; arg_index < node->num_children; arg_index++)
    {
        if (arg_index > 0)
        {
            fprintf(out, ", ");
        }
        gen_node(node->children[arg_index], out);
    }
    
    fprintf(out, ")");
}

// -------------------------------------------------------------
// End of readable codegen VHDL
// -------------------------------------------------------------

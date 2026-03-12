#include "utils.h"

// Buffer-safe string append: appends src to dst without overflowing dst_size
void safe_append(char *dst, size_t dst_size, const char *src)
{
    size_t used = strlen(dst);
    if (used >= dst_size - 1) {
        return;
    }
    size_t available_space = dst_size - 1 - used;
    size_t copy_length = strlen(src);
    if (copy_length > available_space) {
        copy_length = available_space;
    }
    memcpy(dst + used, src, copy_length);
    dst[used + copy_length] = '\0';
}

// Buffer-safe string copy: copies at most limit chars from src into dst
void safe_copy(char *dst, size_t dst_size, const char *src, size_t limit)
{
    if (!dst_size) {
        return;
    }
    size_t source_length = strlen(src);
    if (source_length > limit) {
        source_length = limit;
    }
    if (source_length >= dst_size) {
        source_length = dst_size - 1;
    }
    memcpy(dst, src, source_length);
    dst[source_length] = '\0';
}

// Helper function to get operator precedence
int get_precedence(const char *op)
{
    if (!op) {
        return PREC_UNKNOWN;
    }
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        return PREC_MULTIPLICATIVE;
    }
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
        return PREC_ADDITIVE;
    }
    if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
        return PREC_SHIFT;
    }
    if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
        strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        return PREC_RELATIONAL;
    }
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
        return PREC_EQUALITY;
    }
    if (strcmp(op, "&") == 0) {
        return PREC_BITWISE_AND;
    }
    if (strcmp(op, "^") == 0) {
        return PREC_BITWISE_XOR;
    }
    if (strcmp(op, "|") == 0) {
        return PREC_BITWISE_OR;
    }
    if (strcmp(op, "&&") == 0) {
        return PREC_LOGICAL_AND;
    }
    if (strcmp(op, "||") == 0) {
        return PREC_LOGICAL_OR;
    }
    return PREC_UNKNOWN;
}

// Helper function to print tree branch decoration
void print_tree_prefix(int level, int is_last)
{

    int level_idx = 0;

    for (level_idx = 0; level_idx < level; level_idx++) {
        printf("%s", (level_idx == level - 1) ? (is_last ? "└── " : "├── ") : "    ");
    }
}


int is_number_str(const char *str)
{

    const char *str_ptr = NULL;

    if (!str || !*str) {
        return 0;
    }

    str_ptr = str;

    if (*str_ptr == '+' || *str_ptr == '-') {
        str_ptr++;
    }
    if (!*str_ptr) {
        return 0;
    }

    while (*str_ptr) {
        if (!isdigit((unsigned char)*str_ptr)) {
            return 0;
        }
        str_ptr++;
    }

    return 1;
}

// Add this helper function above generate_vhdl:
int is_negative_literal(const char* value)
{

    int char_idx = 1;
    int has_literal_chars = 0;

    if (!value || value[0] != '-' || strlen(value) < 2)
        return 0;

    while (value[char_idx]) {
        if (isdigit(value[char_idx]) || value[char_idx] == '.' || isalpha(value[char_idx]) || value[char_idx] == '_') {
            has_literal_chars = 1;
        } else {
            return 0;
        }
        char_idx++;
    }

    return has_literal_chars;
}

// Print the AST recursively in a readable tree format
void print_ast(ASTNode* node, int level)
{

    int is_last = 1;
    int child_idx = 0;
    ASTNode *parent = NULL;

    if (!node) {
        return;
    }

    // Find if this node is the last child
    if (node->parent) {
        parent = node->parent;
        for (child_idx = 0; child_idx < parent->num_children; child_idx++) {
            if (parent->children[child_idx] == node) {
                is_last = (child_idx == parent->num_children - 1);
                break;
            }
        }
    }

    print_tree_prefix(level, is_last);

    // Print node type
    switch (node->type) {
        case NODE_PROGRAM:
            printf("PROGRAM\n");
            break;
        case NODE_FUNCTION_DECL:
            printf("FUNCTION: %s (returns: %s)\n",
                   node->value ? node->value : "(null)",
                   node->token.value);
            break;
        case NODE_VAR_DECL:
            printf("VAR: %s %s\n",
                   node->token.value,
                   node->value ? node->value : "(null)");
            break;
        case NODE_STATEMENT:
            printf("STATEMENT\n");
            break;
        case NODE_EXPRESSION:
            printf("EXPR: %s\n", node->value ? node->value : "(null)");
            break;
        case NODE_BINARY_EXPR:
            printf("BINARY: %s\n", node->value ? node->value : "(op)");
            break;
        case NODE_ASSIGNMENT:
            printf("ASSIGN\n");
            break;
        case NODE_BINARY_OP:
            printf("UNARY: %s\n", node->value ? node->value : "(unary)");
            break;
        case NODE_IF_STATEMENT:
            printf("IF\n");
            break;
        case NODE_ELSE_IF_STATEMENT:
            printf("ELSE IF\n");
            break;
        case NODE_ELSE_STATEMENT:
            printf("ELSE\n");
            break;
        case NODE_WHILE_STATEMENT:
            printf("WHILE\n");
            break;
        case NODE_BREAK_STATEMENT:
            printf("BREAK\n");
            break;
        case NODE_CONTINUE_STATEMENT:
            printf("CONTINUE\n");
            break;
        default:
            printf("NODE_TYPE_%d\n", node->type);
            break;
    }

    for (child_idx = 0; child_idx < node->num_children; child_idx++) {
        print_ast(node->children[child_idx], level + 1);
    }
}

// Helper function to map C types to VHDL types
char* ctype_to_vhdl(const char* ctype)
{

    // No local variables needed
    if (strcmp(ctype, "int") == 0) {
        return "std_logic_vector(31 downto 0)";
    } else if (strcmp(ctype, "float") == 0) {
        return "std_logic_vector(31 downto 0)"; // You may want to use 'real' for advanced VHDL
    } else if (strcmp(ctype, "double") == 0) {
        return "std_logic_vector(63 downto 0)"; // Or 'real'
    } else if (strcmp(ctype, "char") == 0) {
        return "std_logic_vector(7 downto 0)";
    }
    // Default fallback
    return "std_logic_vector(31 downto 0)";
}
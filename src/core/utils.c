#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "error_handler.h"

// Prevents buffer overflows when building strings incrementally (e.g. VHDL identifiers).
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

// Bounded copy — callers set limit when the source may be unterminated
// or when only a prefix is needed (e.g. token extraction from input buffer).
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

// Drives the parser's decision of when to reduce binary expressions.
// Higher precedence binds tighter, matching standard C operator rules.
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

// Box-drawing characters (└── / ├──) give visual structure to the AST dump.
static void print_tree_prefix(int level, int is_last)
{
    for (int level_idx = 0; level_idx < level; level_idx++) {
        printf("%s", (level_idx == level - 1) ? (is_last ? "└── " : "├── ") : "    ");
    }
}


int is_number_str(const char *str)
{
    if (!str || !*str) {
        return 0;
    }

    const char *str_ptr = str;

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

// Each AST node type has a distinct display format for the debug tree.
// Extracted so print_ast() stays focused on tree traversal/indentation.
static void print_node_type(const ASTNode *node)
{
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
        case NODE_FOR_STATEMENT:
            printf("FOR\n");
            break;
        case NODE_FUNC_CALL:
            printf("FUNC_CALL: %s\n", node->value ? node->value : "(null)");
            break;
        case NODE_STRUCT_DECL:
            printf("STRUCT: %s\n", node->value ? node->value : "(null)");
            break;
        case NODE_LITERAL:
            printf("LITERAL: %s\n", node->value ? node->value : "(null)");
            break;
        case NODE_IDENTIFIER:
            printf("IDENTIFIER: %s\n", node->value ? node->value : "(null)");
            break;
        default:
            printf("NODE_TYPE_%d\n", node->type);
            break;
    }
}

// Recursive tree-format AST printer for debugging.
// Decorates each level with box-drawing characters to visualize parent-child structure.
void print_ast(ASTNode* node, int level)
{
    if (!node) {
        return;
    }

    int is_last = 1;

    if (node->parent) {
        const ASTNode *parent = node->parent;
        for (int child_idx = 0; child_idx < parent->num_children; child_idx++) {
            if (parent->children[child_idx] == node) {
                is_last = (child_idx == parent->num_children - 1);
                break;
            }
        }
    }

    print_tree_prefix(level, is_last);
    print_node_type(node);

    for (int child_idx = 0; child_idx < node->num_children; child_idx++) {
        print_ast(node->children[child_idx], level + 1);
    }
}

// Uses strtol instead of atoi to detect overflow, trailing garbage, and empty strings.
int safe_strtoi(const char *str, int *result)
{
    if (!str || !*str || !result) {
        return 0;
    }

    char *endptr = NULL;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    if (errno == ERANGE || val > INT_MAX || val < INT_MIN) {
        return 0;
    }
    if (endptr == str || *endptr != '\0') {
        return 0;
    }

    *result = (int)val;
    return 1;
}

// Duplicate a string, aborting on allocation failure.
// In a compiler, OOM is fatal — callers should not need to check.
char *safe_strdup(const char *src)
{
    if (!src) {
        return NULL;
    }

    char *copy = strdup(src);
    if (!copy) {
        log_error(ERROR_CATEGORY_GENERAL, 0, "Failed to allocate memory for string");
        exit(EXIT_FAILURE);
    }
    return copy;
}
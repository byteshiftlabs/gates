#ifndef ASTNODE_H
#define ASTNODE_H

/**
 * @file astnode.h
 * @brief AST node types, structure, and management functions.
 */

#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

// AST Node Types
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION_DECL,
    NODE_STRUCT_DECL,
    NODE_VAR_DECL,
    NODE_STATEMENT,
    NODE_EXPRESSION,
    NODE_BINARY_EXPR,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_ASSIGNMENT,
    NODE_BINARY_OP,
    NODE_IF_STATEMENT,
    NODE_ELSE_IF_STATEMENT,
    NODE_ELSE_STATEMENT,
    NODE_WHILE_STATEMENT,
    NODE_FOR_STATEMENT,
    NODE_BREAK_STATEMENT,
    NODE_CONTINUE_STATEMENT,
    NODE_FUNC_CALL
} NodeType;


// AST Node structure
typedef struct ASTNode {
    NodeType type;
    Token token;               // Original token (if applicable)
    char *value;               // Value or additional info
    struct ASTNode *parent;    // Parent node
    struct ASTNode **children; // Child nodes
    int num_children;          // Number of children
    int capacity;              // Capacity of children array
} ASTNode;


/**
 * @brief Create a new AST node of the given type.
 * @param type  The node type (e.g. NODE_PROGRAM, NODE_IF_STATEMENT).
 * @return Pointer to the newly allocated node, or NULL on failure.
 */
ASTNode* create_node(NodeType type);

/**
 * @brief Recursively free an AST node and all its children.
 * @param node  Root of the subtree to free (may be NULL).
 */
void free_node(ASTNode* node);

/**
 * @brief Append a child node to a parent's children array.
 * @param parent  Parent node to add the child to.
 * @param child   Child node to attach.
 */
void add_child(ASTNode* parent, ASTNode* child);

#ifdef __cplusplus
}
#endif

#endif // ASTNODE_H
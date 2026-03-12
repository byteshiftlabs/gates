#include <stdio.h>
#include <stdlib.h>

#include "astnode.h"
#include "error_handler.h"

#define INITIAL_CHILDREN_CAPACITY 4
#define CHILDREN_GROWTH_FACTOR   2

// Create a new AST node
ASTNode* create_node(NodeType type)
{
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));

    if (!node) {
        fprintf(stderr, "Fatal: Failed to allocate memory for AST node\n");
        exit(EXIT_FAILURE);
    }
    
    node->type = type;
    node->value = NULL;
    node->parent = NULL;
    node->children = NULL;
    node->num_children = 0;
    node->capacity = 0;
    
    return node;
}

// Free an AST node and all its children
void free_node(ASTNode *node)
{
    if (!node) {
        return;
    }
    
    for (int child_idx = 0; child_idx < node->num_children; child_idx++) {
        free_node(node->children[child_idx]);
    }
    
    free(node->children);
    free(node->value);
    free(node);
}

// Add a child node
void add_child(ASTNode *parent, ASTNode *child)
{
    if (!parent || !child) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "add_child called with NULL %s", !parent ? "parent" : "child");
        return;
    }

    if (!parent->children) {
        parent->capacity = INITIAL_CHILDREN_CAPACITY;
        parent->children = (ASTNode**)malloc((size_t)parent->capacity * sizeof(ASTNode*));
        if (!parent->children) {
            log_error(ERROR_CATEGORY_GENERAL, 0, "Failed to allocate memory for child nodes");
            return;
        }
    }
    
    if (parent->num_children >= parent->capacity) {
        parent->capacity *= CHILDREN_GROWTH_FACTOR;
        ASTNode **new_children = (ASTNode**)realloc(parent->children, 
                                             (size_t)parent->capacity * sizeof(ASTNode*));
        if (!new_children) {
            log_error(ERROR_CATEGORY_GENERAL, 0, "Failed to reallocate memory for child nodes");
            return;
        }
        parent->children = new_children;
    }
    
    parent->children[parent->num_children++] = child;
    child->parent = parent;
}
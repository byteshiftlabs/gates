#ifndef UTILS_H
#define UTILS_H

#include "astnode.h"  // ASTNode definition

// Operator precedence constants (mirrors C precedence ordering)
// Higher number = higher precedence
#define PREC_MULTIPLICATIVE   7   // * /
#define PREC_ADDITIVE         6   // + -
#define PREC_SHIFT            5   // << >>
#define PREC_RELATIONAL       4   // < <= > >=
#define PREC_EQUALITY         3   // == !=
#define PREC_BITWISE_AND      2   // &
#define PREC_BITWISE_XOR      1   // ^
#define PREC_BITWISE_OR       0   // |
#define PREC_LOGICAL_AND     -1   // &&
#define PREC_LOGICAL_OR      -2   // || (lowest)
#define PREC_UNKNOWN       -999   // Unknown operator

// Expression parsing precedence bounds
#define PREC_PARENTHESIZED_MIN  1   // Minimum precedence for parenthesized expressions
#define PREC_TOP_LEVEL_MIN     -2   // Minimum precedence for top-level expressions

char* ctype_to_vhdl(const char* ctype);
void print_ast(ASTNode* node, int level);
int is_number_str(const char *s);
int is_negative_literal(const char* value);
int get_precedence(const char *op);

#endif
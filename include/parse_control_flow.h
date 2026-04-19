#ifndef PARSE_CONTROL_FLOW_H
#define PARSE_CONTROL_FLOW_H

#include <stdio.h>
#include "astnode.h"

// Control flow statement parsers (if, while, for, break, continue)
ASTNode* parse_if_statement(FILE *input);
ASTNode* parse_while_statement(FILE *input);
ASTNode* parse_for_statement(FILE *input);
ASTNode* parse_break_statement(FILE *input);
ASTNode* parse_continue_statement(FILE *input);

#endif // PARSE_CONTROL_FLOW_H

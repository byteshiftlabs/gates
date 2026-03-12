#ifndef PARSE_H
#define PARSE_H

#include "astnode.h"

// Parsing interface
ASTNode* parse_program(FILE *input);

// Sub-parser headers
#include "parse_struct.h"
#include "parse_function.h"
#include "parse_statement.h"
#include "parse_expression.h"

#endif
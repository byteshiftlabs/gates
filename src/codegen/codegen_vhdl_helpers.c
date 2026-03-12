// VHDL Code Generator - Helper Utilities Implementation
// -------------------------------------------------------------

#include <ctype.h>
#include <string.h>
#include <strings.h>  // For strcasecmp

#include "codegen_vhdl_helpers.h"
#include "error_handler.h"
#include "codegen_vhdl_constants.h"
#include "codegen_vhdl_emit.h"
#include "symbol_structs.h"

#define VHDL_IDENTIFIER_BUFFER_SIZE 256

// VHDL port "result" conflicts with user variables named "result".
// Reserved names get a "_local" suffix to avoid port shadowing.
static int is_signal_name_reserved(const char *variable_name)
{
    if (variable_name == NULL)
    {
        return 0;
    }
    
    return (strcmp(variable_name, RESERVED_PORT_NAME_RESULT) == 0);
}

// Central emission point for identifiers — ensures every name in the output
// is valid VHDL (no leading digits, no consecutive underscores, no reserved words).
void emit_safe_identifier(const char *name)
{
    if (name == NULL)
    {
        emit_raw("%s", UNKNOWN_IDENTIFIER);
        return;
    }
    
    // Fast path: if already valid, emit directly
    if (is_valid_vhdl_identifier(name))
    {
        emit_raw("%s", name);
        return;
    }
    
    // Sanitize the identifier
    char buffer[VHDL_IDENTIFIER_BUFFER_SIZE];
    if (sanitize_vhdl_identifier(name, buffer, sizeof(buffer)))
    {
        emit_raw("%s", buffer);
    }
    else
    {
        // Fallback - emit warning and use original
        log_warning(ERROR_CATEGORY_CODEGEN, 0,
                    "Could not sanitize identifier '%s' for VHDL", name);
        emit_raw("%s", name);
    }
}

// Emits a signal name, appending "_local" when it would shadow a VHDL port.
// Sanitization runs first so the reserved-name check works on the final form.
void emit_mapped_signal_name(const char *variable_name)
{
    if (variable_name == NULL)
    {
        emit_raw("%s", UNKNOWN_IDENTIFIER);
        return;
    }
    
    char buffer[VHDL_IDENTIFIER_BUFFER_SIZE];
    const char *safe_name = variable_name;
    
    if (!is_valid_vhdl_identifier(variable_name))
    {
        if (sanitize_vhdl_identifier(variable_name, buffer, sizeof(buffer)))
        {
            safe_name = buffer;
        }
        else
        {
            log_warning(ERROR_CATEGORY_CODEGEN, 0,
                        "Could not sanitize signal name '%s' for VHDL", variable_name);
        }
    }
    
    if (is_signal_name_reserved(safe_name))
    {
        emit_raw("%s%s", safe_name, SIGNAL_SUFFIX_LOCAL);
    }
    else
    {
        emit_raw("%s", safe_name);
    }
}

// Boolean comparisons produce std_logic '0'/'1' in VHDL, not integers.
// This distinction drives the emitter's decision to wrap results in boolean gates.
int is_boolean_comparison_operator(const char *op)
{
    if (op == NULL)
    {
        return 0;
    }

    return (strcmp(op, OP_EQUAL) == 0 ||
            strcmp(op, OP_NOT_EQUAL) == 0 ||
            strcmp(op, OP_LESS_THAN) == 0 ||
            strcmp(op, OP_LESS_EQUAL) == 0 ||
            strcmp(op, OP_GREATER_THAN) == 0 ||
            strcmp(op, OP_GREATER_EQUAL) == 0 ||
            strcmp(op, OP_LOGICAL_AND) == 0 ||
            strcmp(op, OP_LOGICAL_OR) == 0);
}

// Determines whether a node produces a boolean result, which controls
// whether the emitter wraps it in std_logic conversion vs arithmetic.
int is_node_boolean_expression(const ASTNode *node)
{
    if (node == NULL)
    {
        return 0;
    }
    
    if (node->type == NODE_BINARY_EXPR && node->value != NULL)
    {
        return is_boolean_comparison_operator(node->value);
    }
    
    if (node->type == NODE_BINARY_OP && 
        node->value != NULL && 
        strcmp(node->value, OP_LOGICAL_NOT) == 0)
    {
        return 1;
    }
    
    return 0;
}

// Plain identifiers emit directly; array/struct accesses need special
// indexing or field-selection syntax in VHDL.
int is_plain_identifier(const char *expression_value)
{
    if (expression_value == NULL)
    {
        return 0;
    }
    
    // Array brackets, dots, and "__" all indicate compound access
    for (const char *character_ptr = expression_value; *character_ptr != '\0'; ++character_ptr)
    {
        if (*character_ptr == '[' || *character_ptr == ']' || *character_ptr == '.')
        {
            return 0;
        }
    }
    
    if (strstr(expression_value, "__") != NULL)
    {
        return 0;
    }
    
    return 1;
}

// Numeric literals need to_unsigned()/to_signed() casts in VHDL;
// identifiers and expressions need unsigned()/signed() wrapping instead.
int is_numeric_literal(const char *value)
{
    const char *char_ptr = value;
    int dot_count = 0;
    
    if (value == NULL || *value == '\0')
    {
        return 0;
    }
    
    // Check each character
    while (*char_ptr != '\0')
    {
        if (*char_ptr == '.')
        {
            ++dot_count;
            if (dot_count > 1)
            {
                return 0;
            }
        }
        else if (!isdigit(*char_ptr))
        {
            return 0;
        }
        ++char_ptr;
    }
    
    return 1;
}

// Negative numeric literals get to_signed() in VHDL, never to_unsigned().
int is_negative_numeric_literal(const char *value)
{
    if (value == NULL || value[0] != '-' || value[1] == '\0')
    {
        return 0;
    }
    
    // Check if what follows '-' is numeric
    return is_numeric_literal(value + 1);
}

// VHDL requires explicit width for numeric conversions;
// VHDL_BIT_WIDTH ensures consistent sizing across the design.
void emit_unsigned_cast(const char *value)
{
    emit_raw("to_unsigned(%s, %d)", value, VHDL_BIT_WIDTH);
}

void emit_signed_cast(const char *value)
{
    emit_raw("to_signed(%s, %d)", value, VHDL_BIT_WIDTH);
}

// Literals use to_unsigned/to_signed; variable references use unsigned()/signed()
// wrapping because they're already std_logic_vector signals.
void emit_typed_operand(ASTNode *operand, int is_signed, void (*node_generator)(ASTNode*))
{
    if (operand == NULL)
    {
        emit_raw("0");
        return;
    }
    
    // Handle expression nodes with literal values
    if (operand->type == NODE_EXPRESSION && operand->value != NULL)
    {
        if (is_negative_numeric_literal(operand->value))
        {
            emit_signed_cast(operand->value);
        }
        else if (is_numeric_literal(operand->value))
        {
            if (is_signed)
            {
                emit_signed_cast(operand->value);
            }
            else
            {
                emit_unsigned_cast(operand->value);
            }
        }
        else
        {
            // Variable or expression - wrap in unsigned/signed cast
            emit_raw("%s(", is_signed ? "signed" : "unsigned");
            emit_raw("%s", operand->value);
            emit_raw(")");
        }
    }
    else
    {
        // Complex expression - recursively generate and wrap
        emit_raw("%s(", is_signed ? "signed" : "unsigned");
        if (node_generator != NULL)
        {
            node_generator(operand);
        }
        emit_raw(")");
    }
}

// The parser wraps assignments and declarations in NODE_STATEMENT nodes.
// Unwrapping lets the codegen handle the inner node directly without
// an extra nesting level in each emitter function.
ASTNode* unwrap_statement_node(ASTNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    
    if (node->type == NODE_STATEMENT && 
        node->num_children == 1 &&
        (node->children[0]->type == NODE_VAR_DECL || 
         node->children[0]->type == NODE_ASSIGNMENT))
    {
        return node->children[0];
    }
    
    return node;
}

// VHDL records can't be assigned as a whole in all contexts,
// so we emit per-field "<=" assignments to ensure compatibility.
void emit_struct_field_assignments(int struct_index, const char *target_name, 
                                   const char *source_name)
{
    const StructInfo *struct_info = get_struct_info(struct_index);
    if (!struct_info || target_name == NULL || source_name == NULL)
    {
        log_warning(ERROR_CATEGORY_CODEGEN, 0,
                    "Cannot emit struct field assignments: invalid struct or names");
        return;
    }
    
    for (int field_index = 0; field_index < struct_info->field_count; ++field_index)
    {
        emit_line("%s.%s <= %s.%s;", 
                target_name,
                struct_info->fields[field_index].field_name,
                source_name,
                struct_info->fields[field_index].field_name);
    }
}

// Map C primitive types to VHDL std_logic_vector widths.
// Uses centralized VHDL_TYPE_* constants so IEEE 754 float support
// requires changing only VHDL_TYPE_FLOAT in codegen_vhdl_constants.c.
const char* ctype_to_vhdl(const char* ctype)
{
    if (strcmp(ctype, C_TYPE_INT) == 0) {
        return VHDL_TYPE_INT;
    } else if (strcmp(ctype, C_TYPE_FLOAT) == 0) {
        return VHDL_TYPE_FLOAT;
    } else if (strcmp(ctype, C_TYPE_DOUBLE) == 0) {
        return VHDL_TYPE_DOUBLE;
    } else if (strcmp(ctype, C_TYPE_CHAR) == 0) {
        return VHDL_TYPE_CHAR;
    }
    return VHDL_TYPE_DEFAULT;
}

// Broader negative check than is_negative_numeric_literal(): also allows
// hex/alpha suffixes (e.g. "-0x1F") that appear in some C literal forms.
int is_negative_literal(const char* value)
{
    if (!value || value[0] != '-' || strlen(value) < 2)
        return 0;

    int char_idx = 1;
    int has_literal_chars = 0;

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

// Sanitization must avoid these — a C identifier like "process" would
// produce invalid VHDL without renaming.
static const char *vhdl_reserved_words[] = {
    "abs", "access", "after", "alias", "all", "and", "architecture",
    "array", "assert", "attribute", "begin", "block", "body", "buffer",
    "bus", "case", "component", "configuration", "constant", "disconnect",
    "downto", "else", "elsif", "end", "entity", "exit", "file", "for",
    "function", "generate", "generic", "group", "guarded", "if", "impure",
    "in", "inertial", "inout", "is", "label", "library", "linkage",
    "literal", "loop", "map", "mod", "nand", "new", "next", "nor", "not",
    "null", "of", "on", "open", "or", "others", "out", "package", "port",
    "postponed", "procedure", "process", "pure", "range", "record",
    "register", "reject", "rem", "report", "return", "rol", "ror",
    "select", "severity", "signal", "shared", "sla", "sll", "sra", "srl",
    "subtype", "then", "to", "transport", "type", "unaffected", "units",
    "until", "use", "variable", "wait", "when", "while", "with", "xnor", "xor",
    NULL
};

// Case-insensitive because VHDL identifiers are case-insensitive.
static int is_vhdl_reserved_word(const char *name)
{
    if (name == NULL)
    {
        return 0;
    }
    
    for (int i = 0; vhdl_reserved_words[i] != NULL; ++i)
    {
        if (strcasecmp(name, vhdl_reserved_words[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// VHDL identifier rules: must start with letter, no consecutive underscores,
// no trailing underscore, no reserved words. Invalid identifiers get sanitized.
int is_valid_vhdl_identifier(const char *name)
{
    if (name == NULL || name[0] == '\0')
    {
        return 0;
    }
    
    // Must start with a letter
    if (!isalpha(name[0]))
    {
        return 0;
    }
    
    size_t len = strlen(name);
    
    // Cannot end with underscore
    if (name[len - 1] == '_')
    {
        return 0;
    }
    
    // Check all characters and consecutive underscores
    int prev_was_underscore = 0;
    for (size_t i = 0; i < len; ++i)
    {
        char c = name[i];
        
        if (c == '_')
        {
            if (prev_was_underscore)
            {
                return 0;  // Consecutive underscores not allowed
            }
            prev_was_underscore = 1;
        }
        else if (isalnum(c))
        {
            prev_was_underscore = 0;
        }
        else
        {
            return 0;  // Invalid character
        }
    }
    
    // Check against reserved words
    if (is_vhdl_reserved_word(name))
    {
        return 0;
    }
    
    return 1;
}

// Transforms C identifiers into valid VHDL: prefixes digits/underscores
// with "c_", collapses double underscores, and escapes reserved words with "v_".
int sanitize_vhdl_identifier(const char *name, char *buffer, size_t buffer_size)
{
    if (name == NULL || buffer == NULL || buffer_size < 4)
    {
        return 0;
    }
    
    size_t out_idx = 0;
    
    // Prefix with 'c_' if starts with digit or underscore
    if (!isalpha(name[0]))
    {
        if (out_idx + 2 >= buffer_size) return 0;
        buffer[out_idx++] = 'c';
        buffer[out_idx++] = '_';
    }
    
    int prev_was_underscore = (out_idx > 0 && buffer[out_idx - 1] == '_');
    size_t name_len = strlen(name);
    for (size_t i = 0; i < name_len && out_idx < buffer_size - 1; ++i)
    {
        char c = name[i];
        
        if (c == '_')
        {
            if (!prev_was_underscore)
            {
                buffer[out_idx++] = '_';
                prev_was_underscore = 1;
            }
            // Skip consecutive underscores
        }
        else if (isalnum(c))
        {
            buffer[out_idx++] = c;
            prev_was_underscore = 0;
        }
        // Skip other invalid characters
    }
    
    // Remove trailing underscore
    if (out_idx > 0 && buffer[out_idx - 1] == '_')
    {
        out_idx--;
    }
    
    buffer[out_idx] = '\0';
    
    // Check if result is a reserved word, prefix with 'v_' if so
    if (is_vhdl_reserved_word(buffer))
    {
        if (out_idx + 3 >= buffer_size) return 0;
        // Shift right and add prefix
        memmove(buffer + 2, buffer, out_idx + 1);
        buffer[0] = 'v';
        buffer[1] = '_';
    }
    
    return 1;
}

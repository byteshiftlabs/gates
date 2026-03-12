// VHDL Code Generator - Emit Utilities Implementation
// -------------------------------------------------------------
// Purpose: Centralized output emission with proper indentation state
// -------------------------------------------------------------

#include <stdio.h>
#include <stdarg.h>

#include "codegen_vhdl_emit.h"

// -------------------------------------------------------------
// Module state
// -------------------------------------------------------------
static FILE *g_output = NULL;
static int g_indent = 0;

// Indent string (2 spaces per level, matching existing style)
#define INDENT_STR "  "
#define MAX_INDENT 16  // Prevent runaway indentation

// -------------------------------------------------------------
// State management
// -------------------------------------------------------------

void emit_init(FILE *output_file)
{
    g_output = output_file;
    g_indent = 0;
}

FILE* emit_get_file(void)
{
    return g_output;
}

void emit_indent_inc(void)
{
    if (g_indent < MAX_INDENT)
    {
        ++g_indent;
    }
}

void emit_indent_dec(void)
{
    if (g_indent > 0)
    {
        --g_indent;
    }
}

int emit_indent_level(void)
{
    return g_indent;
}

void emit_indent_set(int level)
{
    if (level < 0)
    {
        g_indent = 0;
    }
    else if (level > MAX_INDENT)
    {
        g_indent = MAX_INDENT;
    }
    else
    {
        g_indent = level;
    }
}

// -------------------------------------------------------------
// Output functions
// -------------------------------------------------------------

void emit_indent(void)
{
    if (g_output == NULL)
    {
        return;
    }
    
    for (int i = 0; i < g_indent; ++i)
    {
        fprintf(g_output, INDENT_STR);
    }
}

void emit_line(const char *fmt, ...)
{
    if (g_output == NULL || fmt == NULL)
    {
        return;
    }
    
    emit_indent();
    
    va_list args;
    va_start(args, fmt);
    vfprintf(g_output, fmt, args);
    va_end(args);
    
    fprintf(g_output, "\n");
}

void emit_raw(const char *fmt, ...)
{
    if (g_output == NULL || fmt == NULL)
    {
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    vfprintf(g_output, fmt, args);
    va_end(args);
}

void emit_newline(void)
{
    if (g_output != NULL)
    {
        fprintf(g_output, "\n");
    }
}

void emit_indented(const char *fmt, ...)
{
    if (g_output == NULL || fmt == NULL)
    {
        return;
    }
    
    emit_indent();
    
    va_list args;
    va_start(args, fmt);
    vfprintf(g_output, fmt, args);
    va_end(args);
}

void emit_block_begin(const char *fmt, ...)
{
    if (g_output == NULL || fmt == NULL)
    {
        return;
    }
    
    emit_indent();
    
    va_list args;
    va_start(args, fmt);
    vfprintf(g_output, fmt, args);
    va_end(args);
    
    fprintf(g_output, "\n");
    emit_indent_inc();
}

void emit_block_end(const char *fmt, ...)
{
    if (g_output == NULL || fmt == NULL)
    {
        return;
    }
    
    emit_indent_dec();
    emit_indent();
    
    va_list args;
    va_start(args, fmt);
    vfprintf(g_output, fmt, args);
    va_end(args);
    
    fprintf(g_output, "\n");
}

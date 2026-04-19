// VHDL Code Generator - Emit Utilities
// -------------------------------------------------------------
// Purpose: Centralized output emission with proper indentation state
// -------------------------------------------------------------

#ifndef CODEGEN_VHDL_EMIT_H
#define CODEGEN_VHDL_EMIT_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------------------------------
// Emitter state management
// -------------------------------------------------------------

/**
 * Initialize emitter with output file.
 * Resets indentation to 0.
 */
void emit_init(FILE *output_file);

/**
 * Get the current output file.
 */
FILE* emit_get_file(void);

/**
 * Increase indent level by one.
 */
void emit_indent_inc(void);

/**
 * Decrease indent level by one (floors at 0).
 */
void emit_indent_dec(void);

/**
 * Get current indent level.
 */
int emit_indent_level(void);

/**
 * Set indent level directly (use sparingly).
 */
void emit_indent_set(int level);

// -------------------------------------------------------------
// Output functions
// -------------------------------------------------------------

/**
 * Emit current indentation whitespace only (no newline).
 */
void emit_indent(void);

/**
 * Emit a complete line with current indentation.
 * Adds newline automatically.
 * Usage: emit_line("port (");
 */
void emit_line(const char *fmt, ...);

/**
 * Emit text without indentation or newline.
 * For inline content like expressions.
 * Usage: emit_raw("clk : in std_logic");
 */
void emit_raw(const char *fmt, ...);

/**
 * Emit a newline only.
 */
void emit_newline(void);

/**
 * Emit indented text without newline.
 * Usage: emit_indented("while "); followed by emit_raw() for condition
 */
void emit_indented(const char *fmt, ...);

// -------------------------------------------------------------
// Convenience macros for scope blocks
// -------------------------------------------------------------

/**
 * Emit opening line and increase indent.
 * Usage: emit_block_begin("entity %s is", name);
 */
void emit_block_begin(const char *fmt, ...);

/**
 * Decrease indent and emit closing line.
 * Usage: emit_block_end("end entity;");
 */
void emit_block_end(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_VHDL_EMIT_H

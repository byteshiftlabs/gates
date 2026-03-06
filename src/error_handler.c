#include "error_handler.h"
#include <stdio.h>
#include <stdarg.h>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_NONE    ""

// Constants
#define INVALID_LINE_NUMBER 0
#define COLORED_OUTPUT_ENABLED 1
#define COLORED_OUTPUT_DISABLED 0
#define MAX_SOURCE_LINE_LENGTH 1024
#define INDENT_SPACES "    "

// Global counters
static int error_count = 0;
static int warning_count = 0;
static int colored_output_enabled = COLORED_OUTPUT_ENABLED;

// Category names for display
static const char* category_names[] = {
    "Lexer",
    "Parser",
    "Semantic",
    "Codegen",
    "General"
};

// Severity labels
static const char* severity_labels[] = {
    "info",
    "warning",
    "error"
};

// Color codes for hints and suggestions
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"

/**
 * Get color code for severity level
 */
static const char* get_color_for_severity(ErrorSeverity severity)
{
    if (!colored_output_enabled)
    {
        return COLOR_NONE;
    }
    
    switch (severity)
    {
        case SEVERITY_INFO:
            return COLOR_GREEN;
        case SEVERITY_WARNING:
            return COLOR_YELLOW;
        case SEVERITY_ERROR:
            return COLOR_RED;
        default:
            return COLOR_RESET;
    }
}

/**
 * Get reset color code
 */
static const char* get_reset_color(void)
{
    if (!colored_output_enabled)
    {
        return COLOR_NONE;
    }
    return COLOR_RESET;
}

/**
 * Get bold color code
 */
static const char* get_bold_color(void)
{
    if (!colored_output_enabled)
    {
        return COLOR_NONE;
    }
    return COLOR_BOLD;
}

/**
 * Print formatted severity header with color
 */
static void print_severity_header(ErrorSeverity severity, ErrorCategory category)
{
    const char* color = get_color_for_severity(severity);
    const char* reset = get_reset_color();
    const char* bold = get_bold_color();
    
    fprintf(stderr, "%s%s%s[%s]%s ", 
            bold, 
            color, 
            severity_labels[severity],
            category_names[category],
            reset);
}

/**
 * Print line number if valid
 */
static void print_line_number(int line)
{
    if (line > INVALID_LINE_NUMBER)
    {
        fprintf(stderr, "line %d: ", line);
    }
}

/**
 * Print formatted message with variadic arguments
 */
static void print_formatted_message(const char* format, va_list args)
{
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}

/**
 * Update error and warning counters
 */
static void update_counters(ErrorSeverity severity)
{
    if (severity == SEVERITY_ERROR)
    {
        error_count++;
    }
    else if (severity == SEVERITY_WARNING)
    {
        warning_count++;
    }
}

/**
 * Print filename if available
 */
static void print_filename(const char* filename)
{
    if (filename != NULL)
    {
        fprintf(stderr, "%s:", filename);
    }
}

/**
 * Print line and column numbers if valid
 */
static void print_location(int line, int column)
{
    if (line > INVALID_LINE_NUMBER)
    {
        fprintf(stderr, "%d:", line);
        if (column > INVALID_LINE_NUMBER)
        {
            fprintf(stderr, "%d:", column);
        }
    }
}

/**
 * Print error code if provided
 */
static void print_error_code(const char* error_code)
{
    if (error_code != NULL)
    {
        const char* cyan = colored_output_enabled ? COLOR_CYAN : COLOR_NONE;
        const char* reset = get_reset_color();
        fprintf(stderr, "%s[%s]%s", cyan, error_code, reset);
    }
}

/**
 * Print source context with column indicator
 */
static void print_source_context(const char* source_line, int column)
{
    if (source_line == NULL || column <= INVALID_LINE_NUMBER)
    {
        return;
    }
    
    const char* reset = get_reset_color();
    const char* cyan = colored_output_enabled ? COLOR_CYAN : COLOR_NONE;
    
    fprintf(stderr, "%s%s%s\n", INDENT_SPACES, cyan, source_line);
    
    // Print caret indicator at the column position
    fprintf(stderr, "%s", INDENT_SPACES);
    for (int i = 1; i < column; i++)
    {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^%s\n", reset);
}

void report_message(ErrorSeverity severity, ErrorCategory category, 
                   int line, const char* format, ...)
{
    va_list args;
    
    print_severity_header(severity, category);
    print_line_number(line);
    
    va_start(args, format);
    print_formatted_message(format, args);
    va_end(args);
    
    update_counters(severity);
}

void report_message_ex(ErrorSeverity severity, ErrorCategory category,
                      const ErrorLocation* location, const char* error_code,
                      const char* format, ...)
{
    va_list args;
    const char* color = get_color_for_severity(severity);
    const char* reset = get_reset_color();
    const char* bold = get_bold_color();
    
    // Print filename:line:column if available
    if (location != NULL && location->filename != NULL)
    {
        print_filename(location->filename);
        print_location(location->line, location->column);
        fprintf(stderr, " ");
    }
    
    // Print error code if provided
    print_error_code(error_code);
    
    // Print severity and category header
    fprintf(stderr, "%s%s%s[%s]%s ", 
            bold, 
            color, 
            severity_labels[severity],
            category_names[category],
            reset);
    
    // Print line number if location not already shown and line is valid
    if (location != NULL && location->filename == NULL && location->line > INVALID_LINE_NUMBER)
    {
        fprintf(stderr, "line %d: ", location->line);
    }
    
    // Print formatted message
    va_start(args, format);
    print_formatted_message(format, args);
    va_end(args);
    
    // Print source context if available
    if (location != NULL)
    {
        print_source_context(location->source_line, location->column);
    }
    
    update_counters(severity);
}

void add_error_hint(const char* format, ...)
{
    va_list args;
    const char* cyan = colored_output_enabled ? COLOR_CYAN : COLOR_NONE;
    const char* bold = get_bold_color();
    const char* reset = get_reset_color();
    
    fprintf(stderr, "%s%s%shint:%s ", INDENT_SPACES, bold, cyan, reset);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}

void add_suggestion(const char* suggestion)
{
    const char* magenta = colored_output_enabled ? COLOR_MAGENTA : COLOR_NONE;
    const char* bold = get_bold_color();
    const char* reset = get_reset_color();
    
    fprintf(stderr, "%s%s%shelp:%s did you mean '%s'?\n", 
            INDENT_SPACES, bold, magenta, reset, suggestion);
}

void log_info(ErrorCategory category, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    print_severity_header(SEVERITY_INFO, category);
    print_line_number(line);
    print_formatted_message(format, args);
    
    va_end(args);
}

void log_warning(ErrorCategory category, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    print_severity_header(SEVERITY_WARNING, category);
    print_line_number(line);
    print_formatted_message(format, args);
    
    va_end(args);
    
    warning_count++;
}

void log_error(ErrorCategory category, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    print_severity_header(SEVERITY_ERROR, category);
    print_line_number(line);
    print_formatted_message(format, args);
    
    va_end(args);
    
    error_count++;
}

int get_error_count(void)
{
    return error_count;
}

int get_warning_count(void)
{
    return warning_count;
}

void reset_error_counters(void)
{
    error_count = 0;
    warning_count = 0;
}

int has_errors(void)
{
    return error_count > INVALID_LINE_NUMBER;
}

void set_colored_output(int enable)
{
    colored_output_enabled = enable;
}

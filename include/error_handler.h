#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H


// Error severity levels
typedef enum {
    SEVERITY_INFO,      // Informational messages (green)
    SEVERITY_WARNING,   // Warnings that don't stop compilation (yellow)
    SEVERITY_ERROR      // Errors that stop compilation (red)
} ErrorSeverity;

// Error category for better organization
typedef enum {
    ERROR_CATEGORY_LEXER,
    ERROR_CATEGORY_PARSER,
    ERROR_CATEGORY_SEMANTIC,
    ERROR_CATEGORY_CODEGEN,
    ERROR_CATEGORY_GENERAL
} ErrorCategory;

// Constants for location tracking
#define NO_LINE_NUMBER 0
#define NO_COLUMN_NUMBER 0

/**
 * Structure to hold error location information
 */
typedef struct {
    const char* filename;    // Source file name (NULL if not applicable)
    int line;                // Line number (0 if not applicable)
    int column;              // Column number (0 if not applicable)
    const char* source_line; // The actual source line text (NULL if not available)
} ErrorLocation;

/**
 * Report a message with specified severity level and enhanced location info
 * 
 * @param severity  Severity level (INFO, WARNING, ERROR)
 * @param category  Error category
 * @param location  Location information (file, line, column, source text)
 * @param error_code Error code string (e.g., "E0001", can be NULL)
 * @param format    Printf-style format string
 * @param ...       Variable arguments for format string
 */
void report_message_ex(ErrorSeverity severity, ErrorCategory category,
                      const ErrorLocation* location, const char* error_code,
                      const char* format, ...);

/**
 * Add a hint/suggestion line to the most recent error message
 * 
 * @param format Printf-style format string for the hint
 * @param ...    Variable arguments for format string
 */
void add_error_hint(const char* format, ...);

/**
 * Add a "did you mean?" suggestion to the most recent error message
 * 
 * @param suggestion The suggested correction
 */
void add_suggestion(const char* suggestion);

/**
 * Report a message with specified severity level (legacy interface)
 * 
 * @param severity  Severity level (INFO, WARNING, ERROR)
 * @param category  Error category
 * @param line      Line number where the issue occurred (0 if not applicable)
 * @param format    Printf-style format string
 * @param ...       Variable arguments for format string
 */
void report_message(ErrorSeverity severity, ErrorCategory category, 
                   int line, const char* format, ...);

/**
 * Convenience function for informational messages
 */
void log_info(ErrorCategory category, int line, const char* format, ...);

/**
 * Convenience function for warnings
 */
void log_warning(ErrorCategory category, int line, const char* format, ...);

/**
 * Convenience function for errors (will increment error counter)
 */
void log_error(ErrorCategory category, int line, const char* format, ...);

/**
 * Get the current error count
 * 
 * @return Number of errors reported
 */
int get_error_count(void);

/**
 * Get the current warning count
 * 
 * @return Number of warnings reported
 */
int get_warning_count(void);

/**
 * Reset error and warning counters
 */
void reset_error_counters(void);

/**
 * Check if there are any errors that should stop compilation
 * 
 * @return 1 if errors exist, 0 otherwise
 */
int has_errors(void);

/**
 * Enable or disable colored output
 * 
 * @param enable 1 to enable colors, 0 to disable
 */
void set_colored_output(int enable);

#endif // ERROR_HANDLER_H

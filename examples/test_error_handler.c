/**
 * Comprehensive demonstration of the error handling system
 * Shows all features: colors, error codes, source context, hints, and suggestions
 */

#include "error_handler.h"
#include <stdio.h>

int main(void)
{
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║         Enhanced Error Handler System Demonstration           ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    // ===== 1. Basic Messages (Legacy Interface) =====
    printf("1. BASIC MESSAGES (Legacy Interface)\n");
    printf("────────────────────────────────────────────────────────────────\n");
    log_info(ERROR_CATEGORY_GENERAL, 0, "Compilation started");
    log_warning(ERROR_CATEGORY_SEMANTIC, 42, "Implicit type conversion");
    log_error(ERROR_CATEGORY_PARSER, 15, "Expected ';' but found '}'");
    
    printf("\n");
    
    // ===== 2. Extended Messages with Filename and Location =====
    printf("2. EXTENDED MESSAGES WITH FILENAME AND LOCATION\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc1 = {
        .filename = "src/main.c",
        .line = 25,
        .column = 10,
        .source_line = NULL
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc1, "E0001",
                     "Unexpected token '{'");
    
    printf("\n");
    
    // ===== 3. Source Context Display =====
    printf("3. SOURCE CONTEXT DISPLAY WITH COLUMN INDICATOR\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc2 = {
        .filename = "src/calculator.c",
        .line = 42,
        .column = 18,
        .source_line = "int result = x + y * z"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_SEMANTIC, &loc2, "E0100",
                     "Variable 'y' not declared in this scope");
    
    printf("\n");
    
    // ===== 4. Multi-line Errors with Hints =====
    printf("4. MULTI-LINE ERROR WITH HELPFUL HINTS\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc3 = {
        .filename = "src/functions.c",
        .line = 10,
        .column = 5,
        .source_line = "foo(x, y, z"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc3, "E0025",
                     "Missing closing parenthesis in function call");
    add_error_hint("Add ')' at the end of the function call");
    add_error_hint("Check for balanced parentheses in the expression");
    
    printf("\n");
    
    // ===== 5. Error Recovery Suggestions =====
    printf("5. ERROR WITH 'DID YOU MEAN?' SUGGESTION\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc4 = {
        .filename = "src/output.c",
        .line = 33,
        .column = 5,
        .source_line = "print(\"Hello, World!\");"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_SEMANTIC, &loc4, "E0150",
                     "Undeclared function 'print'");
    add_suggestion("printf");
    
    printf("\n");
    
    // ===== 6. Complete Error Report (All Features) =====
    printf("6. COMPLETE ERROR REPORT (ALL FEATURES COMBINED)\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc5 = {
        .filename = "src/control_flow.c",
        .line = 99,
        .column = 5,
        .source_line = "retrun result;"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc5, "E0200",
                     "Unknown keyword 'retrun'");
    add_error_hint("Check the spelling of the keyword");
    add_error_hint("Keywords are case-sensitive in C");
    add_suggestion("return");
    
    printf("\n");
    
    // ===== 7. Warning with Source Context =====
    printf("7. WARNING WITH SOURCE CONTEXT\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc6 = {
        .filename = "src/variables.c",
        .line = 12,
        .column = 9,
        .source_line = "int x = 100000000000;"
    };
    report_message_ex(SEVERITY_WARNING, ERROR_CATEGORY_SEMANTIC, &loc6, "W0050",
                     "Integer constant overflow");
    add_error_hint("Consider using 'long long' for large integer values");
    
    printf("\n");
    
    // ===== 8. Multiple Errors of Different Categories =====
    printf("8. MULTIPLE ERRORS FROM DIFFERENT CATEGORIES\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc7 = {
        .filename = "src/tokens.c",
        .line = 5,
        .column = 12,
        .source_line = "int 123abc = 5;"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_LEXER, &loc7, "E0001",
                     "Invalid identifier: cannot start with a digit");
    
    ErrorLocation loc8 = {
        .filename = "src/parser.c",
        .line = 88,
        .column = 1,
        .source_line = "}"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc8, "E0075",
                     "Unexpected closing brace");
    add_error_hint("Check for matching opening brace");
    
    ErrorLocation loc9 = {
        .filename = "src/types.c",
        .line = 50,
        .column = 14,
        .source_line = "char* str = 42;"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_SEMANTIC, &loc9, "E0300",
                     "Type mismatch: cannot assign 'int' to 'char*'");
    add_error_hint("Use a string literal like \"42\" for char* assignment");
    
    printf("\n");
    
    // ===== 9. Info Messages with Extended Format =====
    printf("9. INFORMATIONAL MESSAGES (Progress Tracking)\n");
    printf("────────────────────────────────────────────────────────────────\n");
    
    ErrorLocation loc10 = {
        .filename = "src/module.c",
        .line = 0,
        .column = 0,
        .source_line = NULL
    };
    report_message_ex(SEVERITY_INFO, ERROR_CATEGORY_CODEGEN, &loc10, NULL,
                     "Successfully generated VHDL entity 'processor'");
    
    log_info(ERROR_CATEGORY_GENERAL, 0, "Total functions parsed: 15");
    log_info(ERROR_CATEGORY_GENERAL, 0, "Total variables declared: 42");
    
    printf("\n");
    
    // ===== Summary =====
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                       COMPILATION SUMMARY                      ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("  📊 Errors:   %d\n", get_error_count());
    printf("  ⚠️  Warnings: %d\n", get_warning_count());
    printf("  ❌ Has errors that stop compilation: %s\n\n", 
           has_errors() ? "Yes" : "No");
    
    // ===== Test Colored Output Toggle =====
    printf("10. TESTING COLORED OUTPUT TOGGLE\n");
    printf("────────────────────────────────────────────────────────────────\n");
    printf("Disabling colors...\n");
    set_colored_output(0);
    
    ErrorLocation loc11 = {
        .filename = "test.c",
        .line = 1,
        .column = 1,
        .source_line = "error here"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_GENERAL, &loc11, "E9999",
                     "This error message has no colors");
    
    printf("\nRe-enabling colors...\n");
    set_colored_output(1);
    
    ErrorLocation loc12 = {
        .filename = "test.c",
        .line = 2,
        .column = 1,
        .source_line = "error here"
    };
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_GENERAL, &loc12, "E9999",
                     "This error message is colorful again!");
    
    printf("\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("                    Demo Complete! ✨\n");
    printf("════════════════════════════════════════════════════════════════\n");
    
    return 0;
}


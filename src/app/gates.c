#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "parse.h"
#include "codegen_vhdl.h"
#include "error_handler.h"
#include "utils.h"

// Minimum: program name + input file + output file
#define MIN_ARGC 3

#ifndef GATES_VERSION
#define GATES_VERSION "unknown"
#endif


int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("gates %s\n", GATES_VERSION);
        return EXIT_SUCCESS;
    }

    if (argc < MIN_ARGC) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Usage: %s <input.c> <output.vhdl>", argv[0]);
        return EXIT_FAILURE;
    }

    // Reject anything that is not a regular file before opening it. fopen()
    // succeeds on a directory, reads then fail with EISDIR, and the lexer sees
    // an immediate EOF that looks like a valid empty translation unit.
    struct stat input_info;
    if (stat(argv[1], &input_info) != 0) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Error opening input file '%s': %s", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }
    if (!S_ISREG(input_info.st_mode)) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Input '%s' is not a regular file", argv[1]);
        return EXIT_FAILURE;
    }

    // Open input file
    FILE *fin = fopen(argv[1], "r");
    if (!fin) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Error opening input file '%s': %s", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    log_info(ERROR_CATEGORY_GENERAL, 0, "Parsing input file '%s'...", argv[1]);

    // Parse the program and build the AST
    ASTNode *program = parse_program(fin);
    fclose(fin);

    #ifdef DEBUG
        print_ast(program, 0);
    #endif

    // Check for parse errors
    if (!program || has_errors()) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Parsing failed with %d error(s)", get_error_count());
        if (program) {
            free_node(program);
        }
        return EXIT_FAILURE;
    }

    // The output file is opened only once parsing has succeeded. Opening it
    // with "w" truncates immediately, so doing it earlier destroyed a good
    // previous output whenever a later compilation failed.
    FILE *fout = fopen(argv[2], "w");
    if (!fout) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Error opening output file '%s': %s", argv[2], strerror(errno));
        free_node(program);
        return EXIT_FAILURE;
    }

    // Generate VHDL code from the AST
    log_info(ERROR_CATEGORY_GENERAL, 0, "Generating VHDL code...");
    generate_vhdl(program, fout);
    free_node(program);

    // Output is fully buffered, so a write failure such as ENOSPC surfaces
    // here rather than at any earlier call
    if (fclose(fout) != 0) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Error writing output file '%s': %s", argv[2], strerror(errno));
        return EXIT_FAILURE;
    }

    // Code generation reports diagnostics through the same error handler as the
    // parser, so the exit status has to be re-checked here. Without this, a
    // codegen failure is printed and then reported as a successful compilation.
    if (has_errors()) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Code generation failed with %d error(s)", get_error_count());
        return EXIT_FAILURE;
    }

    log_info(ERROR_CATEGORY_GENERAL, 0, "Compilation finished successfully.");
    return EXIT_SUCCESS;
}

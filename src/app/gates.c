#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "codegen_vhdl.h"
#include "error_handler.h"
#include "utils.h"

// Minimum: program name + input file + output file
#define MIN_ARGC 3


int main(int argc, char *argv[])
{
    if (argc < MIN_ARGC) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Usage: %s <input.c> <output.vhdl>", argv[0]);
        return EXIT_FAILURE;
    }

    // Open input file
    FILE *fin = fopen(argv[1], "r");
    if (!fin) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Error opening input file '%s': %s", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    // Open output file
    FILE *fout = fopen(argv[2], "w");
    if (!fout) {
        log_error(ERROR_CATEGORY_GENERAL, 0,
                  "Error opening output file '%s': %s", argv[2], strerror(errno));
        fclose(fin);
        return EXIT_FAILURE;
    }

    log_info(ERROR_CATEGORY_GENERAL, 0, "Parsing input file '%s'...", argv[1]);

    // Parse the program and build the AST
    ASTNode *program = parse_program(fin);

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
        fclose(fin);
        fclose(fout);
        return EXIT_FAILURE;
    }

    // Generate VHDL code from the AST
    log_info(ERROR_CATEGORY_GENERAL, 0, "Generating VHDL code...");
    generate_vhdl(program, fout);
    free_node(program);

    fclose(fin);
    fclose(fout);

    log_info(ERROR_CATEGORY_GENERAL, 0, "Compilation finished successfully.");
    return EXIT_SUCCESS;
}

/**
 * @file integration_tests.cpp
 * @brief End-to-end C → VHDL integration tests.
 *
 * Each test writes a C snippet to a temp file, parses it into an AST,
 * generates VHDL, and verifies expected patterns in the output.
 */

#include <gtest/gtest.h>
extern "C" {
#include "astnode.h"
#include "parse.h"
#include "codegen_vhdl.h"
#include "token.h"
#include "error_handler.h"
#include "symbol_arrays.h"
#include "symbol_structs.h"
}
#include <cstdio>
#include <cstring>
#include <string>

// Reset global state before each integration test
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    /**
     * Helper: Parse C source and generate VHDL, returning the output as a string.
     * Returns empty string if parsing fails.
     */
    std::string translate(const char *c_source) {
        FILE *fin = tmpfile();
        if (!fin) return "";
        fwrite(c_source, 1, strlen(c_source), fin);
        rewind(fin);

        ASTNode *program = parse_program(fin);
        fclose(fin);
        if (!program) return "";

        FILE *fout = tmpfile();
        if (!fout) { free_node(program); return ""; }

        generate_vhdl(program, fout);
        free_node(program);

        // Read back the output
        fseek(fout, 0, SEEK_END);
        long size = ftell(fout);
        rewind(fout);
        std::string result(size, '\0');
        size_t bytes_read = fread(&result[0], 1, size, fout);
        (void)bytes_read;
        fclose(fout);
        return result;
    }
};

// -------------------------------------------------------------------
// Basic function → entity/architecture mapping
// -------------------------------------------------------------------
TEST_F(IntegrationTest, SimpleFunctionProducesEntity) {
    const char *src = "int add(int a, int b) { return a + b; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("entity"), std::string::npos);
    EXPECT_NE(vhdl.find("architecture"), std::string::npos);
    EXPECT_NE(vhdl.find("end"), std::string::npos);
}

// -------------------------------------------------------------------
// Variable declaration and assignment
// -------------------------------------------------------------------
TEST_F(IntegrationTest, VariableDeclarationSignal) {
    const char *src = "int compute(int x) { int y = 5; return y; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // Should declare a signal for y
    EXPECT_NE(vhdl.find("signal"), std::string::npos);
}

// -------------------------------------------------------------------
// If/else → VHDL if/elsif/else
// -------------------------------------------------------------------
TEST_F(IntegrationTest, IfElseControlFlow) {
    const char *src =
        "int check(int x) {"
        "  if (x == 0) { return 1; }"
        "  else { return 0; }"
        "}";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("if"), std::string::npos);
    EXPECT_NE(vhdl.find("else"), std::string::npos);
    EXPECT_NE(vhdl.find("end if"), std::string::npos);
}

// -------------------------------------------------------------------
// While loop → VHDL while loop
// -------------------------------------------------------------------
TEST_F(IntegrationTest, WhileLoop) {
    const char *src =
        "int count(int n) {"
        "  int i = 0;"
        "  while (i < n) { i = i + 1; }"
        "  return i;"
        "}";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("while"), std::string::npos);
}

// -------------------------------------------------------------------
// For loop translation
// -------------------------------------------------------------------
TEST_F(IntegrationTest, ForLoop) {
    const char *src =
        "int sum(int n) {"
        "  int s = 0;"
        "  for (int i = 0; i < n; i++) { s = s + i; }"
        "  return s;"
        "}";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // For loops typically translate to while-based constructs in VHDL
    EXPECT_NE(vhdl.find("<="), std::string::npos);
}

// -------------------------------------------------------------------
// Array declaration and access
// -------------------------------------------------------------------
TEST_F(IntegrationTest, ArrayDeclaration) {
    const char *src =
        "int arrtest(int x) {"
        "  int arr[4] = {1, 2, 3, 4};"
        "  arr[0] = x;"
        "  return arr[0];"
        "}";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("arr"), std::string::npos);
}

// -------------------------------------------------------------------
// Function call inside expression
// -------------------------------------------------------------------
TEST_F(IntegrationTest, FunctionCallInExpression) {
    const char *src =
        "int helper(int a) { return a + 1; }"
        "int caller(int x) { int y = helper(x); return y; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // Both functions should appear as entities or architectures
    EXPECT_NE(vhdl.find("helper"), std::string::npos);
}

// -------------------------------------------------------------------
// Struct definition and usage
// -------------------------------------------------------------------
TEST_F(IntegrationTest, StructDefinition) {
    const char *src =
        "struct Point { int x; int y; };"
        "int getx(int a) {"
        "  struct Point p = {1, 2};"
        "  return p.x;"
        "}";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // Struct should generate a record type
    EXPECT_NE(vhdl.find("record"), std::string::npos);
}

// -------------------------------------------------------------------
// Multiple functions in one file
// -------------------------------------------------------------------
TEST_F(IntegrationTest, MultipleFunctions) {
    const char *src =
        "int first(int a) { return a + 1; }"
        "int second(int b) { return b + 2; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // Should find multiple entity/architecture blocks
    size_t first_entity = vhdl.find("entity");
    ASSERT_NE(first_entity, std::string::npos);
    size_t second_entity = vhdl.find("entity", first_entity + 1);
    EXPECT_NE(second_entity, std::string::npos);
}

// -------------------------------------------------------------------
// Binary operators translate to VHDL equivalents
// -------------------------------------------------------------------
TEST_F(IntegrationTest, BinaryOperators) {
    const char *src = "int ops(int a, int b) { return a + b; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // VHDL should contain the addition
    EXPECT_NE(vhdl.find("+"), std::string::npos);
}

// -------------------------------------------------------------------
// Void function
// -------------------------------------------------------------------
TEST_F(IntegrationTest, VoidFunction) {
    const char *src = "void setval(int x) { x = 42; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("entity"), std::string::npos);
}

// -------------------------------------------------------------------
// IEEE library inclusion
// -------------------------------------------------------------------
TEST_F(IntegrationTest, IEEELibraryIncluded) {
    const char *src = "int simple(int x) { return x; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("IEEE"), std::string::npos);
    EXPECT_NE(vhdl.find("STD_LOGIC"), std::string::npos);
}

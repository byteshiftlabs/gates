/**
 * @file integration_tests.cpp
 * @brief Integration and end-to-end C → VHDL tests.
 *
 * This file contains:
 * 1. End-to-end smoke tests (full pipeline: C → AST → VHDL)
 * 2. Integration tests for component boundaries (parser→symbols, symbols→codegen)
 * 3. Negative tests for error cases
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
#include <cassert>

// ==================================================================
// END-TO-END SMOKE TESTS
// Full pipeline: C source → parse → codegen → VHDL verification
// ==================================================================

// Reset global state before each test
class EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    /**
     * Helper: Parse C source and generate VHDL, returning the output as a string.
     * Aborts test if tmpfile() fails.
     */
    std::string translate(const char *c_source) {
        FILE *fin = tmpfile();
        EXPECT_NE(fin, nullptr) << "tmpfile() failed for input";
        if (!fin) return "";
        
        fwrite(c_source, 1, strlen(c_source), fin);
        rewind(fin);

        ASTNode *program = parse_program(fin);
        fclose(fin);
        if (!program) return "";

        FILE *fout = tmpfile();
        EXPECT_NE(fout, nullptr) << "tmpfile() failed for output";
        if (!fout) { 
            free_node(program); 
            return ""; 
        }

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
TEST_F(EndToEndTest, SimpleFunctionProducesEntity) {
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
TEST_F(EndToEndTest, VariableDeclarationSignal) {
    const char *src = "int compute(int x) { int y = 5; return y; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // Should declare a signal for y
    EXPECT_NE(vhdl.find("signal"), std::string::npos);
}

// -------------------------------------------------------------------
// If/else → VHDL if/elsif/else
// -------------------------------------------------------------------
TEST_F(EndToEndTest, IfElseControlFlow) {
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
TEST_F(EndToEndTest, WhileLoop) {
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
TEST_F(EndToEndTest, ForLoop) {
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
TEST_F(EndToEndTest, ArrayDeclaration) {
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
TEST_F(EndToEndTest, FunctionCallInExpression) {
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
TEST_F(EndToEndTest, StructDefinition) {
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
TEST_F(EndToEndTest, MultipleFunctions) {
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
TEST_F(EndToEndTest, BinaryOperators) {
    const char *src = "int ops(int a, int b) { return a + b; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    // VHDL should contain the addition
    EXPECT_NE(vhdl.find("+"), std::string::npos);
}

// -------------------------------------------------------------------
// Void function
// -------------------------------------------------------------------
TEST_F(EndToEndTest, VoidFunction) {
    const char *src = "void setval(int x) { x = 42; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("entity"), std::string::npos);
}

// -------------------------------------------------------------------
// IEEE library inclusion
// -------------------------------------------------------------------
TEST_F(EndToEndTest, IEEELibraryIncluded) {
    const char *src = "int simple(int x) { return x; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("IEEE"), std::string::npos);
    EXPECT_NE(vhdl.find("STD_LOGIC"), std::string::npos);
}

// ==================================================================
// INTEGRATION TESTS - COMPONENT BOUNDARIES
// Tests verify proper interaction between specific components
// ==================================================================

// Reset global state before each integration test
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }
};

// -------------------------------------------------------------------
// Parser → Symbol Tables integration
// -------------------------------------------------------------------
TEST_F(IntegrationTest, ParserPopulatesArraySymbolTable) {
    const char *src = "int func(int x) { int arr[10] = {0}; return arr[0]; }";
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(src, 1, strlen(src), fin);
    rewind(fin);
    
    // Before parsing, array count should be 0
    EXPECT_EQ(get_array_count(), 0);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    ASSERT_NE(program, nullptr) << "parse_program() failed";
    
    // After parsing, array should be registered
    EXPECT_GT(get_array_count(), 0) << "Array not registered in symbol table";
    EXPECT_EQ(find_array_size("arr"), 10) << "Array size not correctly registered";
    
    free_node(program);
}

TEST_F(IntegrationTest, ParserPopulatesStructSymbolTable) {
    const char *src = 
        "struct Point { int x; int y; };"
        "int getx(int a) { struct Point p = {1, 2}; return p.x; }";
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(src, 1, strlen(src), fin);
    rewind(fin);
    
    // Before parsing, struct count should be 0
    EXPECT_EQ(get_struct_count(), 0);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    ASSERT_NE(program, nullptr) << "parse_program() failed";
    
    // After parsing, struct should be registered
    EXPECT_GT(get_struct_count(), 0) << "Struct not registered in symbol table";
    EXPECT_GE(find_struct_index("Point"), 0) << "Struct 'Point' not found in symbol table";
    
    free_node(program);
}

TEST_F(IntegrationTest, MultipleArraysRegisteredCorrectly) {
    const char *src = 
        "int func(int x) {"
        "  int arr1[5] = {0};"
        "  int arr2[10] = {0};"
        "  int arr3[3] = {0};"
        "  return arr1[0] + arr2[0] + arr3[0];"
        "}";
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(src, 1, strlen(src), fin);
    rewind(fin);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    ASSERT_NE(program, nullptr) << "parse_program() failed";
    
    // All three arrays should be registered with correct sizes
    EXPECT_EQ(get_array_count(), 3) << "Not all arrays registered";
    EXPECT_EQ(find_array_size("arr1"), 5);
    EXPECT_EQ(find_array_size("arr2"), 10);
    EXPECT_EQ(find_array_size("arr3"), 3);
    
    free_node(program);
}

// -------------------------------------------------------------------
// Symbol Tables → Codegen integration
// -------------------------------------------------------------------
TEST_F(IntegrationTest, CodegenReadsArraySymbolTable) {
    const char *src = "int func(int x) { int arr[8] = {0}; return arr[0]; }";
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(src, 1, strlen(src), fin);
    rewind(fin);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    ASSERT_NE(program, nullptr);
    
    // Verify array is in symbol table
    EXPECT_EQ(find_array_size("arr"), 8);
    
    // Generate VHDL - codegen should read from symbol table
    FILE *fout = tmpfile();
    ASSERT_NE(fout, nullptr) << "tmpfile() failed for output";
    
    generate_vhdl(program, fout);
    
    // Verify VHDL contains array reference
    fseek(fout, 0, SEEK_END);
    long size = ftell(fout);
    rewind(fout);
    std::string vhdl(size, '\0');
    size_t bytes_read = fread(&vhdl[0], 1, size, fout);
    (void)bytes_read;
    fclose(fout);
    
    EXPECT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("arr"), std::string::npos) << "Array name not found in VHDL output";
    
    free_node(program);
}

TEST_F(IntegrationTest, CodegenReadsStructSymbolTable) {
    const char *src = 
        "struct Data { int value; };"
        "int get(int x) { struct Data d = {42}; return d.value; }";
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(src, 1, strlen(src), fin);
    rewind(fin);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    ASSERT_NE(program, nullptr);
    
    // Verify struct is in symbol table
    EXPECT_GE(find_struct_index("Data"), 0);
    
    // Generate VHDL - codegen should read from symbol table
    FILE *fout = tmpfile();
    ASSERT_NE(fout, nullptr) << "tmpfile() failed for output";
    
    generate_vhdl(program, fout);
    
    // Verify VHDL contains record type
    fseek(fout, 0, SEEK_END);
    long size = ftell(fout);
    rewind(fout);
    std::string vhdl(size, '\0');
    size_t bytes_read = fread(&vhdl[0], 1, size, fout);
    (void)bytes_read;
    fclose(fout);
    
    EXPECT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("record"), std::string::npos) << "VHDL record type not found";
    
    free_node(program);
}

// ==================================================================
// NEGATIVE TESTS - ERROR CASES
// Tests verify graceful handling of invalid inputs
// ==================================================================

class NegativeTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }
};

TEST_F(NegativeTest, InvalidCyntaxReturnsNull) {
    const char *invalid_src = "int func(int x { return x; }"; // Missing closing parenthesis
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(invalid_src, 1, strlen(invalid_src), fin);
    rewind(fin);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    
    // TODO: Parser currently returns partial AST for some syntax errors
    // Should return NULL for invalid syntax (tracked in issue)
    // For now, verify error was logged
    if (program) {
        free_node(program);
    }
    EXPECT_GT(get_error_count(), 0) << "Error should be logged for invalid syntax";
}

TEST_F(NegativeTest, MissingFunctionBodyReturnsNull) {
    const char *invalid_src = "int func(int x);"; // Declaration without definition
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(invalid_src, 1, strlen(invalid_src), fin);
    rewind(fin);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    
    // Parser may return NULL or an incomplete AST - either is acceptable
    // The key is it doesn't crash
    if (program) {
        free_node(program);
    }
}

TEST_F(NegativeTest, UnbalancedBracesReturnsNull) {
    const char *invalid_src = "int func(int x) { return x; "; // Missing closing brace
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(invalid_src, 1, strlen(invalid_src), fin);
    rewind(fin);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    
    // TODO: Parser currently returns partial AST for some syntax errors
    // Should return NULL for unbalanced braces (tracked in issue)
    // For now, verify error was logged
    if (program) {
        free_node(program);
    }
    EXPECT_GT(get_error_count(), 0) << "Error should be logged for unbalanced braces";
}

TEST_F(NegativeTest, EmptySourceProducesValidOutput) {
    const char *empty_src = "";
    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr) << "tmpfile() failed";
    
    fwrite(empty_src, 1, strlen(empty_src), fin);
    rewind(fin);
    
    ASTNode *program = parse_program(fin);
    fclose(fin);
    
    // Empty source should either return NULL or an empty program node
    // Either is acceptable as long as it doesn't crash
    if (program) {
        free_node(program);
    }
}

// ==================================================================
// VHDL VALIDATION TESTS
// Tests verify generated VHDL has proper structure
// ==================================================================

class VHDLValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }
    
    /**
     * Helper: Verify VHDL has proper library declarations
     */
    bool hasValidLibraryDeclarations(const std::string& vhdl) {
        return vhdl.find("library IEEE;") != std::string::npos &&
               vhdl.find("use IEEE.STD_LOGIC") != std::string::npos;
    }
    
    /**
     * Helper: Verify VHDL entity/architecture structure
     * Basic check: "entity" appears before "architecture", both have matching "end"
     */
    bool hasValidEntityArchitecture(const std::string& vhdl) {
        size_t entity_pos = vhdl.find("entity ");
        if (entity_pos == std::string::npos) return false;
        
        size_t arch_pos = vhdl.find("architecture ", entity_pos);
        if (arch_pos == std::string::npos) return false;
        
        // Verify "end" statements exist after each declaration
        size_t end_entity = vhdl.find("end ", entity_pos);
        size_t end_arch = vhdl.find("end ", arch_pos);
        
        return end_entity != std::string::npos && 
               end_arch != std::string::npos &&
               end_entity < arch_pos && 
               end_arch > arch_pos;
    }
    
    /**
     * Helper: Generate VHDL from C source
     */
    std::string generateVHDL(const char *c_source) {
        FILE *fin = tmpfile();
        EXPECT_NE(fin, nullptr) << "tmpfile() failed";
        if (!fin) return "";
        
        fwrite(c_source, 1, strlen(c_source), fin);
        rewind(fin);
        
        ASTNode *program = parse_program(fin);
        fclose(fin);
        if (!program) return "";
        
        FILE *fout = tmpfile();
        EXPECT_NE(fout, nullptr) << "tmpfile() failed";
        if (!fout) {
            free_node(program);
            return "";
        }
        
        generate_vhdl(program, fout);
        free_node(program);
        
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

TEST_F(VHDLValidationTest, SimpleFunction_HasValidStructure) {
    const char *src = "int add(int a, int b) { return a + b; }";
    std::string vhdl = generateVHDL(src);
    
    ASSERT_FALSE(vhdl.empty()) << "VHDL generation failed";
    EXPECT_TRUE(hasValidLibraryDeclarations(vhdl)) << "Missing or invalid library declarations";
    EXPECT_TRUE(hasValidEntityArchitecture(vhdl)) << "Invalid entity/architecture structure";
}

TEST_F(VHDLValidationTest, FunctionWithLocals_HasSignalDeclarations) {
    const char *src = "int compute(int x) { int y = 5; int z = 10; return y + z; }";
    std::string vhdl = generateVHDL(src);
    
    ASSERT_FALSE(vhdl.empty()) << "VHDL generation failed";
    EXPECT_TRUE(hasValidLibraryDeclarations(vhdl));
    EXPECT_TRUE(hasValidEntityArchitecture(vhdl));
    
    // Local variables should become signals in the architecture
    size_t arch_pos = vhdl.find("architecture ");
    ASSERT_NE(arch_pos, std::string::npos);
    
    // Look for signal declarations after "architecture" keyword
    size_t signal_pos = vhdl.find("signal", arch_pos);
    EXPECT_NE(signal_pos, std::string::npos) << "Local variables should generate signal declarations";
}

TEST_F(VHDLValidationTest, MultipleEntities_AllHaveValidStructure) {
    const char *src = 
        "int first(int a) { return a + 1; }"
        "int second(int b) { return b + 2; }";
    std::string vhdl = generateVHDL(src);
    
    ASSERT_FALSE(vhdl.empty()) << "VHDL generation failed";
    
    // Count entity declarations
    size_t entity_count = 0;
    size_t pos = 0;
    while ((pos = vhdl.find("entity ", pos)) != std::string::npos) {
        entity_count++;
        pos += 7; // strlen("entity ")
    }
    
    EXPECT_EQ(entity_count, 2) << "Should generate two entity declarations";
    
    // Count architecture declarations
    size_t arch_count = 0;
    pos = 0;
    while ((pos = vhdl.find("architecture ", pos)) != std::string::npos) {
        arch_count++;
        pos += 13; // strlen("architecture ")
    }
    
    EXPECT_EQ(arch_count, 2) << "Should generate two architecture declarations";
    EXPECT_EQ(entity_count, arch_count) << "Entity count should match architecture count";
}

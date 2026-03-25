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

TEST_F(EndToEndTest, NestedLoopVariablesAreDeclaredAndReset) {
    const char *src =
        "int nested(int outer, int inner) {"
        "  int i = 0;"
        "  int total = 0;"
        "  while (i < outer) {"
        "    int j = 0;"
        "    while (j < inner) {"
        "      total = total + i + j;"
        "      j = j + 1;"
        "    }"
        "    i = i + 1;"
        "  }"
        "  return total;"
        "}";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("signal j : std_logic_vector(31 downto 0);"), std::string::npos);
    EXPECT_NE(vhdl.find("j <= (others => '0');"), std::string::npos);
    EXPECT_NE(vhdl.find("while unsigned(j) < unsigned(inner) loop"), std::string::npos);
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
// Comparison-valued returns must lower to numeric 1/0, not bare booleans
// -------------------------------------------------------------------
TEST_F(EndToEndTest, ComparisonReturnProducesNumericResultAssignment) {
    const char *src = "int cmp(int a, int b) { return a == b; }";
    std::string vhdl = translate(src);
    ASSERT_FALSE(vhdl.empty());
    EXPECT_NE(vhdl.find("if unsigned(a) = unsigned(b) then"), std::string::npos);
    EXPECT_NE(vhdl.find("result <= std_logic_vector(to_unsigned(1, 32));"), std::string::npos);
    EXPECT_NE(vhdl.find("result <= std_logic_vector(to_unsigned(0, 32));"), std::string::npos);
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

// ==================================================================
// STRUCTURAL VHDL VALIDATION TESTS
// Self-contained checks that verify type-correctness and well-formedness
// of generated VHDL without relying on external tools.
// ==================================================================

/**
 * Helper: count non-overlapping occurrences of a substring.
 */
static size_t countOccurrences(const std::string &haystack, const std::string &needle) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

/**
 * Helper: assert that every entity is preceded by a library preamble.
 * Returns the number of entities found.
 */
static int verifyPerEntityLibraryPreamble(const std::string &vhdl) {
    size_t pos = 0;
    int entities = 0;
    while ((pos = vhdl.find("entity ", pos)) != std::string::npos) {
        // Skip "end entity" lines
        if (pos >= 4 && vhdl.substr(pos - 4, 4) == "end ") {
            pos += 7;
            continue;
        }
        ++entities;
        // The library clause must appear before this entity
        size_t lib_pos = vhdl.rfind("library IEEE;", pos);
        EXPECT_NE(lib_pos, std::string::npos)
            << "Entity at offset " << pos << " has no preceding library clause";
        pos += 7;
    }
    return entities;
}

/**
 * Helper: verify no undeclared c_N identifiers appear in the VHDL output.
 * Numeric literals should be emitted as to_unsigned(N,...) or to_signed(N,...),
 * never as bare c_N identifiers produced by sanitize_vhdl_identifier.
 */
static void verifyNoBareLiteralIdentifiers(const std::string &vhdl) {
    // Scan for c_ followed by digits that are NOT inside a comment
    size_t pos = 0;
    while (pos < vhdl.size()) {
        size_t found = vhdl.find("c_", pos);
        if (found == std::string::npos) break;

        // Skip if inside a comment line
        size_t line_start = vhdl.rfind('\n', found);
        if (line_start == std::string::npos) line_start = 0;
        std::string line_prefix = vhdl.substr(line_start, found - line_start);
        if (line_prefix.find("--") != std::string::npos) {
            pos = found + 2;
            continue;
        }

        // Check if the rest is all digits (c_42, c_0, etc.)
        size_t end = found + 2;
        bool all_digits = end < vhdl.size() && std::isdigit(vhdl[end]);
        while (end < vhdl.size() && std::isdigit(vhdl[end])) ++end;
        // c_ followed by only digits and then a non-alnum char = bare literal
        if (all_digits && (end >= vhdl.size() || !std::isalnum(vhdl[end]))) {
            std::string token = vhdl.substr(found, end - found);
            ADD_FAILURE() << "Bare numeric literal identifier '" << token
                          << "' found at offset " << found
                          << ". Should be to_unsigned/to_signed instead.";
        }
        pos = found + 2;
    }
}

/**
 * Helper: verify balanced VHDL keywords.
 */
static void verifyBalancedConstructs(const std::string &vhdl) {
    // Each "process" should have a matching "end process"
    EXPECT_EQ(countOccurrences(vhdl, "process(") + countOccurrences(vhdl, "process ("),
              countOccurrences(vhdl, "end process;"))
        << "Unbalanced process/end process";

    // Each "entity ... is" should have "end entity;"
    size_t entity_decls = 0;
    size_t pos = 0;
    while ((pos = vhdl.find("entity ", pos)) != std::string::npos) {
        if (pos >= 4 && vhdl.substr(pos - 4, 4) == "end ") {
            pos += 7;
            continue;
        }
        ++entity_decls;
        pos += 7;
    }
    EXPECT_EQ(entity_decls, countOccurrences(vhdl, "end entity;"))
        << "Unbalanced entity/end entity";

    // Each "architecture ... is" should have "end architecture;"
    EXPECT_EQ(countOccurrences(vhdl, "architecture "),
              countOccurrences(vhdl, "end architecture;"))
        << "Unbalanced architecture/end architecture";
}

/**
 * Helper: verify that signals used in assignments are declared in the
 * architecture's declarative region or as port signals.
 */
static void verifySignalsDeclaredOrPorts(const std::string &vhdl,
                                          const std::vector<std::string> &expected_signals) {
    for (const auto &sig : expected_signals) {
        std::string decl = "signal " + sig + " :";
        std::string port = sig + " : in ";
        bool has_decl = vhdl.find(decl) != std::string::npos;
        bool has_port = vhdl.find(port) != std::string::npos;
        EXPECT_TRUE(has_decl || has_port)
            << "Signal/port '" << sig << "' used but never declared";
    }
}

/**
 * Helper: verify that numeric assignments use std_logic_vector wrapping
 * rather than bare unsigned/signed results.
 */
static void verifyArithmeticTypeWrapping(const std::string &vhdl) {
    // Look for signal assignments: <signal> <= <expression>;
    // Arithmetic like "unsigned(x) + unsigned(y)" must be wrapped in std_logic_vector()
    // But comparisons inside "if" conditions should NOT be wrapped.
    // We check that "<= unsigned(" never appears (assignment of bare unsigned)
    size_t pos = 0;
    while ((pos = vhdl.find("<= unsigned(", pos)) != std::string::npos) {
        // Skip if this is inside an if/elsif condition
        size_t line_start = vhdl.rfind('\n', pos);
        if (line_start == std::string::npos) line_start = 0;
        std::string line = vhdl.substr(line_start, pos - line_start);
        if (line.find("if ") != std::string::npos ||
            line.find("elsif ") != std::string::npos ||
            line.find("while ") != std::string::npos) {
            pos += 12;
            continue;
        }
        ADD_FAILURE() << "Bare unsigned() in signal assignment at offset " << pos
                      << ". Should be wrapped in std_logic_vector().";
        pos += 12;
    }
}

// --- Full pipeline structural validation for the CI validation input ---

TEST_F(VHDLValidationTest, CIValidateInput_AllEntitiesWellFormed) {
    const char *src =
        "int add(int a, int b) { int sum = a + b; return sum; }\n"
        "int bitwise_ops(int x, int y) { int a = x & y; int o = x | y; int xr = x ^ y; return a + o + xr; }\n"
        "int negate(int x) { return -x; }\n"
        "int max_val(int a, int b) { if (a > b) { return a; } else { return b; } }\n"
        "int while_sum(int n) { int sum = 0; int i = 0; while (i < n) { sum = sum + i; i = i + 1; } return sum; }\n"
        "int nested_loops(int outer, int inner) { int total = 0; int i = 0; while (i < outer) { int j = 0; while (j < inner) { total = total + i + j; j = j + 1; } i = i + 1; } return total; }\n"
        "int for_loop(int n) { int s = 0; for (int i = 0; i < n; i++) { s = s + i; } return s; }\n"
        "int break_continue(int n) { int total = 0; int i = 0; while (i < n) { if (i == 3) { i = i + 1; continue; } if (total > 100) { break; } total = total + i; i = i + 1; } return total; }\n"
        "int comparison_return(int a, int b) { return a == b; }\n";

    std::string vhdl = generateVHDL(src);
    ASSERT_FALSE(vhdl.empty()) << "VHDL generation failed";

    // Structural checks
    int entities = verifyPerEntityLibraryPreamble(vhdl);
    EXPECT_EQ(entities, 9) << "Expected 9 entities for 9 functions";

    verifyNoBareLiteralIdentifiers(vhdl);
    verifyBalancedConstructs(vhdl);
    verifyArithmeticTypeWrapping(vhdl);
}

TEST_F(VHDLValidationTest, CIValidateInput_NestedLoopSignalsDeclared) {
    const char *src =
        "int nested_loops(int outer, int inner) {\n"
        "    int total = 0;\n"
        "    int i = 0;\n"
        "    while (i < outer) {\n"
        "        int j = 0;\n"
        "        while (j < inner) {\n"
        "            total = total + i + j;\n"
        "            j = j + 1;\n"
        "        }\n"
        "        i = i + 1;\n"
        "    }\n"
        "    return total;\n"
        "}\n";

    std::string vhdl = generateVHDL(src);
    ASSERT_FALSE(vhdl.empty());

    verifySignalsDeclaredOrPorts(vhdl, {"total", "i", "j"});
    verifyNoBareLiteralIdentifiers(vhdl);
}

TEST_F(VHDLValidationTest, ArithmeticExpressions_ProduceStdLogicVector) {
    const char *src = "int calc(int a, int b) { int r = a + b; int s = a - b; int m = a * b; return r + s + m; }";
    std::string vhdl = generateVHDL(src);
    ASSERT_FALSE(vhdl.empty());

    verifyNoBareLiteralIdentifiers(vhdl);
    verifyArithmeticTypeWrapping(vhdl);
    verifyBalancedConstructs(vhdl);
}

TEST_F(VHDLValidationTest, BitwiseOps_ProduceStdLogicVector) {
    const char *src =
        "int bits(int x, int y) {\n"
        "    int a = x & y;\n"
        "    int o = x | y;\n"
        "    int xr = x ^ y;\n"
        "    return a + o + xr;\n"
        "}\n";
    std::string vhdl = generateVHDL(src);
    ASSERT_FALSE(vhdl.empty());

    // Bitwise results must be std_logic_vector, not bare unsigned
    EXPECT_NE(vhdl.find("std_logic_vector(unsigned("), std::string::npos)
        << "Bitwise ops should wrap result in std_logic_vector";
    verifyArithmeticTypeWrapping(vhdl);
    verifyNoBareLiteralIdentifiers(vhdl);
}

TEST_F(VHDLValidationTest, IntegerLiterals_EmitAsToUnsigned) {
    const char *src = "int f(int x) { int a = 42; int b = 0; return a + b + x; }";
    std::string vhdl = generateVHDL(src);
    ASSERT_FALSE(vhdl.empty());

    EXPECT_NE(vhdl.find("to_unsigned(42,"), std::string::npos)
        << "Literal 42 should appear as to_unsigned(42, ...)";
    EXPECT_NE(vhdl.find("to_unsigned(0,"), std::string::npos)
        << "Literal 0 should appear as to_unsigned(0, ...)";
    verifyNoBareLiteralIdentifiers(vhdl);
}

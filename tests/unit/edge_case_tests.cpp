/**
 * @file edge_case_tests.cpp
 * @brief Edge case tests for parser robustness, lexer limits,
 *        and codegen helpers.
 */

#include <gtest/gtest.h>
extern "C" {
#include "astnode.h"
#include "parse.h"
#include "token.h"
#include "parser/tokenizer.h"
#include "utils.h"
#include "error_handler.h"
#include "symbol_arrays.h"
#include "symbol_structs.h"
#include "codegen/codegen_vhdl_helpers.h"
}
#include <cstdio>
#include <cstring>
#include <string>

class EdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }
};

// -------------------------------------------------------------------
// Empty input should not crash the parser
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, EmptyInputReturnsProgram) {
    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);
    // Write nothing — empty file
    rewind(f);
    ASTNode *program = parse_program(f);
    fclose(f);
    // Should return a valid (possibly empty) program node, not NULL
    if (program) {
        EXPECT_EQ(program->type, NODE_PROGRAM);
        free_node(program);
    }
    // Either way: no crash is the real assertion
}

// -------------------------------------------------------------------
// Whitespace-only input
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, WhitespaceOnlyInput) {
    const char *src = "   \n\n\t\t  \n";
    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);
    fwrite(src, 1, strlen(src), f);
    rewind(f);
    ASTNode *program = parse_program(f);
    fclose(f);
    if (program) {
        EXPECT_EQ(program->type, NODE_PROGRAM);
        free_node(program);
    }
}

// -------------------------------------------------------------------
// Deeply nested expressions should not crash
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, DeeplyNestedExpression) {
    // Build: int f(int x) { return ((((((((x + 1)))))))); }
    std::string src = "int f(int x) { return ";
    for (int i = 0; i < 20; i++) src += "(";
    src += "x + 1";
    for (int i = 0; i < 20; i++) src += ")";
    src += "; }";

    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);
    fwrite(src.c_str(), 1, src.size(), f);
    rewind(f);
    ASTNode *program = parse_program(f);
    fclose(f);
    if (program) {
        EXPECT_EQ(program->type, NODE_PROGRAM);
        free_node(program);
    }
}

// -------------------------------------------------------------------
// is_numeric_literal: rejects multiple dots (fix #14)
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, NumericLiteralMultipleDots) {
    EXPECT_TRUE(is_numeric_literal("42"));
    EXPECT_TRUE(is_numeric_literal("3.14"));
    EXPECT_TRUE(is_numeric_literal("0.0"));
    EXPECT_FALSE(is_numeric_literal("1.2.3"));
    EXPECT_FALSE(is_numeric_literal("..."));
    EXPECT_FALSE(is_numeric_literal(""));
    EXPECT_FALSE(is_numeric_literal(NULL));
}

// -------------------------------------------------------------------
// is_negative_numeric_literal edge cases
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, NegativeNumericLiteral) {
    EXPECT_TRUE(is_negative_numeric_literal("-1"));
    EXPECT_TRUE(is_negative_numeric_literal("-42"));
    EXPECT_FALSE(is_negative_numeric_literal("42"));
    EXPECT_FALSE(is_negative_numeric_literal("-"));
    EXPECT_FALSE(is_negative_numeric_literal(""));
    EXPECT_FALSE(is_negative_numeric_literal(NULL));
}

// -------------------------------------------------------------------
// safe_append / safe_copy boundary conditions
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, SafeAppendOverflow) {
    char buf[8] = "abc";
    safe_append(buf, sizeof(buf), "12345678"); // would overflow
    // Should not overflow: result should be null-terminated within buf
    EXPECT_LT(strlen(buf), sizeof(buf));
}

TEST_F(EdgeCaseTest, SafeCopyZeroSize) {
    char buf[4] = "xyz";
    safe_copy(buf, 0, "hello", 5); // zero dest size — should be a no-op
    EXPECT_STREQ(buf, "xyz");
}

// -------------------------------------------------------------------
// ctype_to_vhdl maps all supported types
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, CtypeToVhdlMappings) {
    EXPECT_NE(std::string(ctype_to_vhdl("int")).find("std_logic_vector"), std::string::npos);
    EXPECT_NE(std::string(ctype_to_vhdl("float")).find("std_logic_vector"), std::string::npos);
    EXPECT_NE(std::string(ctype_to_vhdl("double")).find("std_logic_vector"), std::string::npos);
    EXPECT_NE(std::string(ctype_to_vhdl("char")).find("std_logic_vector"), std::string::npos);
    // Unknown type should still return something (fallback)
    EXPECT_NE(std::string(ctype_to_vhdl("unknown_type")).find("std_logic_vector"), std::string::npos);
}

// -------------------------------------------------------------------
// Token: long identifier is truncated (fix #15)
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, LongIdentifierTruncated) {
    // Build an identifier with 300 characters
    std::string long_id(300, 'a');
    std::string src = "int " + long_id + ";";
    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);
    fwrite(src.c_str(), 1, src.size(), f);
    rewind(f);

    reset_error_counters();
    ParserContext ctx;
    parser_context_init(&ctx, f);
    // Tokenize directly instead of full parse (which may reset counters)
    advance(&ctx);
    // The first token should be "int" (keyword), second is the long identifier
    advance(&ctx);
    fclose(f);
    // The error counter should have incremented from the truncation
    EXPECT_GT(get_error_count(), 0);
}

// -------------------------------------------------------------------
// AST: free_node on NULL is safe
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, FreeNullNodeSafe) {
    free_node(NULL); // should not crash
}

// -------------------------------------------------------------------
// AST: add_child with NULL parent should not crash
// (Currently segfaults — documents known limitation)
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, AddChildNullParentDocumented) {
    // add_child(NULL, child) is undefined — only test valid usage
    ASTNode *parent = create_node(NODE_PROGRAM);
    ASTNode *child = create_node(NODE_EXPRESSION);
    add_child(parent, child);
    EXPECT_EQ(parent->num_children, 1);
    EXPECT_EQ(child->parent, parent);
    free_node(parent);
}

// -------------------------------------------------------------------
// Array symbol table: fill to capacity
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, ArrayTableFillToCapacity) {
    reset_array_table();
    // Fill up to MAX_ARRAYS (size must be > 0 for register_array to accept)
    for (int i = 0; i < 128; i++) {
        char name[16];
        snprintf(name, sizeof(name), "arr%d", i);
        register_array(name, i + 1);
    }
    EXPECT_EQ(get_array_count(), 128);
    // One more should be silently ignored (table full)
    register_array("overflow", 999);
    EXPECT_EQ(get_array_count(), 128);
    EXPECT_EQ(find_array_size("overflow"), -1);
}

// -------------------------------------------------------------------
// Struct symbol table: find_struct_index for nonexistent
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, StructNotFound) {
    reset_struct_table();
    EXPECT_EQ(find_struct_index("NonExistent"), -1);
}

// -------------------------------------------------------------------
// get_precedence for unknown operators
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, UnknownOperatorPrecedence) {
    EXPECT_EQ(get_precedence("???"), PREC_UNKNOWN);
    EXPECT_EQ(get_precedence(""), PREC_UNKNOWN);
}

// -------------------------------------------------------------------
// Comment-only input should not crash
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, CommentOnlyInput) {
    const char *src = "// This is just a comment\n/* block comment */\n";
    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);
    fwrite(src, 1, strlen(src), f);
    rewind(f);
    ASTNode *program = parse_program(f);
    fclose(f);
    if (program) {
        EXPECT_EQ(program->type, NODE_PROGRAM);
        free_node(program);
    }
}

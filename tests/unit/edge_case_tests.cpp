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
#include "config.h"
#include "codegen_vhdl.h"
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
// AST: add_child with a NULL parent or child logs an error and returns
// instead of crashing (astnode.c guards both cases explicitly).
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, AddChildNullParentIsSafe) {
    ASTNode *child = create_node(NODE_EXPRESSION);
    add_child(NULL, child); // should log an error, not crash
    EXPECT_EQ(child->parent, nullptr);
    free_node(child);
}

TEST_F(EdgeCaseTest, AddChildNullChildIsSafe) {
    ASTNode *parent = create_node(NODE_PROGRAM);
    add_child(parent, NULL); // should log an error, not crash
    EXPECT_EQ(parent->num_children, 0);
    free_node(parent);
}

TEST_F(EdgeCaseTest, AddChildValidUsage) {
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

// -------------------------------------------------------------------
// emit_indent_inc() clamped at MAX_INDENT while emit_indent_dec() always
// decremented, so nesting deeper than the clamp unwound past zero and
// every later line lost its indentation.
// -------------------------------------------------------------------
TEST_F(EdgeCaseTest, DeepNestingDoesNotLoseIndentation) {
    const int depth = 20;   // MAX_INDENT is 16
    std::string src = "int f(int a) { int r; r = 0; ";
    for (int i = 0; i < depth; i++) src += "if (a) { ";
    src += "r = 1; ";
    for (int i = 0; i < depth; i++) src += "} ";
    src += "return r; }";

    FILE *fin = tmpfile();
    ASSERT_NE(fin, nullptr);
    fwrite(src.c_str(), 1, src.size(), fin);
    rewind(fin);
    ASTNode *program = parse_program(fin);
    fclose(fin);
    ASSERT_NE(program, nullptr);

    FILE *fout = tmpfile();
    ASSERT_NE(fout, nullptr);
    generate_vhdl(program, fout);
    free_node(program);

    fseek(fout, 0, SEEK_END);
    long size = ftell(fout);
    rewind(fout);
    std::string vhdl(static_cast<size_t>(size), '\0');
    size_t got = fread(&vhdl[0], 1, static_cast<size_t>(size), fout);
    (void)got;
    fclose(fout);

    EXPECT_NE(vhdl.find("  end process;"), std::string::npos)
        << "Closing lines must keep their indentation after deep nesting";
    EXPECT_EQ(vhdl.find("\nend process;"), std::string::npos)
        << "end process must not unwind to column zero";
}

TEST_F(EdgeCaseTest, SafeAppendHandlesZeroSizedDestination) {
    char buf[4] = "xyz";
    safe_append(buf, 0, "hello");   // must be a no-op, not an unbounded copy
    EXPECT_STREQ(buf, "xyz");
}


// -------------------------------------------------------------------
// for-header clauses desynchronised in two ways: a bare-expression init
// was backtracked over but never consumed, and the caller advanced past
// an extra semicolon whenever the condition clause was empty.
// -------------------------------------------------------------------
class ForHeaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    bool parses(const char *loop) {
        std::string src = "int f(int a) { int i; int r; i = 0; r = 0; ";
        src += loop;
        src += " return r; }";
        FILE *file = tmpfile();
        EXPECT_NE(file, nullptr);
        if (!file) return false;
        fwrite(src.c_str(), 1, src.size(), file);
        rewind(file);
        reset_error_counters();
        ASTNode *program = parse_program(file);
        fclose(file);
        const bool ok = (program != NULL) && (get_error_count() == 0);
        if (program) free_node(program);
        return ok;
    }
};

TEST_F(ForHeaderTest, BareExpressionInitIsConsumed) {
    EXPECT_TRUE(parses("for (i; i < 4; i = i + 1) { r = r + 1; }"));
}

TEST_F(ForHeaderTest, EmptyConditionWithNonEmptyInit) {
    EXPECT_TRUE(parses("for (i; ; i = i + 1) { r = 1; }"));
    EXPECT_TRUE(parses("for (int j = 0; ; j = j + 1) { r = 1; }"));
}

TEST_F(ForHeaderTest, ExistingFormsStillParse) {
    EXPECT_TRUE(parses("for (;;) { r = 1; }"));
    EXPECT_TRUE(parses("for (int j = 0; j < 4; j++) { r = r + 1; }"));
}


// ==================================================================
// CODEGEN BUFFER BOUNDS
// Long array names and index expressions used to overflow the fixed
// 64-byte codegen buffers (unclamped strncpy + out-of-bounds NUL).
// ==================================================================

class CodegenBufferBoundsTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    void translate(const std::string &c_source) {
        FILE *fin = tmpfile();
        ASSERT_NE(fin, nullptr) << "tmpfile() failed";
        fwrite(c_source.c_str(), 1, c_source.size(), fin);
        rewind(fin);

        ASTNode *program = parse_program(fin);
        fclose(fin);
        if (!program) return;

        FILE *fout = tmpfile();
        ASSERT_NE(fout, nullptr) << "tmpfile() failed";
        generate_vhdl(program, fout);
        fclose(fout);
        free_node(program);
    }
};

TEST_F(CodegenBufferBoundsTest, OverlongArrayIndexIsRejectedNotOverflowed) {
    // Index text far exceeds the 64-byte codegen buffer.
    std::string index = "i";
    for (int n = 0; n < 40; n++) index += " + i";
    translate("int f(int a) { int i; int arr[4]; i = 0; int x; x = arr[" +
              index + "]; return x; }");
    EXPECT_GT(get_error_count(), 0)
        << "An index expression longer than the buffer must be diagnosed";
}

TEST_F(CodegenBufferBoundsTest, OverlongArrayNameIsRejectedNotOverflowed) {
    const std::string name(80, 'b');
    translate("int f(int a) { int x; int " + name + "[4]; x = " + name +
              "[0]; return x; }");
    EXPECT_GT(get_error_count(), 0)
        << "An array name longer than the buffer must be diagnosed";
}

TEST_F(CodegenBufferBoundsTest, OverlongArrayNameOnAssignmentIsRejected) {
    const std::string name(80, 'a');
    translate("int f(int a) { int " + name + "[4]; " + name +
              "[0] = 1; return a; }");
    EXPECT_GT(get_error_count(), 0)
        << "An array name longer than the buffer must be diagnosed on assignment";
}

// ==================================================================
// RECURSION DEPTH
// The parser is recursive descent and the lexer recursed once per
// comment, so nested or comment-heavy input exhausted the stack.
// Sizes below are chosen to exceed the depth that used to segfault.
// ==================================================================

class RecursionDepthTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    // Parse without crashing; returns true when the parser reported an error.
    bool parseReportsError(const std::string &src) {
        FILE *file = tmpfile();
        EXPECT_NE(file, nullptr);
        if (!file) return false;
        fwrite(src.c_str(), 1, src.size(), file);
        rewind(file);

        ASTNode *program = parse_program(file);
        fclose(file);
        const bool reported = (get_error_count() > 0);
        if (program) free_node(program);
        return reported;
    }
};

TEST_F(RecursionDepthTest, DeeplyNestedParenthesesAreRejected) {
    const int depth = GATES_MAX_PARSE_DEPTH * 100;
    std::string src = "int f(int a) { int x; x = ";
    src.append(static_cast<size_t>(depth), '(');
    src += "a";
    src.append(static_cast<size_t>(depth), ')');
    src += "; return x; }";
    EXPECT_TRUE(parseReportsError(src));
}

TEST_F(RecursionDepthTest, DeeplyNestedStatementsAreRejected) {
    const int depth = GATES_MAX_PARSE_DEPTH * 50;
    std::string src = "int f(int a) { int r; r = 0; ";
    for (int i = 0; i < depth; i++) src += "if (a) { ";
    src += "r = 1; ";
    for (int i = 0; i < depth; i++) src += "} ";
    src += "return r; }";
    EXPECT_TRUE(parseReportsError(src));
}

TEST_F(RecursionDepthTest, NestingWithinTheLimitStillParses) {
    const int depth = GATES_MAX_PARSE_DEPTH / 2;
    std::string src = "int f(int a) { int x; x = ";
    src.append(static_cast<size_t>(depth), '(');
    src += "a";
    src.append(static_cast<size_t>(depth), ')');
    src += "; return x; }";
    EXPECT_FALSE(parseReportsError(src));
}

TEST_F(RecursionDepthTest, LongCommentRunDoesNotExhaustTheStack) {
    // The lexer used to consume one stack frame per skipped comment.
    std::string src;
    for (int i = 0; i < 80000; i++) src += "// c\n";
    src += "int f(int a) { return a; }";
    EXPECT_FALSE(parseReportsError(src));
}

// ==================================================================
// EMPTY CONDITIONS
// `if ()` and `while ()` used to crash: parse_expression returned NULL
// without logging, so codegen ran on a node with no children.
// ==================================================================

class EmptyConditionTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    // Run the full pipeline, returning true when the source was rejected
    // without crashing.
    bool rejects(const char *c_source) {
        FILE *fin = tmpfile();
        EXPECT_NE(fin, nullptr);
        if (!fin) return false;
        fwrite(c_source, 1, strlen(c_source), fin);
        rewind(fin);

        ASTNode *program = parse_program(fin);
        fclose(fin);

        const bool reported = (get_error_count() > 0);

        // Generate anyway: a malformed tree reaching codegen must not crash.
        if (program) {
            FILE *fout = tmpfile();
            EXPECT_NE(fout, nullptr);
            if (fout) {
                generate_vhdl(program, fout);
                fclose(fout);
            }
            free_node(program);
        }
        return reported;
    }
};

TEST_F(EmptyConditionTest, EmptyWhileConditionIsRejected) {
    EXPECT_TRUE(rejects("int f(int a) { while () { } return a; }"));
}

TEST_F(EmptyConditionTest, EmptyIfConditionIsRejected) {
    EXPECT_TRUE(rejects("int f(int a) { if () { } return a; }"));
}

TEST_F(EmptyConditionTest, EmptyElseIfConditionIsRejected) {
    EXPECT_TRUE(rejects("int f(int a) { if (a) { } else if () { } return a; }"));
}

// -------------------------------------------------------------------
// Content between an initializer and the semicolon used to be discarded
// silently, so a second declarator vanished and its name was later
// referenced as an undeclared signal.
// -------------------------------------------------------------------
class DeclaratorTailTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    bool reportsError(const char *src) {
        FILE *file = tmpfile();
        EXPECT_NE(file, nullptr);
        if (!file) return false;
        fwrite(src, 1, strlen(src), file);
        rewind(file);
        ASTNode *program = parse_program(file);
        fclose(file);
        const bool reported = (get_error_count() > 0);
        if (program) free_node(program);
        return reported;
    }
};

TEST_F(DeclaratorTailTest, SecondDeclaratorIsRejected) {
    EXPECT_TRUE(reportsError(
        "int f(int a) { int x = 1, y = 2; int r; r = x + y; return r; }"));
}

TEST_F(DeclaratorTailTest, GarbageAfterInitializerIsRejected) {
    EXPECT_TRUE(reportsError(
        "int f(int a) { int x = 1 this is garbage 42 ; return x; }"));
}

TEST_F(DeclaratorTailTest, SeparateDeclarationsStillParse) {
    EXPECT_FALSE(reportsError(
        "int f(int a) { int x = 1; int y = 2; return x + y; }"));
}

// -------------------------------------------------------------------
// Expression identifiers were silently truncated at 127 characters
// while parameter lists kept the full name, so two identifiers
// differing only past that point collapsed into one undeclared signal.
// -------------------------------------------------------------------
class LongIdentifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    bool reportsError(const std::string &src) {
        FILE *file = tmpfile();
        EXPECT_NE(file, nullptr);
        if (!file) return false;
        fwrite(src.c_str(), 1, src.size(), file);
        rewind(file);
        ASTNode *program = parse_program(file);
        fclose(file);
        const bool reported = (get_error_count() > 0);
        if (program) free_node(program);
        return reported;
    }
};

TEST_F(LongIdentifierTest, IdentifiersPastTheLimitAreRejected) {
    const std::string base(133, 'v');
    const std::string src =
        "int f(int " + base + "AAA, int " + base + "BBB) { int r; r = " +
        base + "AAA + " + base + "BBB; return r; }";
    EXPECT_TRUE(reportsError(src))
        << "Two identifiers differing only past the truncation point must not "
           "collapse silently";
}

TEST_F(LongIdentifierTest, IdentifiersWithinTheLimitStillParse) {
    const std::string name(100, 'v');
    const std::string src =
        "int f(int " + name + ") { int r; r = " + name + "; return r; }";
    EXPECT_FALSE(reportsError(src));
}

// -------------------------------------------------------------------
// Parenthesized expressions must accept every precedence level.
// Parentheses previously restarted parsing at PREC_PARENTHESIZED_MIN (1),
// so operators binding more loosely than ^ terminated the subexpression
// early and the closing paren was then rejected.
// -------------------------------------------------------------------
class ParenthesizedPrecedenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_array_table();
        reset_struct_table();
        reset_error_counters();
    }

    // Parse a function whose body assigns `expr`, returning true when the
    // source parsed without any diagnostic.
    bool parses(const std::string &expr) {
        const std::string src =
            "int f(int a, int b) { int r; r = " + expr + "; return r; }";
        FILE *file = tmpfile();
        EXPECT_NE(file, nullptr);
        if (!file) return false;
        fwrite(src.c_str(), 1, src.size(), file);
        rewind(file);

        reset_error_counters();
        ASTNode *program = parse_program(file);
        fclose(file);
        const bool ok = (program != NULL) && (get_error_count() == 0);
        if (program) free_node(program);
        return ok;
    }
};

TEST_F(ParenthesizedPrecedenceTest, AcceptsTightlyBindingOperators) {
    EXPECT_TRUE(parses("(a * b) + a"));
    EXPECT_TRUE(parses("(a + b) + a"));
    EXPECT_TRUE(parses("(a << b) + a"));
    EXPECT_TRUE(parses("(a < b) + a"));
    EXPECT_TRUE(parses("(a == b) + a"));
    EXPECT_TRUE(parses("(a & b) + a"));
    EXPECT_TRUE(parses("(a ^ b) + a"));
}

TEST_F(ParenthesizedPrecedenceTest, AcceptsLooselyBindingOperators) {
    EXPECT_TRUE(parses("(a | b) + a"));
    EXPECT_TRUE(parses("(a && b) + a"));
    EXPECT_TRUE(parses("(a || b) + a"));
}

TEST_F(ParenthesizedPrecedenceTest, AcceptsMixedLogicalGrouping) {
    EXPECT_TRUE(parses("(a || b) && a"));
    EXPECT_TRUE(parses("(a && b) || (a | b)"));
    EXPECT_TRUE(parses("((a || b))"));
}

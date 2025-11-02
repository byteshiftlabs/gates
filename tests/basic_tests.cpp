#include <gtest/gtest.h>
extern "C" {
#include "astnode.h"
#include "parse.h"
#include "token.h"
#include "utils.h"
#include "symbol_arrays.h"
}
#include <cstdio>
#include <cstring>
#include <memory>

// Provide externs for internal globals needed by tests
extern "C" {
    extern int g_array_count; // declared in utils.c
    extern Token current_token; // defined in token.c
}

// Existing basic AST creation test
TEST(ASTNodeTests, CreateAndLink) {
    ASTNode* program = create_node(NODE_PROGRAM);
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->num_children, 0);

    ASTNode* child = create_node(NODE_STATEMENT);
    add_child(program, child);
    EXPECT_EQ(program->num_children, 1);
    EXPECT_EQ(program->children[0], child);

    free_node(program); // should free recursively without crash
}

// Test dynamic expansion of children array (capacity growth)
TEST(ASTNodeTests, DynamicChildGrowth) {
    ASTNode* parent = create_node(NODE_STATEMENT);
    // Add more than initial capacity (4) to force realloc path
    const int kAdd = 10;
    for (int i = 0; i < kAdd; ++i) {
        ASTNode* c = create_node(NODE_EXPRESSION);
        add_child(parent, c);
        EXPECT_EQ(parent->children[i], c);
        EXPECT_EQ(c->parent, parent);
    }
    EXPECT_EQ(parent->num_children, kAdd);
    free_node(parent);
}

// Test get_precedence ordering relationships
TEST(UtilsTests, OperatorPrecedenceOrdering) {
    EXPECT_GT(get_precedence("*"), get_precedence("+"));
    EXPECT_GT(get_precedence("+"), get_precedence("<<"));
    EXPECT_GT(get_precedence("<<"), get_precedence("<"));
    EXPECT_GT(get_precedence("<"), get_precedence("=="));
    EXPECT_GT(get_precedence("=="), get_precedence("&"));
    EXPECT_GT(get_precedence("&"), get_precedence("^"));
    EXPECT_GT(get_precedence("^"), get_precedence("|"));
    EXPECT_GT(get_precedence("|"), get_precedence("&&"));
    EXPECT_GT(get_precedence("&&"), get_precedence("||"));
}

// Test is_number_str utility
TEST(UtilsTests, IsNumberStr) {
    EXPECT_TRUE(is_number_str("0"));
    EXPECT_TRUE(is_number_str("12345"));
    EXPECT_TRUE(is_number_str("-42"));
    EXPECT_TRUE(is_number_str("+7"));
    EXPECT_FALSE(is_number_str(""));
    EXPECT_FALSE(is_number_str("-"));
    EXPECT_FALSE(is_number_str("12a"));
    EXPECT_FALSE(is_number_str("3.14")); // dot not accepted by is_number_str
}

// Test registering and querying array sizes (no duplicates)
TEST(UtilsTests, RegisterArrayAndLookup) {
    g_array_count = 0; // reset global state for test isolation
    register_array("arr", 5);
    EXPECT_EQ(find_array_size("arr"), 5);
    // Re-register with different size should update
    register_array("arr", 8);
    EXPECT_EQ(find_array_size("arr"), 8);
    // Unknown array
    EXPECT_EQ(find_array_size("none"), -1);
}

// Test tokenization of identifiers, numbers, and multi-char operators
TEST(TokenTests, BasicLexing) {
    const char* src = "int x = a + 42; // comment\nif (x==43) x = x-1;";
    FILE* f = tmpfile();
    ASSERT_NE(f, nullptr);
    fwrite(src, 1, strlen(src), f);
    rewind(f);
    current_line = 1; // reset line tracking
    advance(f); // prime first token
    // int
    EXPECT_EQ(current_token.type, TOKEN_KEYWORD);
    EXPECT_STREQ(current_token.value, "int");
    advance(f); // x
    EXPECT_EQ(current_token.type, TOKEN_IDENTIFIER);
    advance(f); // =
    EXPECT_EQ(current_token.type, TOKEN_OPERATOR);
    advance(f); // a
    EXPECT_EQ(current_token.type, TOKEN_IDENTIFIER);
    advance(f); // +
    EXPECT_EQ(current_token.type, TOKEN_OPERATOR);
    advance(f); // 42
    EXPECT_EQ(current_token.type, TOKEN_NUMBER);
    // Skip to end of first statement ;
    while (current_token.type != TOKEN_SEMICOLON && current_token.type != TOKEN_EOF) advance(f);
    if (current_token.type == TOKEN_SEMICOLON) advance(f);
    // if
    while (current_token.type != TOKEN_KEYWORD && current_token.type != TOKEN_EOF) advance(f);
    EXPECT_EQ(current_token.type, TOKEN_KEYWORD);
    EXPECT_STREQ(current_token.value, "if");
    // scan until ==
    bool saw_eqeq = false;
    while (current_token.type != TOKEN_EOF) {
        if (current_token.type == TOKEN_OPERATOR && strcmp(current_token.value, "==") == 0) { saw_eqeq = true; break; }
        advance(f);
    }
    EXPECT_TRUE(saw_eqeq);
    fclose(f);
}

// Test negative literal detection utility
TEST(UtilsTests, NegativeLiteralDetection) {
    EXPECT_TRUE(is_negative_literal("-123"));
    EXPECT_TRUE(is_negative_literal("-x"));
    EXPECT_TRUE(is_negative_literal("-x1"));
    EXPECT_FALSE(is_negative_literal("123"));
    EXPECT_FALSE(is_negative_literal("--1"));
    EXPECT_FALSE(is_negative_literal("-"));
    EXPECT_FALSE(is_negative_literal(NULL));
}

// Placeholder test for future parser work (needs a file-based parser currently)
TEST(ParserTests, Placeholder) {
    EXPECT_GE(get_precedence("+"), -2);
    EXPECT_LT(get_precedence("+"), get_precedence("*"));
}

// Test function call AST node creation
TEST(FunctionCallTests, CreateFunctionCallNode) {
    ASTNode* call_node = create_node(NODE_FUNC_CALL);
    ASSERT_NE(call_node, nullptr);
    EXPECT_EQ(call_node->type, NODE_FUNC_CALL);
    EXPECT_EQ(call_node->num_children, 0);
    
    call_node->value = strdup("add");
    EXPECT_STREQ(call_node->value, "add");
    
    free_node(call_node);
}

// Test function call with multiple arguments
TEST(FunctionCallTests, FunctionCallWithArguments) {
    ASTNode* call_node = create_node(NODE_FUNC_CALL);
    call_node->value = strdup("multiply");
    
    // Add first argument
    ASTNode* arg1 = create_node(NODE_EXPRESSION);
    arg1->value = strdup("x");
    add_child(call_node, arg1);
    
    // Add second argument
    ASTNode* arg2 = create_node(NODE_EXPRESSION);
    arg2->value = strdup("y");
    add_child(call_node, arg2);
    
    EXPECT_EQ(call_node->num_children, 2);
    EXPECT_EQ(call_node->children[0], arg1);
    EXPECT_EQ(call_node->children[1], arg2);
    EXPECT_STREQ(call_node->children[0]->value, "x");
    EXPECT_STREQ(call_node->children[1]->value, "y");
    
    free_node(call_node);
}

// Test nested function calls
TEST(FunctionCallTests, NestedFunctionCalls) {
    // Outer call: add(...)
    ASTNode* outer_call = create_node(NODE_FUNC_CALL);
    outer_call->value = strdup("add");
    
    // First argument: multiply(a, 2)
    ASTNode* inner_call1 = create_node(NODE_FUNC_CALL);
    inner_call1->value = strdup("multiply");
    ASTNode* a = create_node(NODE_EXPRESSION);
    a->value = strdup("a");
    ASTNode* two = create_node(NODE_EXPRESSION);
    two->value = strdup("2");
    add_child(inner_call1, a);
    add_child(inner_call1, two);
    
    // Second argument: multiply(b, 3)
    ASTNode* inner_call2 = create_node(NODE_FUNC_CALL);
    inner_call2->value = strdup("multiply");
    ASTNode* b = create_node(NODE_EXPRESSION);
    b->value = strdup("b");
    ASTNode* three = create_node(NODE_EXPRESSION);
    three->value = strdup("3");
    add_child(inner_call2, b);
    add_child(inner_call2, three);
    
    add_child(outer_call, inner_call1);
    add_child(outer_call, inner_call2);
    
    EXPECT_EQ(outer_call->num_children, 2);
    EXPECT_EQ(outer_call->children[0]->type, NODE_FUNC_CALL);
    EXPECT_EQ(outer_call->children[1]->type, NODE_FUNC_CALL);
    EXPECT_STREQ(outer_call->children[0]->value, "multiply");
    EXPECT_STREQ(outer_call->children[1]->value, "multiply");
    
    free_node(outer_call);
}

// Test function call with no arguments
TEST(FunctionCallTests, FunctionCallNoArguments) {
    ASTNode* call_node = create_node(NODE_FUNC_CALL);
    call_node->value = strdup("get_value");
    
    EXPECT_EQ(call_node->num_children, 0);
    EXPECT_STREQ(call_node->value, "get_value");
    
    free_node(call_node);
}

// Test function call in assignment
TEST(FunctionCallTests, FunctionCallInAssignment) {
    ASTNode* assign_node = create_node(NODE_ASSIGNMENT);
    
    // LHS: z
    ASTNode* lhs = create_node(NODE_EXPRESSION);
    lhs->value = strdup("z");
    add_child(assign_node, lhs);
    
    // RHS: add(x, y)
    ASTNode* rhs = create_node(NODE_FUNC_CALL);
    rhs->value = strdup("add");
    ASTNode* arg_x = create_node(NODE_EXPRESSION);
    arg_x->value = strdup("x");
    ASTNode* arg_y = create_node(NODE_EXPRESSION);
    arg_y->value = strdup("y");
    add_child(rhs, arg_x);
    add_child(rhs, arg_y);
    add_child(assign_node, rhs);
    
    EXPECT_EQ(assign_node->num_children, 2);
    EXPECT_EQ(assign_node->children[1]->type, NODE_FUNC_CALL);
    EXPECT_STREQ(assign_node->children[1]->value, "add");
    
    free_node(assign_node);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include <gtest/gtest.h>
extern "C" {
    #include "error_handler.h"
}
#include <sstream>
#include <string>

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>

// Helper class to capture stderr output from C fprintf
class StderrCapture {
public:
    StderrCapture() {
        fflush(stderr);
        old_stderr = dup(STDERR_FILENO);
        
        if (pipe(pipe_fds) != 0)
        {
            throw std::runtime_error("Failed to create pipe");
        }
        
        dup2(pipe_fds[1], STDERR_FILENO);
        close(pipe_fds[1]);
    }
    
    ~StderrCapture() {
        dup2(old_stderr, STDERR_FILENO);
        close(old_stderr);
        close(pipe_fds[0]);
    }
    
    std::string get_output() {
        fflush(stderr);
        
        char buffer[4096];
        std::string output;
        
        int flags = fcntl(pipe_fds[0], F_GETFL, 0);
        fcntl(pipe_fds[0], F_SETFL, flags | O_NONBLOCK);
        
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_fds[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytes_read] = '\0';
            output += buffer;
        }
        
        return output;
    }
    
    void clear() {
        get_output(); // Drain the pipe
    }
    
private:
    int old_stderr;
    int pipe_fds[2];
};

class ErrorHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_error_counters();
        set_colored_output(0); // Disable colors for testing
    }
    
    void TearDown() override {
        reset_error_counters();
        set_colored_output(1); // Re-enable colors
    }
};

TEST_F(ErrorHandlerTest, InfoMessageDoesNotIncrementCounters) {
    StderrCapture capture;
    
    log_info(ERROR_CATEGORY_PARSER, 10, "This is an informational message");
    
    EXPECT_EQ(get_error_count(), 0);
    EXPECT_EQ(get_warning_count(), 0);
    EXPECT_FALSE(has_errors());
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("info"), std::string::npos);
    EXPECT_NE(output.find("Parser"), std::string::npos);
    EXPECT_NE(output.find("line 10"), std::string::npos);
    EXPECT_NE(output.find("informational message"), std::string::npos);
}

TEST_F(ErrorHandlerTest, WarningMessageIncrementsWarningCounter) {
    StderrCapture capture;
    
    log_warning(ERROR_CATEGORY_SEMANTIC, 25, "Implicit type conversion");
    
    EXPECT_EQ(get_error_count(), 0);
    EXPECT_EQ(get_warning_count(), 1);
    EXPECT_FALSE(has_errors());
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("warning"), std::string::npos);
    EXPECT_NE(output.find("Semantic"), std::string::npos);
    EXPECT_NE(output.find("line 25"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ErrorMessageIncrementsErrorCounter) {
    StderrCapture capture;
    
    log_error(ERROR_CATEGORY_PARSER, 42, "Unexpected token");
    
    EXPECT_EQ(get_error_count(), 1);
    EXPECT_EQ(get_warning_count(), 0);
    EXPECT_TRUE(has_errors());
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("error"), std::string::npos);
    EXPECT_NE(output.find("Parser"), std::string::npos);
    EXPECT_NE(output.find("line 42"), std::string::npos);
}

TEST_F(ErrorHandlerTest, MultipleErrorsAccumulate) {
    log_error(ERROR_CATEGORY_LEXER, 1, "First error");
    log_error(ERROR_CATEGORY_PARSER, 2, "Second error");
    log_error(ERROR_CATEGORY_CODEGEN, 3, "Third error");
    
    EXPECT_EQ(get_error_count(), 3);
    EXPECT_TRUE(has_errors());
}

TEST_F(ErrorHandlerTest, MultipleWarningsAccumulate) {
    log_warning(ERROR_CATEGORY_PARSER, 10, "First warning");
    log_warning(ERROR_CATEGORY_SEMANTIC, 20, "Second warning");
    
    EXPECT_EQ(get_warning_count(), 2);
    EXPECT_EQ(get_error_count(), 0);
    EXPECT_FALSE(has_errors());
}

TEST_F(ErrorHandlerTest, MixedMessagesTrackSeparately) {
    log_info(ERROR_CATEGORY_GENERAL, 5, "Info");
    log_warning(ERROR_CATEGORY_PARSER, 10, "Warning 1");
    log_error(ERROR_CATEGORY_LEXER, 15, "Error 1");
    log_warning(ERROR_CATEGORY_SEMANTIC, 20, "Warning 2");
    log_error(ERROR_CATEGORY_CODEGEN, 25, "Error 2");
    
    EXPECT_EQ(get_error_count(), 2);
    EXPECT_EQ(get_warning_count(), 2);
    EXPECT_TRUE(has_errors());
}

TEST_F(ErrorHandlerTest, ResetCountersClearsAll) {
    log_error(ERROR_CATEGORY_PARSER, 10, "Error");
    log_warning(ERROR_CATEGORY_PARSER, 15, "Warning");
    
    EXPECT_EQ(get_error_count(), 1);
    EXPECT_EQ(get_warning_count(), 1);
    
    reset_error_counters();
    
    EXPECT_EQ(get_error_count(), 0);
    EXPECT_EQ(get_warning_count(), 0);
    EXPECT_FALSE(has_errors());
}

TEST_F(ErrorHandlerTest, MessageWithoutLineNumber) {
    StderrCapture capture;
    
    log_error(ERROR_CATEGORY_GENERAL, 0, "Generic error message");
    
    std::string output = capture.get_output();
    EXPECT_EQ(output.find("line"), std::string::npos);
    EXPECT_NE(output.find("Generic error message"), std::string::npos);
}

TEST_F(ErrorHandlerTest, AllErrorCategories) {
    StderrCapture capture;
    
    log_error(ERROR_CATEGORY_LEXER, 1, "Lexer error");
    capture.clear();
    
    log_error(ERROR_CATEGORY_PARSER, 1, "Parser error");
    EXPECT_NE(capture.get_output().find("Parser"), std::string::npos);
    capture.clear();
    
    log_error(ERROR_CATEGORY_SEMANTIC, 1, "Semantic error");
    EXPECT_NE(capture.get_output().find("Semantic"), std::string::npos);
    capture.clear();
    
    log_error(ERROR_CATEGORY_CODEGEN, 1, "Codegen error");
    EXPECT_NE(capture.get_output().find("Codegen"), std::string::npos);
    capture.clear();
    
    log_error(ERROR_CATEGORY_GENERAL, 1, "General error");
    EXPECT_NE(capture.get_output().find("General"), std::string::npos);
    
    EXPECT_EQ(get_error_count(), 5);
}

TEST_F(ErrorHandlerTest, ReportMessageFunction) {
    StderrCapture capture;
    
    report_message(SEVERITY_WARNING, ERROR_CATEGORY_PARSER, 33, 
                   "Function '%s' is deprecated", "old_func");
    
    EXPECT_EQ(get_warning_count(), 1);
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("warning"), std::string::npos);
    EXPECT_NE(output.find("old_func"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ColoredOutputCanBeDisabled) {
    // Colors already disabled in SetUp
    StderrCapture capture;
    
    log_error(ERROR_CATEGORY_PARSER, 10, "Test error");
    
    std::string output = capture.get_output();
    // Should not contain ANSI escape codes
    EXPECT_EQ(output.find("\033["), std::string::npos);
}

// ===== New Extended Features Tests =====

TEST_F(ErrorHandlerTest, ExtendedLocationWithFilename) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = "test.c",
        .line = 42,
        .column = 15,
        .source_line = NULL
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc, NULL,
                     "Unexpected token");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("test.c:42:15:"), std::string::npos);
    EXPECT_NE(output.find("Unexpected token"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ExtendedLocationWithSourceContext) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = "main.c",
        .line = 10,
        .column = 9,
        .source_line = "int x = 5"
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc, NULL,
                     "Expected ';' after expression");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("main.c:10:9:"), std::string::npos);
    EXPECT_NE(output.find("int x = 5"), std::string::npos);
    EXPECT_NE(output.find("^"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ErrorCodeDisplay) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = "test.c",
        .line = 25,
        .column = 3,
        .source_line = NULL
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_SEMANTIC, &loc, "E0042",
                     "Type mismatch");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("[E0042]"), std::string::npos);
    EXPECT_NE(output.find("Type mismatch"), std::string::npos);
}

TEST_F(ErrorHandlerTest, MultilineErrorWithHint) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = "example.c",
        .line = 15,
        .column = 5,
        .source_line = "foo(x, y"
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc, "E0001",
                     "Missing closing parenthesis");
    add_error_hint("Add ')' at the end of the function call");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("Missing closing parenthesis"), std::string::npos);
    EXPECT_NE(output.find("hint:"), std::string::npos);
    EXPECT_NE(output.find("Add ')' at the end"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ErrorWithSuggestion) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = "vars.c",
        .line = 20,
        .column = 10,
        .source_line = "print(\"hello\");"
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_SEMANTIC, &loc, "E0100",
                     "Unknown function 'print'");
    add_suggestion("printf");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("Unknown function"), std::string::npos);
    EXPECT_NE(output.find("help:"), std::string::npos);
    EXPECT_NE(output.find("did you mean 'printf'?"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ErrorWithHintAndSuggestion) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = "typo.c",
        .line = 8,
        .column = 5,
        .source_line = "retrun 0;"
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc, "E0050",
                     "Unknown keyword 'retrun'");
    add_error_hint("Check the spelling of keywords");
    add_suggestion("return");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("Unknown keyword"), std::string::npos);
    EXPECT_NE(output.find("hint:"), std::string::npos);
    EXPECT_NE(output.find("help:"), std::string::npos);
    EXPECT_NE(output.find("return"), std::string::npos);
}

TEST_F(ErrorHandlerTest, SourceContextCaretAlignment) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = NULL,
        .line = 42,
        .column = 15,
        .source_line = "int result = x + y * z;"
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_SEMANTIC, &loc, NULL,
                     "Variable 'y' not declared");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("int result = x + y * z;"), std::string::npos);
    // The caret should be approximately at column 15
    EXPECT_NE(output.find("^"), std::string::npos);
}

TEST_F(ErrorHandlerTest, ExtendedErrorIncrementsCounter) {
    ErrorLocation loc = {
        .filename = "test.c",
        .line = 10,
        .column = 5,
        .source_line = NULL
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc, "E0001",
                     "Syntax error");
    
    EXPECT_EQ(get_error_count(), 1);
    EXPECT_TRUE(has_errors());
}

TEST_F(ErrorHandlerTest, ExtendedWarningIncrementsCounter) {
    ErrorLocation loc = {
        .filename = "warn.c",
        .line = 5,
        .column = 3,
        .source_line = NULL
    };
    
    report_message_ex(SEVERITY_WARNING, ERROR_CATEGORY_SEMANTIC, &loc, "W0010",
                     "Unused variable");
    
    EXPECT_EQ(get_warning_count(), 1);
    EXPECT_EQ(get_error_count(), 0);
}

TEST_F(ErrorHandlerTest, LocationWithoutFilename) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = NULL,
        .line = 33,
        .column = 0,
        .source_line = NULL
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc, NULL,
                     "Parse error");
    
    std::string output = capture.get_output();
    EXPECT_NE(output.find("line 33:"), std::string::npos);
    EXPECT_NE(output.find("Parse error"), std::string::npos);
}

TEST_F(ErrorHandlerTest, CompleteErrorReport) {
    StderrCapture capture;
    
    ErrorLocation loc = {
        .filename = "complete.c",
        .line = 100,
        .column = 20,
        .source_line = "if (condition)  else { }"
    };
    
    report_message_ex(SEVERITY_ERROR, ERROR_CATEGORY_PARSER, &loc, "E0075",
                     "Expected statement before 'else'");
    add_error_hint("An 'if' statement requires a body before the 'else' clause");
    add_suggestion("Add braces: if (condition) { } else { }");
    
    std::string output = capture.get_output();
    
    // Check all components are present
    EXPECT_NE(output.find("complete.c:100:20:"), std::string::npos);
    EXPECT_NE(output.find("[E0075]"), std::string::npos);
    EXPECT_NE(output.find("Expected statement before 'else'"), std::string::npos);
    EXPECT_NE(output.find("if (condition)  else { }"), std::string::npos);
    EXPECT_NE(output.find("^"), std::string::npos);
    EXPECT_NE(output.find("hint:"), std::string::npos);
    EXPECT_NE(output.find("help:"), std::string::npos);
    
    EXPECT_EQ(get_error_count(), 1);
}


#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <c_definition.json.h>


TEST(parse_C, basic_program) {
    auto tokens = TextParser(R"(
#include <stdio.h>

/* A standard block comment */
int main(void) {
    // Print message
    printf("Hello \n \"World\"!");
    return 0;
}
)", &c_definition);

    // Root should find: Preprocessor (#include), BlockComment, Keyword (int, void, return), Variable (main, printf), CodeBlock, etc.
    bool found_preprocessor = false;
    bool found_block_comment = false;
    bool found_keyword = false;
    bool found_code_block = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        if (type == "Preprocessor") {
            found_preprocessor = true;
        }
        if (type == "BlockComment") {
            found_block_comment = true;
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    EXPECT_TRUE(found_preprocessor);
    EXPECT_TRUE(found_block_comment);
    EXPECT_TRUE(found_code_block);

    // Get CodeBlock children
    int class_cb_idx = -1;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (std::string(tokens[i].type) == "CodeBlock") {
            class_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(class_cb_idx, -1);

    auto class_cb = tokens[class_cb_idx];
    bool found_line_comment = false;
    bool found_printf_var = false;
    bool found_double_str = false;
    bool found_return_keyword = false;
    bool found_number = false;

    for (size_t i = 0; i < class_cb.children; ++i) {
        std::string type = class_cb[i].type;
        if (type == "LineComment") found_line_comment = true;
        if (type == "Variable") found_printf_var = true;
        if (type == "DoubleString") found_double_str = true;
        if (type == "Keyword") found_return_keyword = true;
        if (type == "Number") found_number = true;
    }

    EXPECT_TRUE(found_line_comment);
    EXPECT_TRUE(found_printf_var);
    EXPECT_TRUE(found_double_str);
    EXPECT_TRUE(found_return_keyword);
    EXPECT_TRUE(found_number);

    // DoubleString should have nested StringEscape
    int double_str_idx = -1;
    for (size_t i = 0; i < class_cb.children; ++i) {
        if (std::string(class_cb[i].type) == "DoubleString") {
            double_str_idx = i;
            break;
        }
    }
    ASSERT_NE(double_str_idx, -1);

    auto double_str = class_cb[double_str_idx];
    ASSERT_GE(double_str.children, 1);
    EXPECT_STREQ(double_str[0].type, "StringEscape");
}

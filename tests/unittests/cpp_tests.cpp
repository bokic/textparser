#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <cpp_definition.json.h>


TEST(parse_CPP, basic_cpp_program) {
    auto tokens = TextParser(R"(
#include <iostream>
using namespace std;

template<typename T>
class Hello {
public:
    void print() {
        std::cout << "Hello \n \"World\"!" << std::endl;
    }
};
)", &cpp_definition);

    // Root should find: Preprocessor (#include), Keyword (using, namespace, template, typename, class), Variable (std, T, Hello), CodeBlock, etc.
    bool found_preprocessor = false;
    bool found_template_keyword = false;
    bool found_class_keyword = false;
    bool found_code_block = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        if (type == "Preprocessor") {
            found_preprocessor = true;
        }
        if (type == "Keyword") {
            // Check template keyword
            found_template_keyword = true;
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    EXPECT_TRUE(found_preprocessor);
    EXPECT_TRUE(found_template_keyword);
    EXPECT_TRUE(found_code_block);

    // Get CodeBlock index
    int class_cb_idx = -1;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (std::string(tokens[i].type) == "CodeBlock") {
            class_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(class_cb_idx, -1);

    auto class_cb = tokens[class_cb_idx];
    bool found_public_keyword = false;
    bool found_print_method_cb = false;

    for (size_t i = 0; i < class_cb.children; ++i) {
        std::string type = class_cb[i].type;
        if (type == "Keyword") found_public_keyword = true;
        if (type == "CodeBlock") found_print_method_cb = true;
    }

    EXPECT_TRUE(found_public_keyword);
    EXPECT_TRUE(found_print_method_cb);

    // Inside print method CodeBlock
    int method_cb_idx = -1;
    for (size_t i = 0; i < class_cb.children; ++i) {
        if (std::string(class_cb[i].type) == "CodeBlock") {
            method_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(method_cb_idx, -1);

    auto method_cb = class_cb[method_cb_idx];
    bool found_double_str = false;
    bool found_scope_resolution = false;

    for (size_t i = 0; i < method_cb.children; ++i) {
        std::string type = method_cb[i].type;
        if (type == "DoubleString") found_double_str = true;
        if (type == "Operator") {
            // Semicolon, ::, << are all operators
            found_scope_resolution = true;
        }
    }

    EXPECT_TRUE(found_double_str);
    EXPECT_TRUE(found_scope_resolution);

    // DoubleString should have nested StringEscape
    int double_str_idx = -1;
    for (size_t i = 0; i < method_cb.children; ++i) {
        if (std::string(method_cb[i].type) == "DoubleString") {
            double_str_idx = i;
            break;
        }
    }
    ASSERT_NE(double_str_idx, -1);

    auto double_str = method_cb[double_str_idx];
    ASSERT_GE(double_str.children, 1);
    EXPECT_STREQ(double_str[0].type, "StringEscape");
}

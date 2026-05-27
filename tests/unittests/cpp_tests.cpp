#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <cpp_definition.json.h>


TEST(parse_CPP, basic_cpp_program) {
    auto tokens = TextParser(R"(
// A LineComment to verify parsing
/* A BlockComment to
   verify parsing multi-line block comments */
#include <iostream>
using namespace std;

template<typename T>
class Hello {
public:
    void print() {
        std::cout << "Hello \n \"World\"!" << std::endl;
        char single = '\n'; // SingleString with nested StringEscape
        bool active = true; // Boolean
        bool inactive = false; // Boolean
        double pi = 3.14159; // Number (decimal float)
        int hexVal = 0x2A; // Number (hexadecimal integer)
        int binVal = 0b1010; // Number (binary integer)
        double sciVal = 1e-9; // Number (scientific notation)
    }
};
)", &cpp_definition);

    // Root should find: LineComment, BlockComment, Preprocessor (#include), Keyword (using, namespace, template, typename, class), Variable (std, T, Hello), CodeBlock, etc.
    bool found_line_comment = false;
    bool found_block_comment = false;
    bool found_preprocessor = false;
    bool found_template_keyword = false;
    bool found_class_keyword = false;
    bool found_code_block = false;
    bool found_variable_root = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        if (type == "LineComment") {
            found_line_comment = true;
        }
        if (type == "BlockComment") {
            found_block_comment = true;
        }
        if (type == "Preprocessor") {
            found_preprocessor = true;
        }
        if (type == "Keyword") {
            if (tokens[i].value == "template") {
                found_template_keyword = true;
            }
            if (tokens[i].value == "class") {
                found_class_keyword = true;
            }
        }
        if (type == "Variable") {
            if (tokens[i].value == "Hello") {
                found_variable_root = true;
            }
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    EXPECT_TRUE(found_line_comment);
    EXPECT_TRUE(found_block_comment);
    EXPECT_TRUE(found_preprocessor);
    EXPECT_TRUE(found_template_keyword);
    EXPECT_TRUE(found_class_keyword);
    EXPECT_TRUE(found_variable_root);
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
        if (type == "Keyword" && class_cb[i].value == "public") found_public_keyword = true;
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
    bool found_single_str = false;
    bool found_boolean_true = false;
    bool found_boolean_false = false;
    bool found_decimal_num = false;
    bool found_hex_num = false;
    bool found_bin_num = false;
    bool found_sci_num = false;
    bool found_scope_resolution = false;

    for (size_t i = 0; i < method_cb.children; ++i) {
        std::string type = method_cb[i].type;
        std::string val = method_cb[i].value;
        if (type == "DoubleString") found_double_str = true;
        if (type == "SingleString") found_single_str = true;
        if (type == "Boolean") {
            if (val == "true") found_boolean_true = true;
            if (val == "false") found_boolean_false = true;
        }
        if (type == "Number") {
            if (val == "3.14159") found_decimal_num = true;
            if (val == "0x2A") found_hex_num = true;
            if (val == "0b1010") found_bin_num = true;
            if (val == "1e-9") found_sci_num = true;
        }
        if (type == "Operator") {
            if (val == "::") found_scope_resolution = true;
        }
    }

    EXPECT_TRUE(found_double_str);
    EXPECT_TRUE(found_single_str);
    EXPECT_TRUE(found_boolean_true);
    EXPECT_TRUE(found_boolean_false);
    EXPECT_TRUE(found_decimal_num);
    EXPECT_TRUE(found_hex_num);
    EXPECT_TRUE(found_bin_num);
    EXPECT_TRUE(found_sci_num);
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

    // SingleString should have nested StringEscape
    int single_str_idx = -1;
    for (size_t i = 0; i < method_cb.children; ++i) {
        if (std::string(method_cb[i].type) == "SingleString") {
            single_str_idx = i;
            break;
        }
    }
    ASSERT_NE(single_str_idx, -1);

    auto single_str = method_cb[single_str_idx];
    ASSERT_GE(single_str.children, 1);
    EXPECT_STREQ(single_str[0].type, "StringEscape");
}

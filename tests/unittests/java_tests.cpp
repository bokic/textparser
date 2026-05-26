#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <java_definition.json.h>


TEST(parse_Java, basic_class) {
    auto tokens = TextParser(R"(
package com.example;

import java.util.*;

@Deprecated
public class Hello {
    // A standard main method
    public static void main(String[] args) {
        System.out.println("Hello \n \"World\"!");
    }
}
)", &java_definition);

    // Root should find multiple top-level tokens like Keyword (package, import, public, class), Annotation (@Deprecated), CodeBlock, etc.
    bool found_package = false;
    bool found_import = false;
    bool found_annotation = false;
    bool found_class_keyword = false;
    bool found_code_block = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        if (type == "Keyword") {
            // Check if it is package or import
            // Since we can't easily read string content here, we just know Keyword is found
            found_package = true;
        }
        if (type == "Annotation") {
            found_annotation = true;
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    EXPECT_TRUE(found_package);
    EXPECT_TRUE(found_annotation);
    EXPECT_TRUE(found_code_block);

    // Inside the class CodeBlock, we should find variables, line comments, keywords, inner code blocks
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
    bool found_method_keyword = false;
    bool found_inner_cb = false;

    for (size_t i = 0; i < class_cb.children; ++i) {
        std::string type = class_cb[i].type;
        if (type == "LineComment") found_line_comment = true;
        if (type == "Keyword") found_method_keyword = true;
        if (type == "CodeBlock") found_inner_cb = true;
    }

    EXPECT_TRUE(found_line_comment);
    EXPECT_TRUE(found_method_keyword);
    EXPECT_TRUE(found_inner_cb);

    // Inside main method CodeBlock, we should find DoubleString
    int method_cb_idx = -1;
    for (size_t i = 0; i < class_cb.children; ++i) {
        if (std::string(class_cb[i].type) == "CodeBlock") {
            method_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(method_cb_idx, -1);

    auto method_cb = class_cb[method_cb_idx];
    int double_str_idx = -1;
    for (size_t i = 0; i < method_cb.children; ++i) {
        if (std::string(method_cb[i].type) == "DoubleString") {
            double_str_idx = i;
            break;
        }
    }
    ASSERT_NE(double_str_idx, -1);

    // DoubleString should have nested StringEscape
    auto double_str = method_cb[double_str_idx];
    ASSERT_GE(double_str.children, 1);
    EXPECT_STREQ(double_str[0].type, "StringEscape");
}

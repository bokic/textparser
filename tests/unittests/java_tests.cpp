#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <java_definition.json.h>

void collect_token_types(const TokenParserItem &item, std::set<std::string> &collected) {
    if (item.type != nullptr) {
        collected.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        collect_token_types(item[i], collected);
    }
}

TEST(parse_Java, basic_class) {
    const char *source = R"(
package com.example;

import java.util.*;

/* This is a multi-line
   BlockComment */
@Deprecated
public class Hello {
    // A standard main method
    public static void main(String[] args) {
        boolean flag = true;
        flag = false;
        char c = '\n';
        int num = 42;
        double val = 3.14f;
        var text = null;
        int hex = 0x2f;
        int bin = 0b1010;
        long large = 1_234_567L;
        Runnable r = () -> {};
        java.util.function.Consumer<String> printer = System.out::println;
        System.out.println("Hello \n \"World\"!");
    }
}
)";

    auto tokens = TextParser(source, &java_definition);
    auto get_value = [&](const TokenParserItem &item) {
        return std::string(source + item.position, item.length);
    };

    bool found_package = false;
    bool found_import = false;
    bool found_annotation = false;
    bool found_block_comment = false;
    bool found_code_block = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        std::string value = get_value(tokens[i]);
        if (type == "Keyword") {
            if (value == "package") found_package = true;
            if (value == "import") found_import = true;
        }
        if (type == "Annotation" && value == "@Deprecated") {
            found_annotation = true;
        }
        if (type == "BlockComment") {
            if (value.find("This is a multi-line") != std::string::npos) {
                found_block_comment = true;
            }
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    EXPECT_TRUE(found_package);
    EXPECT_TRUE(found_import);
    EXPECT_TRUE(found_annotation);
    EXPECT_TRUE(found_block_comment);
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
        std::string value = get_value(class_cb[i]);
        if (type == "LineComment" && value.find("A standard main method") != std::string::npos) {
            found_line_comment = true;
        }
        if (type == "Keyword" && value == "static") {
            found_method_keyword = true;
        }
        if (type == "CodeBlock") {
            found_inner_cb = true;
        }
    }

    EXPECT_TRUE(found_line_comment);
    EXPECT_TRUE(found_method_keyword);
    EXPECT_TRUE(found_inner_cb);

    // Inside main method CodeBlock, we should find SingleString, DoubleString, Boolean, Number, Operator
    int method_cb_idx = -1;
    for (size_t i = 0; i < class_cb.children; ++i) {
        if (std::string(class_cb[i].type) == "CodeBlock") {
            method_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(method_cb_idx, -1);

    auto method_cb = class_cb[method_cb_idx];
    bool found_var_keyword = false;
    bool found_null_keyword = false;
    bool found_true_bool = false;
    bool found_false_bool = false;
    bool found_single_string = false;
    bool found_double_string = false;
    bool found_num_42 = false;
    bool found_num_float = false;
    bool found_num_hex = false;
    bool found_num_bin = false;
    bool found_num_large = false;
    bool found_lambda_op = false;
    bool found_method_ref_op = false;

    for (size_t i = 0; i < method_cb.children; ++i) {
        std::string type = method_cb[i].type;
        std::string value = get_value(method_cb[i]);
        if (type == "Keyword") {
            if (value == "var") found_var_keyword = true;
            if (value == "null") found_null_keyword = true;
        }
        if (type == "Boolean") {
            if (value == "true") found_true_bool = true;
            if (value == "false") found_false_bool = true;
        }
        if (type == "SingleString" && value == "'\\n'") {
            found_single_string = true;
        }
        if (type == "DoubleString" && value.find("Hello") != std::string::npos) {
            found_double_string = true;
        }
        if (type == "Number") {
            if (value == "42") found_num_42 = true;
            if (value == "3.14f") found_num_float = true;
            if (value == "0x2f") found_num_hex = true;
            if (value == "0b1010") found_num_bin = true;
            if (value == "1_234_567L") found_num_large = true;
        }
        if (type == "Operator") {
            if (value == "->") found_lambda_op = true;
            if (value == "::") found_method_ref_op = true;
        }
    }

    EXPECT_TRUE(found_var_keyword);
    EXPECT_TRUE(found_null_keyword);
    EXPECT_TRUE(found_true_bool);
    EXPECT_TRUE(found_false_bool);
    EXPECT_TRUE(found_single_string);
    EXPECT_TRUE(found_double_string);
    EXPECT_TRUE(found_num_42);
    EXPECT_TRUE(found_num_float);
    EXPECT_TRUE(found_num_hex);
    EXPECT_TRUE(found_num_bin);
    EXPECT_TRUE(found_num_large);
    EXPECT_TRUE(found_lambda_op);
    EXPECT_TRUE(found_method_ref_op);

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
    EXPECT_EQ(get_value(double_str[0]), "\\n");

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
    EXPECT_EQ(get_value(single_str[0]), "\\n");

    // Let's verify that EVERY defined token has been parsed and covered!
    std::set<std::string> collected_types;
    for (size_t i = 0; i < tokens.count; ++i) {
        collect_token_types(tokens[i], collected_types);
    }

    std::set<std::string> expected_types = {
        "LineComment",
        "BlockComment",
        "Annotation",
        "Keyword",
        "Variable",
        "CodeBlock",
        "Operator",
        "SingleString",
        "DoubleString",
        "StringEscape",
        "Number",
        "Boolean"
    };

    for (const auto &type : expected_types) {
        EXPECT_TRUE(collected_types.count(type) > 0) << "Token type not covered: " << type;
    }
}

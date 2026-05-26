#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <php_definition.json.h>


TEST(parse_PHP, simple_php_block) {
    auto tokens = TextParser(R"(<?php echo "hello"; ?>)", &php_definition);
    ASSERT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "Tag");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   22);
    
    // Within Tag, we should have Keyword, DoubleString, and Operator (;)
    ASSERT_EQ   (tokens[0].children, 3);
    
    EXPECT_STREQ(tokens[0][0].type, "Keyword");
    EXPECT_EQ   (tokens[0][0].position, 6);
    EXPECT_EQ   (tokens[0][0].length,   4); // "echo"

    EXPECT_STREQ(tokens[0][1].type, "DoubleString");
    EXPECT_EQ   (tokens[0][1].position, 11);
    EXPECT_EQ   (tokens[0][1].length,   7); // "\"hello\""

    EXPECT_STREQ(tokens[0][2].type, "Operator");
    EXPECT_EQ   (tokens[0][2].position, 18);
    EXPECT_EQ   (tokens[0][2].length,   1); // ";"
}

TEST(parse_PHP, php_comments) {
    auto tokens = TextParser(R"(<?php
// line comment
# shell comment
/* block
comment */
?>)", &php_definition);
    ASSERT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "Tag");
    
    // Tag should have 3 comment children
    ASSERT_EQ(tokens[0].children, 3);
    
    EXPECT_STREQ(tokens[0][0].type, "LineComment");
    EXPECT_STREQ(tokens[0][1].type, "LineComment");
    EXPECT_STREQ(tokens[0][2].type, "BlockComment");
}

TEST(parse_PHP, variables_and_blocks) {
    auto tokens = TextParser(R"(<?php
$x = 42;
if (true) {
    $y = false;
}
?>)", &php_definition);
    ASSERT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "Tag");
    
    // Tag children: $x (Variable), = (Operator), 42 (Number), ; (Operator), if (Keyword), true (Boolean), CodeBlock, ; (Operator) or similar.
    // Let's verify variable and code block are present
    bool found_variable = false;
    bool found_code_block = false;
    bool found_number = false;
    bool found_boolean = false;

    for (int i = 0; i < tokens[0].children; ++i) {
        std::string type = tokens[0][i].type;
        if (type == "Variable") found_variable = true;
        if (type == "CodeBlock") found_code_block = true;
        if (type == "Number") found_number = true;
        if (type == "Boolean") found_boolean = true;
    }

    EXPECT_TRUE(found_variable);
    EXPECT_TRUE(found_code_block);
    EXPECT_TRUE(found_number);
    EXPECT_TRUE(found_boolean);
}

TEST(parse_PHP, double_string_interpolation) {
    auto tokens = TextParser(R"(<?php $str = "hello $world"; ?>)", &php_definition);
    ASSERT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "Tag");

    // Let's find DoubleString
    int double_str_idx = -1;
    for (int i = 0; i < tokens[0].children; ++i) {
        if (std::string(tokens[0][i].type) == "DoubleString") {
            double_str_idx = i;
            break;
        }
    }
    ASSERT_NE(double_str_idx, -1);

    // DoubleString should have a child of type Variable ($world)
    auto double_str = tokens[0][double_str_idx];
    ASSERT_EQ(double_str.children, 1);
    EXPECT_STREQ(double_str[0].type, "Variable");
}

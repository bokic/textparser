#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <php_definition.json.h>
#include <php.h>
#include <set>
#include <string>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}


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
    
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Boolean"));
}

TEST(parse_PHP, double_string_interpolation) {
    auto tokens = TextParser(R"(<?php $str = "hello $world"; ?>)", &php_definition);
    ASSERT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "Tag");

    // Let's find DoubleString
    int double_str_idx = -1;
    for (size_t i = 0; i < tokens[0].children; ++i) {
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

TEST(parse_PHP, extra_tokens_coverage) {
    auto tokens = TextParser(R"(<?php
$arr = ['key' => 'value'];
$obj->method();
?>)", &php_definition);
    
    ASSERT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "Tag");

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("ArrayKeyValue"));
    EXPECT_TRUE(found.contains("MemberAccess"));
    EXPECT_TRUE(found.contains("SingleString"));
}

TEST(parse_PHP, string_escapes) {
    auto tokens = TextParser(R"(<?php
$s1 = 'escaped \' \\ string';
$s2 = "escaped \" \$ \\ \n \r \t string";
?>)", &php_definition);
    ASSERT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "Tag");

    // Find s1 and s2 strings
    int single_str_idx = -1;
    int double_str_idx = -1;
    for (size_t i = 0; i < tokens[0].children; ++i) {
        if (std::string(tokens[0][i].type) == "SingleString") {
            single_str_idx = i;
        }
        if (std::string(tokens[0][i].type) == "DoubleString") {
            double_str_idx = i;
        }
    }
    ASSERT_NE(single_str_idx, -1);
    ASSERT_NE(double_str_idx, -1);

    auto single_str = tokens[0][single_str_idx];
    // SingleString should have 2 children of type StringEscape (\' and \\)
    ASSERT_EQ(single_str.children, 2);
    EXPECT_STREQ(single_str[0].type, "StringEscape");
    EXPECT_STREQ(single_str[1].type, "StringEscape");

    auto double_str = tokens[0][double_str_idx];
    // DoubleString should have 6 children of type StringEscape ( \", \$, \\, \n, \r, \t )
    int escape_count = 0;
    for (size_t i = 0; i < double_str.children; ++i) {
        if (std::string(double_str[i].type) == "StringEscape") {
            escape_count++;
        }
    }
    EXPECT_EQ(escape_count, 6);
}

TEST(validate_PHP, built_in_functions) {
    // 1. Valid call to strlen
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<?php strlen('hello'); ?>", 25, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &php_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_php(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }

    // 2. Invalid call to strlen (0 arguments instead of 1)
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<?php strlen(); ?>", 18, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &php_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_php(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Function [strlen] requires at least 1 arguments, but 0 were provided");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 3. Unknown function call
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<?php nonexistent_function(); ?>", 32, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &php_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_php(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Unknown PHP function: [nonexistent_function]");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 4. User defined function call should not raise error
    {
        textparser_t handle = nullptr;
        const char *code = "<?php function custom_func() {} custom_func(); ?>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &php_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_php(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }
}



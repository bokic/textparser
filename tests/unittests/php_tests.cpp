#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>

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

// Tokenizes with the v2 lexer and runs the php.legality validator. Owns the
// handle lifetime and returns the first validation message.
static std::string validate_php_message(const char *source, bool *has_error) {
    textparser_t handle = nullptr;
    if (textparser_openmem(source, (int)strlen(source), TEXTPARSER_ENCODING_LATIN1, &handle) != 0) {
        *has_error = true;
        return "openmem failed";
    }
    textparser_parse(handle, &php_definition);
    textparser_php_register_validators(handle);
    textparser_match_result result{};
    textparser_execute_language_grammar(handle, &php_definition, &result);
    textparser_validation *validation = textparser_validate_php(handle);
    std::string message;
    if (validation != nullptr) {
        *has_error = true;
        if (validation->len > 0) message = validation->items[0]->text;
        textparser_validation_clear(validation);
    } else {
        *has_error = false;
    }
    textparser_close(handle);
    return message;
}

TEST(parse_PHP, simple_php_block) {
    auto tokens = TextParser(R"(<?php echo "hello"; ?>)", &php_definition);
    ASSERT_EQ(tokens.count, 5);

    EXPECT_STREQ(tokens[0].type, "PHPOpenTag");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   5);

    EXPECT_STREQ(tokens[1].type, "EchoKeyword");
    EXPECT_EQ   (tokens[1].position, 6);
    EXPECT_EQ   (tokens[1].length,   4);

    EXPECT_STREQ(tokens[2].type, "DoubleString");
    EXPECT_EQ   (tokens[2].position, 11);
    EXPECT_EQ   (tokens[2].length,   7);

    EXPECT_STREQ(tokens[3].type, "Semicolon");
    EXPECT_EQ   (tokens[3].position, 18);
    EXPECT_EQ   (tokens[3].length,   1);

    EXPECT_STREQ(tokens[4].type, "PHPCloseTag");
    EXPECT_EQ   (tokens[4].position, 20);
    EXPECT_EQ   (tokens[4].length,   2);
}

TEST(parse_PHP, variables_and_blocks) {
    auto tokens = TextParser(R"(<?php
$x = 42;
if (true) {
    $y = false;
}
?>)", &php_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("LBrace"));
    EXPECT_TRUE(found.contains("RBrace"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Boolean"));
}

TEST(parse_PHP, double_string_is_a_single_token) {
    auto tokens = TextParser(R"(<?php $str = "hello $world"; ?>)", &php_definition);

    int double_str_idx = -1;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (std::string(tokens[i].type) == "DoubleString") {
            double_str_idx = (int)i;
            break;
        }
    }
    ASSERT_NE(double_str_idx, -1);
    EXPECT_EQ(tokens[double_str_idx].value, "\"hello $world\"");
    EXPECT_EQ(tokens[double_str_idx].position, 13);
    EXPECT_EQ(tokens[double_str_idx].length, 14);
}

TEST(parse_PHP, extra_tokens_coverage) {
    auto tokens = TextParser(R"(<?php
$arr = ['key' => 'value'];
$obj->method();
?>)", &php_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("DoubleArrow"));
    EXPECT_TRUE(found.contains("MemberAccess"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("Identifier"));
}

TEST(parse_PHP, comments_are_tokenized_by_the_contextual_lexer) {
    auto tokens = TextParser(R"(<?php // line
$x = 1; /* block */ ?>)", &php_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_FALSE(found.contains("MulOperator"));
}

TEST(parse_PHP, string_literals_keep_escapes) {
    auto tokens = TextParser(R"(<?php
$s1 = 'escaped \' \\ string';
$s2 = "escaped \" \$ \\ \n \r \t string";
?>)", &php_definition);

    bool found_single = false;
    bool found_double = false;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (std::string(tokens[i].type) == "SingleString") {
            found_single = true;
            EXPECT_NE(tokens[i].value.find("escaped"), std::string::npos);
        }
        if (std::string(tokens[i].type) == "DoubleString") {
            found_double = true;
            EXPECT_NE(tokens[i].value.find("escaped"), std::string::npos);
        }
    }
    EXPECT_TRUE(found_single);
    EXPECT_TRUE(found_double);
}

TEST(validate_PHP, built_in_functions) {
    bool has_error = false;
    std::string message;

    // Valid call to strlen
    message = validate_php_message("<?php strlen('hello'); ?>", &has_error);
    EXPECT_FALSE(has_error);
    EXPECT_TRUE(message.empty());

    // Invalid call to strlen (0 arguments instead of 1)
    message = validate_php_message("<?php strlen(); ?>", &has_error);
    ASSERT_TRUE(has_error);
    EXPECT_EQ(message, "PHP2008: Function [strlen] requires at least 1 arguments, but 0 were provided");

    // The v2 validator intentionally does not flag unknown functions: a call to
    // a function defined in another file is indistinguishable from a typo.
    message = validate_php_message("<?php nonexistent_function(); ?>", &has_error);
    EXPECT_FALSE(has_error);
    EXPECT_TRUE(message.empty());

    // User defined function call should not raise error
    message = validate_php_message("<?php function custom_func() {} custom_func(); ?>", &has_error);
    EXPECT_FALSE(has_error);
    EXPECT_TRUE(message.empty());

    // Mixed-case built-in function names are recognized case-insensitively
    message = validate_php_message("<?php STRLEN('a'); StrToLower('B'); substr('x', 0); ?>", &has_error);
    EXPECT_FALSE(has_error);
    EXPECT_TRUE(message.empty());

    // `new ClassName(...)` is not a function call
    message = validate_php_message("<?php new Foo(); ?>", &has_error);
    EXPECT_FALSE(has_error);
    EXPECT_TRUE(message.empty());

    // Method call syntax -> and :: is not a function call
    message = validate_php_message("<?php $obj->method(); ClassName::staticMethod(); ?>", &has_error);
    EXPECT_FALSE(has_error);
    EXPECT_TRUE(message.empty());
}

TEST(validate_PHP, php85_signature_arity) {
    const struct { const char *source; bool valid; } cases[] = {
        {"<?php array_merge();", true},
        {"<?php array_merge([1], [2], [3]);", true},
        {"<?php array_diff([1]);", true},
        {"<?php array_diff();", false},
        {"<?php sprintf('literal');", true},
        {"<?php sprintf('%s %s', 'a', 'b');", true},
        {"<?php sprintf();", false},
        {"<?php max(1);", true},
        {"<?php strlen('a', 'b');", false},
        {"<?php sizeof([1]);", true},
        {"<?php sizeof();", false},
        {"<?php ldap_connect();", true},
        {"<?php ldap_connect('uri', 389);", true},
        {"<?php ldap_connect('uri', 389, 'wallet', 'password', 0);", true},
        {"<?php ldap_connect('uri', 389, 'wallet', 'password', 0, 1);", false},
        {"<?php cli_get_process_title();", true},
        {"<?php cli_get_process_title(1);", false},
        {"<?php dl();", false},
    };
    for (const auto &test : cases) {
        SCOPED_TRACE(test.source);
        textparser_t handle = nullptr;
        ASSERT_EQ(textparser_openmem(test.source, strlen(test.source), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &php_definition), 0);
        ASSERT_EQ(textparser_php_register_validators(handle), 0);
        textparser_match_result result{};
        ASSERT_EQ(textparser_execute_language_grammar(handle, &php_definition, &result), 0);
        textparser_validation *validation = textparser_validate_php(handle);
        EXPECT_EQ(validation == nullptr, test.valid);
        if (validation) textparser_validation_clear(validation);
        textparser_close(handle);
    }
}

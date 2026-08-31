#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>

static char *dummy_identifier_decoder(textparser_t parser, const char *raw_text, size_t length, void *user_data) {
    (void)parser;
    (void)user_data;
    if (!raw_text || length == 0) return nullptr;
    char *buf = (char *)malloc(length + 1);
    if (!buf) return nullptr;
    memcpy(buf, raw_text, length);
    buf[length] = '\0';
    return buf;
}

static bool dummy_numeric_validator(textparser_t parser, const char *raw_text, size_t length, const char **out_error, void *user_data) {
    (void)parser;
    (void)user_data;
    if (!raw_text || length == 0) {
        if (out_error) *out_error = "Empty numeric literal";
        return false;
    }
    for (size_t i = 0; i < length; i++) {
        if (!isdigit((unsigned char)raw_text[i]) && raw_text[i] != '.' && raw_text[i] != 'e' && raw_text[i] != 'E' && raw_text[i] != '+' && raw_text[i] != '-') {
            if (out_error) *out_error = "Invalid character in numeric literal";
            return false;
        }
    }
    return true;
}

TEST(lexer_modes, mode_stack_push_pop_and_rollback) {
    textparser_t handle = nullptr;
    const char *code = "{}";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_STREQ(textparser_get_current_mode(handle), "default");

    EXPECT_EQ(textparser_push_mode(handle, "JSXTag"), 0);
    EXPECT_STREQ(textparser_get_current_mode(handle), "JSXTag");

    EXPECT_EQ(textparser_push_mode(handle, "TemplateText"), 0);
    EXPECT_STREQ(textparser_get_current_mode(handle), "TemplateText");

    EXPECT_EQ(textparser_pop_mode(handle), 0);
    EXPECT_STREQ(textparser_get_current_mode(handle), "JSXTag");

    EXPECT_EQ(textparser_pop_mode(handle), 0);
    EXPECT_STREQ(textparser_get_current_mode(handle), "default");

    // Pop on empty
    EXPECT_NE(textparser_pop_mode(handle), 0);

    textparser_close(handle);
}

TEST(lexer_modes, lexical_goals) {
    textparser_t handle = nullptr;
    const char *code = "{}";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_EQ(textparser_get_lexical_goal(handle), nullptr);

    textparser_set_lexical_goal(handle, "ExpressionStart");
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "ExpressionStart");

    textparser_set_lexical_goal(handle, "ExpressionContinuation");
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "ExpressionContinuation");

    textparser_set_lexical_goal(handle, nullptr);
    EXPECT_EQ(textparser_get_lexical_goal(handle), nullptr);

    textparser_close(handle);
}

TEST(lexer_modes, trivia_line_terminators) {
    textparser_t handle = nullptr;
    const char *code = "a = 1;\n\nb = 2;";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    // Span across newline
    EXPECT_TRUE(textparser_has_line_terminator_between(handle, 5, 9));

    // Span within single line "a = 1;"
    EXPECT_FALSE(textparser_has_line_terminator_between(handle, 0, 5));

    textparser_close(handle);
}

TEST(lexer_modes, decoders_and_validators) {
    textparser_t handle = nullptr;
    const char *code = "{\"id\": 123}";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_EQ(textparser_register_decoder(handle, "ecmascript.identifier", dummy_identifier_decoder, nullptr), 0);
    EXPECT_EQ(textparser_register_validator(handle, "ecmascript.numericLiteral", dummy_numeric_validator, nullptr), 0);

    // Test decoder
    char *decoded = textparser_decode_token(handle, "ecmascript.identifier", "myVar", 5);
    ASSERT_NE(decoded, nullptr);
    EXPECT_STREQ(decoded, "myVar");
    free(decoded);

    // Test validator with valid numeric literal
    const char *err_msg = nullptr;
    EXPECT_TRUE(textparser_validate_token(handle, "ecmascript.numericLiteral", "123.45", 6, &err_msg));
    EXPECT_EQ(err_msg, nullptr);

    // Test validator with invalid numeric literal
    EXPECT_FALSE(textparser_validate_token(handle, "ecmascript.numericLiteral", "123a45", 6, &err_msg));
    EXPECT_NE(err_msg, nullptr);

    textparser_close(handle);
}

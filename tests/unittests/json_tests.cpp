#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <json_definition.json.h>


TEST(parse_JSON, empty_array) {
    auto tokens = TextParser(R"([])", &json_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "Array");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   2);
    EXPECT_EQ   (tokens[0].children, 0);
}

TEST(parse_JSON, empty_object) {
    auto tokens = TextParser(R"({})", &json_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "Object");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   2);
    EXPECT_EQ   (tokens[0].children, 0);
}

TEST(parse_JSON, simple_array) {
    auto tokens = TextParser(R"([1,2,3])", &json_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "Array");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   7);
    EXPECT_EQ   (tokens[0].children, 5);

    EXPECT_STREQ(tokens[0][0].type, "Number");
    EXPECT_EQ   (tokens[0][0].position, 1);
    EXPECT_EQ   (tokens[0][0].length,   1);
    EXPECT_EQ   (tokens[0][0].children, 0);

    EXPECT_STREQ(tokens[0][1].type, "ValueSeparator");
    EXPECT_EQ   (tokens[0][1].position, 2);
    EXPECT_EQ   (tokens[0][1].length,   1);
    EXPECT_EQ   (tokens[0][1].children, 0);

    EXPECT_STREQ(tokens[0][2].type, "Number");
    EXPECT_EQ   (tokens[0][2].position, 3);
    EXPECT_EQ   (tokens[0][2].length,   1);
    EXPECT_EQ   (tokens[0][2].children, 0);

    EXPECT_STREQ(tokens[0][3].type, "ValueSeparator");
    EXPECT_EQ   (tokens[0][3].position, 4);
    EXPECT_EQ   (tokens[0][3].length,   1);
    EXPECT_EQ   (tokens[0][3].children, 0);

    EXPECT_STREQ(tokens[0][4].type, "Number");
    EXPECT_EQ   (tokens[0][4].position, 5);
    EXPECT_EQ   (tokens[0][4].length,   1);
    EXPECT_EQ   (tokens[0][4].children, 0);
}

TEST(parse_JSON, simple_object) {
    auto tokens = TextParser(R"({"key": "value"})", &json_definition);
    EXPECT_EQ(tokens.count, 1);
    

}

TEST(parse_JSON, unicode_object) {
    const char16_t text[] = u"{\"key\": \"value\"}";
    size_t byte_len = (sizeof(text) / sizeof(char16_t) - 1) * sizeof(char16_t);
    
    textparser_t handle = nullptr;
    int err = textparser_openmem((const char *)text, byte_len, TEXTPARSER_ENCODING_UNICODE, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);
    
    err = textparser_parse(handle, &json_definition);
    ASSERT_EQ(err, 0);
    
    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, first), "Object");
    EXPECT_EQ(first->position, 0);
    EXPECT_EQ(first->len, byte_len / sizeof(uint16_t));
    
    textparser_token_item *child = first->child;
    ASSERT_NE(child, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, child), "Key");
    
    char *key_text = textparser_get_token_text(handle, child);
    ASSERT_NE(key_text, nullptr);
    EXPECT_STREQ(key_text, "\"key\"");
    free(key_text);
    
    textparser_close(handle);
}

TEST(parse_JSON, utf32_object) {
    const char32_t text[] = U"{\"key\": \"value\"}";
    size_t byte_len = (sizeof(text) / sizeof(char32_t) - 1) * sizeof(char32_t);
    
    textparser_t handle = nullptr;
    int err = textparser_openmem((const char *)text, byte_len, TEXTPARSER_ENCODING_UTF_32, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);
    
    err = textparser_parse(handle, &json_definition);
    ASSERT_EQ(err, 0);
    
    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, first), "Object");
    EXPECT_EQ(first->position, 0);
    EXPECT_EQ(first->len, byte_len / sizeof(uint32_t));
    
    textparser_token_item *child = first->child;
    ASSERT_NE(child, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, child), "Key");
    
    char *key_text = textparser_get_token_text(handle, child);
    ASSERT_NE(key_text, nullptr);
    EXPECT_STREQ(key_text, "\"key\"");
    free(key_text);
    
    textparser_close(handle);
}

TEST(parse_JSON, unicode_non_ascii) {
    // String contains non-ASCII characters: Greek Omega 'Ω' (U+03A9) and Gothic '𐍈' (U+10348)
    // UTF-16 representation of: {"Ω𐍈": "value"}
    const char16_t text[] = u"{\"\u03A9\U00010348\": \"value\"}";
    size_t byte_len = (sizeof(text) / sizeof(char16_t) - 1) * sizeof(char16_t);
    
    textparser_t handle = nullptr;
    int err = textparser_openmem((const char *)text, byte_len, TEXTPARSER_ENCODING_UNICODE, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);
    
    err = textparser_parse(handle, &json_definition);
    ASSERT_EQ(err, 0);
    
    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    
    textparser_token_item *child = first->child;
    ASSERT_NE(child, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, child), "Key");
    
    uint16_t *key_text = textparser_get_token_text16(handle, child);
    ASSERT_NE(key_text, nullptr);
    // Expected char16_t array is: { '"', 0x03A9, 0xD800, 0xDF48, '"', 0 }
    EXPECT_EQ(key_text[0], '"');
    EXPECT_EQ(key_text[1], 0x03A9);
    EXPECT_EQ(key_text[2], 0xD800);
    EXPECT_EQ(key_text[3], 0xDF48);
    EXPECT_EQ(key_text[4], '"');
    EXPECT_EQ(key_text[5], 0);
    free(key_text);
    
    textparser_close(handle);
}

TEST(parse_JSON, utf32_non_ascii) {
    // UTF-32 representation of: {"Ω𐍈": "value"}
    const char32_t text[] = U"{\"\u03A9\U00010348\": \"value\"}";
    size_t byte_len = (sizeof(text) / sizeof(char32_t) - 1) * sizeof(char32_t);
    
    textparser_t handle = nullptr;
    int err = textparser_openmem((const char *)text, byte_len, TEXTPARSER_ENCODING_UTF_32, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);
    
    err = textparser_parse(handle, &json_definition);
    ASSERT_EQ(err, 0);
    
    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    
    textparser_token_item *child = first->child;
    ASSERT_NE(child, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, child), "Key");
    
    uint32_t *key_text = textparser_get_token_text32(handle, child);
    ASSERT_NE(key_text, nullptr);
    // Expected char32_t array is: { '"', 0x03A9, 0x10348, '"', 0 }
    EXPECT_EQ(key_text[0], '"');
    EXPECT_EQ(key_text[1], 0x03A9);
    EXPECT_EQ(key_text[2], 0x10348);
    EXPECT_EQ(key_text[3], '"');
    EXPECT_EQ(key_text[4], 0);
    free(key_text);
    
    textparser_close(handle);
}

#include <textparser-json.h>

TEST(parse_JSON, runtime_load_definition_from_string) {
    const char *json_def_str = R"({
        "name": "test_lang",
        "version": 2.5,
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": ["MyToken"],
        "otherTextInside": true,
        "tokens": {
            "MyToken": {
                "type": "SimpleToken",
                "startRegex": "hello",
                "textColor": "0x123456",
                "textBackground": "0x789abc",
                "textFlags": "3"
            }
        }
    })";

    textparser_language_definition *definition = nullptr;
    int err = textparser_json_load_language_definition_from_string(json_def_str, &definition);
    ASSERT_EQ(err, 0);
    ASSERT_NE(definition, nullptr);

    EXPECT_STREQ(definition->name, "test_lang");
    EXPECT_DOUBLE_EQ(definition->version, 2.5);
    EXPECT_TRUE(definition->case_sensitivity);
    ASSERT_NE(definition->default_file_extensions, nullptr);
    EXPECT_STREQ(definition->default_file_extensions[0], "txt");
    EXPECT_EQ(definition->default_text_encoding, TEXTPARSER_ENCODING_UTF_8);
    EXPECT_TRUE(definition->other_text_inside);

    ASSERT_NE(definition->tokens, nullptr);
    EXPECT_STREQ(definition->tokens[0].name, "MyToken");
    EXPECT_EQ(definition->tokens[0].type, TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN);
    EXPECT_STREQ(definition->tokens[0].start_regex, "hello");
    EXPECT_EQ(definition->tokens[0].text_color, 0x123456);
    EXPECT_EQ(definition->tokens[0].text_background, 0x789abc);
    EXPECT_EQ(definition->tokens[0].text_flags, 3);

    textparser_free_language_definition(definition);
}


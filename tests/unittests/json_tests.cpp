#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>
#include <json.h>

static bool json_has_unprocessed(const textparser_token_item *token) {
    for (; token != nullptr; token = token->next) {
        if (token->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED)
            return true;
        if (json_has_unprocessed(token->child))
            return true;
    }
    return false;
}

TEST(parse_JSON, empty_array) {
    auto tokens = TextParser(R"([])", &json_definition);
    EXPECT_EQ(tokens.count, 2);

    EXPECT_STREQ(tokens[0].type, "Array_Start");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   1);

    EXPECT_STREQ(tokens[1].type, "Array_End");
    EXPECT_EQ   (tokens[1].position, 1);
    EXPECT_EQ   (tokens[1].length,   1);
}

TEST(parse_JSON, empty_object) {
    auto tokens = TextParser(R"({})", &json_definition);
    EXPECT_EQ(tokens.count, 2);

    EXPECT_STREQ(tokens[0].type, "Object_Start");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   1);

    EXPECT_STREQ(tokens[1].type, "Object_End");
    EXPECT_EQ   (tokens[1].position, 1);
    EXPECT_EQ   (tokens[1].length,   1);
}

TEST(parse_JSON, simple_array) {
    auto tokens = TextParser(R"([1,2,3])", &json_definition);
    EXPECT_EQ(tokens.count, 7);

    EXPECT_STREQ(tokens[0].type, "Array_Start");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   1);

    EXPECT_STREQ(tokens[1].type, "Number");
    EXPECT_EQ   (tokens[1].position, 1);
    EXPECT_EQ   (tokens[1].length,   1);

    EXPECT_STREQ(tokens[2].type, "ValueSeparator");
    EXPECT_EQ   (tokens[2].position, 2);
    EXPECT_EQ   (tokens[2].length,   1);

    EXPECT_STREQ(tokens[3].type, "Number");
    EXPECT_EQ   (tokens[3].position, 3);
    EXPECT_EQ   (tokens[3].length,   1);

    EXPECT_STREQ(tokens[4].type, "ValueSeparator");
    EXPECT_EQ   (tokens[4].position, 4);
    EXPECT_EQ   (tokens[4].length,   1);

    EXPECT_STREQ(tokens[5].type, "Number");
    EXPECT_EQ   (tokens[5].position, 5);
    EXPECT_EQ   (tokens[5].length,   1);

    EXPECT_STREQ(tokens[6].type, "Array_End");
    EXPECT_EQ   (tokens[6].position, 6);
    EXPECT_EQ   (tokens[6].length,   1);
}

TEST(parse_JSON, simple_object) {
    auto tokens = TextParser(R"({"key": "value"})", &json_definition);
    EXPECT_EQ(tokens.count, 5);

    EXPECT_STREQ(tokens[0].type, "Object_Start");
    EXPECT_STREQ(tokens[1].type, "String");
    EXPECT_STREQ(tokens[1].value.c_str(), "\"key\"");
    EXPECT_STREQ(tokens[2].type, "KeyValueSeparator");
    EXPECT_STREQ(tokens[3].type, "String");
    EXPECT_STREQ(tokens[3].value.c_str(), "\"value\"");
    EXPECT_STREQ(tokens[4].type, "Object_End");
}

TEST(parse_JSON, negative_number_object_value) {
    auto tokens = TextParser(R"({"a": -5})", &json_definition);
    ASSERT_EQ(tokens.count, 5);
    EXPECT_STREQ(tokens[0].type, "Object_Start");
    EXPECT_STREQ(tokens[1].type, "String");
    EXPECT_STREQ(tokens[2].type, "KeyValueSeparator");
    EXPECT_STREQ(tokens[3].type, "Number");
    EXPECT_EQ   (tokens[3].position, 6);
    EXPECT_EQ   (tokens[3].length,   2);
    EXPECT_STREQ(tokens[3].value.c_str(), "-5");
    EXPECT_STREQ(tokens[4].type, "Object_End");
}

TEST(parse_JSON, negative_number_array_items) {
    auto tokens = TextParser(R"([-1,-2.5,-3e+2])", &json_definition);
    ASSERT_EQ(tokens.count, 7);
    EXPECT_STREQ(tokens[0].type, "Array_Start");

    EXPECT_STREQ(tokens[1].type, "Number");
    EXPECT_STREQ(tokens[1].value.c_str(), "-1");
    EXPECT_EQ   (tokens[1].position, 1);
    EXPECT_EQ   (tokens[1].length,   2);

    EXPECT_STREQ(tokens[2].type, "ValueSeparator");

    EXPECT_STREQ(tokens[3].type, "Number");
    EXPECT_STREQ(tokens[3].value.c_str(), "-2.5");
    EXPECT_EQ   (tokens[3].position, 4);
    EXPECT_EQ   (tokens[3].length,   4);

    EXPECT_STREQ(tokens[4].type, "ValueSeparator");

    EXPECT_STREQ(tokens[5].type, "Number");
    EXPECT_STREQ(tokens[5].value.c_str(), "-3e+2");
    EXPECT_EQ   (tokens[5].position, 9);
    EXPECT_EQ   (tokens[5].length,   5);

    EXPECT_STREQ(tokens[6].type, "Array_End");
}

TEST(parse_JSON, negative_exponent_number_object_value) {
    auto tokens = TextParser(R"({"n": -1.5e+10})", &json_definition);
    ASSERT_EQ(tokens.count, 5);
    EXPECT_STREQ(tokens[3].type, "Number");
    EXPECT_STREQ(tokens[3].value.c_str(), "-1.5e+10");
}

TEST(parse_JSON, string_content_is_tokenized) {
    auto tokens = TextParser(R"({"k": "abc"})", &json_definition);
    ASSERT_EQ(tokens.count, 5);

    EXPECT_STREQ(tokens[3].type, "String");
    EXPECT_STREQ(tokens[3].value.c_str(), "\"abc\"");
    EXPECT_EQ   (tokens[3].position, 6);
    EXPECT_EQ   (tokens[3].length,   5);
}

TEST(parse_JSON, escaped_quote_inside_string) {
    auto tokens = TextParser(R"({"k": "a\"b"})", &json_definition);
    ASSERT_EQ(tokens.count, 5);

    EXPECT_STREQ(tokens[3].type, "String");
    EXPECT_STREQ(tokens[3].value.c_str(), "\"a\\\"b\"");
    EXPECT_EQ   (tokens[3].position, 6);
    EXPECT_EQ   (tokens[3].length,   6);
}

TEST(parse_JSON, escaped_backslash_inside_string) {
    auto tokens = TextParser(R"({"k": "a\\b"})", &json_definition);
    ASSERT_EQ(tokens.count, 5);

    EXPECT_STREQ(tokens[3].type, "String");
    EXPECT_STREQ(tokens[3].value.c_str(), "\"a\\\\b\"");
    EXPECT_EQ   (tokens[3].position, 6);
    EXPECT_EQ   (tokens[3].length,   6);
}

TEST(parse_JSON, valid_string_has_no_unprocessed) {
    auto tokens = TextParser(R"({"k": "a\"b\\c"})", &json_definition);
    ASSERT_EQ(tokens.count, 5);
    EXPECT_FALSE(json_has_unprocessed(tokens[0].raw_token()));
}

TEST(parse_JSON, unquoted_literal_is_unprocessed) {
    auto tokens = TextParser(R"({"k": invalid_word})", &json_definition);
    EXPECT_TRUE(json_has_unprocessed(tokens[0].raw_token()));
}

TEST(parse_JSON, unexpected_text_in_object_is_unprocessed) {
    auto tokens = TextParser(R"({"x": @})", &json_definition);
    ASSERT_EQ(tokens.count, 4);
    EXPECT_TRUE(json_has_unprocessed(tokens[0].raw_token()));
}

TEST(parse_JSON, lone_minus_is_not_a_number) {
    auto tokens = TextParser(R"({"a": -})", &json_definition);
    ASSERT_EQ(tokens.count, 4);
    EXPECT_STREQ(tokens[0].type, "Object_Start");
    // The stray '-' must remain an unprocessed leaf, not become a Number token.
    EXPECT_FALSE(has_token_type(tokens, "Number"));
    EXPECT_TRUE(json_has_unprocessed(tokens[0].raw_token()));
}

TEST(parse_JSON, multiline_object) {
    auto tokens = TextParser("{\n  \"key\": \"value\",\n  \"num\": 123\n}", &json_definition);
    EXPECT_EQ(tokens.count, 9);
    EXPECT_STREQ(tokens[0].type, "Object_Start");
    EXPECT_STREQ(tokens[8].type, "Object_End");
}

TEST(parse_JSON, multiline_nested_array_and_object_values) {
    auto tokens = TextParser("{\n  \"a\": [\n    \"x\"\n  ],\n  \"b\": {\n    \"c\": 1\n  }\n}", &json_definition);
    ASSERT_EQ(tokens.count, 15);
    EXPECT_STREQ(tokens[0].type, "Object_Start");
    EXPECT_STREQ(tokens[1].type, "String");
    EXPECT_STREQ(tokens[3].type, "Array_Start");
    EXPECT_STREQ(tokens[4].type, "String");
    EXPECT_STREQ(tokens[5].type, "Array_End");
    EXPECT_STREQ(tokens[9].type, "Object_Start");
    EXPECT_STREQ(tokens[10].type, "String");
    EXPECT_STREQ(tokens[12].type, "Number");
    EXPECT_STREQ(tokens[13].type, "Object_End");
    EXPECT_STREQ(tokens[14].type, "Object_End");
}

TEST(parse_JSON, multiline_array_without_trailing_newline) {
    auto tokens = TextParser("{\n  \"a\": [\n    1\n  ]\n}", &json_definition);
    ASSERT_EQ(tokens.count, 7);
    EXPECT_STREQ(tokens[0].type, "Object_Start");
    EXPECT_STREQ(tokens[3].type, "Array_Start");
    EXPECT_STREQ(tokens[4].type, "Number");
    EXPECT_STREQ(tokens[5].type, "Array_End");
    EXPECT_STREQ(tokens[6].type, "Object_End");
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
    
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, first), "Object_Start");
    EXPECT_EQ(textparser_get_token_position(first), 0u);
    EXPECT_EQ(first->len, 1);
    
    textparser_token_item *key_tok = first->next;
    while (key_tok && key_tok->token_id < 0) key_tok = key_tok->next;
    ASSERT_NE(key_tok, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, key_tok), "String");
    
    char *key_text = textparser_get_token_text(handle, key_tok);
    ASSERT_NE(key_text, nullptr);
    EXPECT_STREQ(key_text, "\"key\"");
    textparser_free_token_text(key_text);
    
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
    
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, first), "Object_Start");
    EXPECT_EQ(textparser_get_token_position(first), 0u);
    EXPECT_EQ(first->len, 1);
    
    textparser_token_item *key_tok = first->next;
    while (key_tok && key_tok->token_id < 0) key_tok = key_tok->next;
    ASSERT_NE(key_tok, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, key_tok), "String");
    
    char *key_text = textparser_get_token_text(handle, key_tok);
    ASSERT_NE(key_text, nullptr);
    EXPECT_STREQ(key_text, "\"key\"");
    textparser_free_token_text(key_text);
    
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
    
    textparser_token_item *key_tok = first->next;
    while (key_tok && key_tok->token_id < 0) key_tok = key_tok->next;
    ASSERT_NE(key_tok, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, key_tok), "String");
    
    uint16_t *key_text = textparser_get_token_text16(handle, key_tok);
    ASSERT_NE(key_text, nullptr);
    // Expected char16_t array is: { '"', 0x03A9, 0xD800, 0xDF48, '"', 0 }
    EXPECT_EQ(key_text[0], '"');
    EXPECT_EQ(key_text[1], 0x03A9);
    EXPECT_EQ(key_text[2], 0xD800);
    EXPECT_EQ(key_text[3], 0xDF48);
    EXPECT_EQ(key_text[4], '"');
    EXPECT_EQ(key_text[5], 0);
    textparser_free_token_text(key_text);
    
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
    
    textparser_token_item *key_tok = first->next;
    while (key_tok && key_tok->token_id < 0) key_tok = key_tok->next;
    ASSERT_NE(key_tok, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&json_definition, key_tok), "String");
    
    uint32_t *key_text = textparser_get_token_text32(handle, key_tok);
    ASSERT_NE(key_text, nullptr);
    // Expected char32_t array is: { '"', 0x03A9, 0x10348, '"', 0 }
    EXPECT_EQ(key_text[0], '"');
    EXPECT_EQ(key_text[1], 0x03A9);
    EXPECT_EQ(key_text[2], 0x10348);
    EXPECT_EQ(key_text[3], '"');
    EXPECT_EQ(key_text[4], 0);
    textparser_free_token_text(key_text);
    
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
                "textFlags": "3",
                "delimiterTextColor": "0xfe9876",
                "delimiterTextBackground": "0x543210",
                "delimiterTextFlags": "5"
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
    EXPECT_EQ(definition->tokens[0].delimiter_text_color, 0xfe9876);
    EXPECT_EQ(definition->tokens[0].delimiter_text_background, 0x543210);
    EXPECT_EQ(definition->tokens[0].delimiter_text_flags, 5);

    textparser_free_language_definition(definition);
}

TEST(parse_JSON, recursive_group_find_token_depth_limit) {
    const char *json_def_str = R"({
        "name": "recursive_lang",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": ["Value"],
        "otherTextInside": false,
        "tokens": {
            "Value": {
                "type": "Group",
                "nestedTokens": ["Value"]
            }
        }
    })";

    textparser_language_definition *definition = nullptr;
    int err = textparser_json_load_language_definition_from_string(json_def_str, &definition);
    ASSERT_EQ(err, 0);
    ASSERT_NE(definition, nullptr);

    textparser_t handle = nullptr;
    const char *text = "test input";
    err = textparser_openmem(text, strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    // Parsing should complete safely without stack overflow and yield no matched semantic tokens
    err = textparser_parse(handle, definition);
    EXPECT_EQ(err, 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    while (first && first->token_id < 0) first = first->next;
    EXPECT_EQ(first, nullptr);

    textparser_close(handle);
    textparser_free_language_definition(definition);
}

TEST(parse_JSON, non_string_start_tokens_element) {
    const char *json_def_str = R"({
        "name": "test_lang",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": [123],
        "otherTextInside": true,
        "tokens": {
            "MyToken": {
                "type": "SimpleToken",
                "startRegex": "hello"
            }
        }
    })";

    textparser_language_definition *definition = nullptr;
    int err = textparser_json_load_language_definition_from_string(json_def_str, &definition);
    EXPECT_EQ(err, TEXTPARSER_JSON_STARTS_WITH_ELEMENT_NOT_STRING);
    EXPECT_EQ(definition, nullptr);
}

TEST(parse_JSON, non_string_nested_tokens_element) {
    const char *json_def_str = R"({
        "name": "test_lang",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": ["MyToken"],
        "otherTextInside": true,
        "tokens": {
            "MyToken": {
                "type": "Group",
                "nestedTokens": [null]
            }
        }
    })";

    textparser_language_definition *definition = nullptr;
    int err = textparser_json_load_language_definition_from_string(json_def_str, &definition);
    EXPECT_EQ(err, TEXTPARSER_JSON_NESTED_TOKENS_ELEMENT_NOT_STRING);
    EXPECT_EQ(definition, nullptr);
}

TEST(parse_JSON, empty_segment_language_parsing) {
    // 1. Present emptySegmentLanguage
    const char *json_present = R"({
        "name": "test_lang",
        "version": 1.0,
        "emptySegmentLanguage": "html",
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": ["MyToken"],
        "tokens": {
            "MyToken": {
                "type": "SimpleToken",
                "startRegex": "hello"
            }
        }
    })";

    textparser_language_definition *def1 = nullptr;
    int err1 = textparser_json_load_language_definition_from_string(json_present, &def1);
    ASSERT_EQ(err1, 0);
    ASSERT_NE(def1, nullptr);
    EXPECT_DOUBLE_EQ(def1->version, 1.0);
    ASSERT_NE(def1->empty_segment_language, nullptr);
    EXPECT_STREQ(def1->empty_segment_language, "html");
    textparser_free_language_definition(def1);

    // 2. Absent emptySegmentLanguage
    const char *json_none = R"({
        "name": "test_lang2",
        "version": 4.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": ["MyToken"],
        "tokens": {
            "MyToken": {
                "type": "SimpleToken",
                "startRegex": "hello"
            }
        }
    })";

    textparser_language_definition *def2 = nullptr;
    int err2 = textparser_json_load_language_definition_from_string(json_none, &def2);
    ASSERT_EQ(err2, 0);
    ASSERT_NE(def2, nullptr);
    EXPECT_DOUBLE_EQ(def2->version, 4.0);
    EXPECT_EQ(def2->empty_segment_language, nullptr);
    textparser_free_language_definition(def2);
}

TEST(parse_JSON, json_strerror_messages) {
    EXPECT_STREQ(textparser_json_strerror(TEXTPARSER_JSON_NO_ERROR), "Success");
    EXPECT_STREQ(textparser_json_strerror(TEXTPARSER_JSON_ROOT_OBJ_IS_NULL), "JSON root object is null");
    EXPECT_STREQ(textparser_json_strerror(TEXTPARSER_JSON_DEFINITION_IS_NULL), "Definition pointer is null");
    EXPECT_STREQ(textparser_json_strerror(TEXTPARSER_JSON_OUT_OF_MEMORY), "Out of memory");
    EXPECT_STREQ(textparser_json_strerror(TEXTPARSER_JSON_NAME_NOT_FOUND), "Mandatory field 'name' not found or invalid");
    EXPECT_STREQ(textparser_json_strerror(TEXTPARSER_JSON_INVALID_TOKEN_TYPE), "Invalid token type");
    EXPECT_STREQ(textparser_json_strerror(9999), "Unknown JSON parser error");
}

TEST(parse_JSON, corner_cases_and_edge_conditions) {
    const char *json_src = R"json({
        "empty_str": "",
        "bool_true": true,
        "bool_false": false,
        "null_val": null,
        "deep_nest": [[[[{}]]]],
        "exponential": 1.25e-10,
        "zero": 0,
        "negative_float": -0.005,
        "utf8_emoji": "hello 🚀 world"
    })json";

    auto tokens = TextParser(json_src, &json_definition);
    ASSERT_GT(tokens.count, 0);
    EXPECT_FALSE(json_has_unprocessed(tokens[0].raw_token()));

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (tokens[i].type) found.insert(tokens[i].type);
    }

    EXPECT_TRUE(found.contains("Object_Start"));
    EXPECT_TRUE(found.contains("Object_End"));
    EXPECT_TRUE(found.contains("Array_Start"));
    EXPECT_TRUE(found.contains("Array_End"));
    EXPECT_TRUE(found.contains("String"));
    EXPECT_TRUE(found.contains("Bool"));
    EXPECT_TRUE(found.contains("Null"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("KeyValueSeparator"));
    EXPECT_TRUE(found.contains("ValueSeparator"));
}

struct JSONValidationFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition_dynamic = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/json_definition.json", &json_definition_dynamic),
                      TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json_definition_dynamic, nullptr);
            ASSERT_NE(json_definition_dynamic->grammar, nullptr);
        }
    }

    void TearDown() override {
        if (json_definition_dynamic != nullptr) {
            textparser_free_language_definition(json_definition_dynamic);
            json_definition_dynamic = nullptr;
        }
    }

    const textparser_language_definition *get_definition() const {
        return GetParam() ? json_definition_dynamic : &json_definition;
    }

    std::vector<std::string> validate(const char *source) {
        textparser_t handle = nullptr;
        EXPECT_EQ(textparser_openmem(source, (int)strlen(source), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
        EXPECT_EQ(textparser_parse(handle, get_definition()), 0);
        EXPECT_EQ(textparser_json_register_validators(handle), 0);
        textparser_match_result match{};
        EXPECT_EQ(textparser_execute_language_grammar(handle, get_definition(), &match), 0);

        std::vector<std::string> messages;
        textparser_validation *val = textparser_validate_json(handle);
        if (val != nullptr) {
            for (int i = 0; i < val->len; i++) {
                if (val->items[i]->text) {
                    messages.push_back(val->items[i]->text);
                }
            }
            textparser_validation_clear(val);
        }
        textparser_close(handle);
        return messages;
    }

    bool has_error_code(const std::vector<std::string> &errors, const std::string &code) {
        for (const auto &err : errors) {
            if (err.find(code) != std::string::npos) return true;
        }
        return false;
    }
};

INSTANTIATE_TEST_SUITE_P(JSONDefinitionSources, JSONValidationFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(JSONValidationFixture, valid_json_produces_no_diagnostics) {
    auto clean1 = validate("{}");
    EXPECT_TRUE(clean1.empty());

    auto clean2 = validate("[]");
    EXPECT_TRUE(clean2.empty());

    auto clean3 = validate(R"({
        "name": "textparser",
        "version": 2.0,
        "is_active": true,
        "tags": ["parser", "cst", "ast", null],
        "nested": {
            "escape": "line1\nline2\t\"quoted\"",
            "unicode": "hello \u0041 \uD83D\uDE80"
        }
    })");
    EXPECT_TRUE(clean3.empty());
}

TEST_P(JSONValidationFixture, duplicate_key_in_object_rejected_JSON1001) {
    auto err1 = validate(R"({"a": 1, "b": 2, "a": 3})");
    EXPECT_TRUE(has_error_code(err1, "JSON1001"));

    auto err_nested = validate(R"({"outer": {"k": 1, "k": 2}})");
    EXPECT_TRUE(has_error_code(err_nested, "JSON1001"));

    // Different objects having the same key name is valid
    auto clean_diff_objs = validate(R"({"obj1": {"id": 1}, "obj2": {"id": 2}})");
    EXPECT_TRUE(clean_diff_objs.empty());
}

TEST_P(JSONValidationFixture, invalid_string_escape_rejected_JSON1002) {
    auto err_x = validate(R"({"a": "bad\x12"})");
    EXPECT_TRUE(has_error_code(err_x, "JSON1002"));

    auto err_sq = validate(R"({"a": "bad\'quote"})");
    EXPECT_TRUE(has_error_code(err_sq, "JSON1002"));

    auto err_null = validate(R"({"a": "bad\0char"})");
    EXPECT_TRUE(has_error_code(err_null, "JSON1002"));

    auto err_u_short = validate(R"({"a": "bad\u12"})");
    EXPECT_TRUE(has_error_code(err_u_short, "JSON1002"));

    auto err_u_hex = validate(R"({"a": "bad\u12ZZ"})");
    EXPECT_TRUE(has_error_code(err_u_hex, "JSON1002"));

    auto err_isolated_high = validate("{\"a\": \"bad\\uD800abc\"}");
    EXPECT_TRUE(has_error_code(err_isolated_high, "JSON1002"));

    auto err_isolated_low = validate("{\"a\": \"bad\\uDC00\"}");
    EXPECT_TRUE(has_error_code(err_isolated_low, "JSON1002"));

    auto clean_paired = validate("{\"a\": \"emoji \\uD83D\\uDE80\"}");
    EXPECT_TRUE(clean_paired.empty());
}

TEST_P(JSONValidationFixture, unescaped_control_character_rejected_JSON1002) {
    // String containing raw literal ASCII newline (0x0A)
    const char raw_nl[] = "{\"a\": \"hello\nworld\"}";
    auto err_nl = validate(raw_nl);
    EXPECT_TRUE(has_error_code(err_nl, "JSON1002"));

    // String containing raw literal ASCII tab (0x09)
    const char raw_tab[] = "{\"a\": \"hello\tworld\"}";
    auto err_tab = validate(raw_tab);
    EXPECT_TRUE(has_error_code(err_tab, "JSON1002"));
}









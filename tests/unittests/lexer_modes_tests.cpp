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

namespace {
int contextual_token_id(const textparser_language_definition *definition, const char *name) {
    for (int i = 0; definition->tokens[i].name != nullptr; i++)
        if (strcmp(definition->tokens[i].name, name) == 0) return i;
    return -1;
}

const char *contextual_lexer_json = R"json({
  "name":"contextual", "version":2, "caseSensitivity":true,
  "defaultFileExtensions":["txt"], "defaultTextEncoding":"utf-8",
  "otherTextInside":true,
  "lexer":{
    "initialMode":"default",
    "tokens":{
      "Open":{"regex":"<","pushMode":"tag"},
      "Text":{"regex":"[a-z]+"},
      "Name":{"regex":"[a-z]+","priority":5},
      "Close":{"regex":">","popMode":true},
      "Slash":{"regex":"/"},
      "Regex":{"regex":"/[^/]+/"}
    },
    "trivia":{"Space":{"regex":"[ \\t\\r\\n]+"}},
    "modes":{
      "default":{"tokens":["Open","Text","Slash"],"trivia":["Space"]},
      "tag":{"tokens":["Name","Close"],"trivia":["Space"]}
    },
    "goals":{"ExpressionStart":{"Slash":"Regex"}}
  }
})json";
} // namespace

TEST(lexer_modes, contextual_scanner_applies_modes_transitions_and_trivia) {
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(contextual_lexer_json, &definition), 0);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("< x > body", 10, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    const int children[] = {0, 1, 2, 3};
    const textparser_production productions[] = {
        {0, "Open", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Open"), -1},
        {1, "Name", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Name"), -1},
        {2, "Close", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Close"), -1},
        {3, "Text", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Text"), -1},
        {4, "Root", TEXTPARSER_PROD_SEQUENCE, children, 4, -1, -1},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 4, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 4u);
    EXPECT_STREQ(textparser_get_current_mode(parser.get()), "default");
    textparser_parser_state_view state{};
    ASSERT_EQ(parser.parser_state(&state), 0);
    EXPECT_EQ(state.source_offset, 10u);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(lexer_modes, lexical_goal_changes_scan_and_uses_separate_cache_entry) {
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(contextual_lexer_json, &definition), 0);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("/abc/", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_production slash[] = {
        {0, "Slash", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Slash"), -1},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(slash, 1, 0, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.node->len, 1u);

    textparser_set_lexical_goal(parser.get(), "ExpressionStart");
    textparser_production regex[] = {
        {0, "Regex", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Regex"), -1},
    };
    ASSERT_EQ(parser.execute_production(regex, 1, 0, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.node->len, 5u);
    EXPECT_EQ(result.node->token_id, contextual_token_id(definition, "Regex"));
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(lexer_modes, speculative_mode_transition_is_rolled_back) {
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(contextual_lexer_json, &definition), 0);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("< body", 6, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    const int failed[] = {0, 1};
    const int alternatives[] = {2, 3};
    const textparser_production productions[] = {
        {0, "Open", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Open"), -1},
        {1, "Close", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Close"), -1},
        {2, "Failed", TEXTPARSER_PROD_SEQUENCE, failed, 2, -1, -1},
        {3, "OpenFallback", TEXTPARSER_PROD_TOKEN, nullptr, 0, contextual_token_id(definition, "Open"), -1},
        {4, "Choice", TEXTPARSER_PROD_CHOICE, alternatives, 2, -1, -1},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 4, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_STREQ(textparser_get_current_mode(parser.get()), "tag");
    parser.reset();
    textparser_free_language_definition(definition);
}

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <cstring>
#include <string>

namespace {

std::string language_with_grammar(const std::string &grammar) {
    return R"json({
      "name":"json_grammar", "version":2, "caseSensitivity":true,
      "defaultFileExtensions":["txt"], "defaultTextEncoding":"utf-8",
      "startTokens":["A","B","C"], "otherTextInside":true,
      "tokens":{
        "A":{"type":"SimpleToken","startRegex":"a"},
        "B":{"type":"SimpleToken","startRegex":"b"},
        "C":{"type":"SimpleToken","startRegex":"c"}
      },
      "grammar":)json" + grammar + "}";
}

int load(const std::string &grammar, textparser_language_definition **definition) {
    std::string json = language_with_grammar(grammar);
    return textparser_json_load_language_definition_from_string(json.c_str(), definition);
}

} // namespace

TEST(json_grammar, loads_flattens_resolves_and_executes) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{
        "Root":{"sequence":[
          {"token":"A"},
          {"repeat":{"ref":"BToken"}},
          {"optional":{"token":"C"}}
        ]},
        "BToken":{"token":"B"}
      }
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(definition, nullptr);
    ASSERT_NE(definition->grammar, nullptr);
    EXPECT_EQ(definition->grammar->start_production, 0);
    EXPECT_EQ(definition->grammar->production_count, 7u);
    EXPECT_STREQ(definition->grammar->productions[0].name, "Root");
    EXPECT_EQ(definition->grammar->productions[0].kind, TEXTPARSER_PROD_SEQUENCE);

    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a b b c", 7, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 4u);
    ASSERT_NE(result.node, nullptr);
    EXPECT_EQ(result.node->len, 7u);

    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, loads_schema_v2_lexer_and_trivia_names) {
    const char *json = R"json({
      "formatVersion":2,
      "name":"schema_v2_minimal", "version":2, "caseSensitivity":true,
      "defaultFileExtensions":["txt"], "defaultTextEncoding":"utf-8",
      "otherTextInside":true,
      "lexer":{
        "tokens":{"A":{"regex":"a"}},
        "trivia":{"Comment":{"regex":"//[^\\r\\n]*"}}
      },
      "grammar":{
        "start":"Root",
        "productions":{"Root":{"choice":[{"token":"A"},{"token":"Comment"}]}}
      }
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(json, &definition), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(definition, nullptr);
    ASSERT_NE(definition->grammar, nullptr);
    ASSERT_NE(definition->tokens, nullptr);
    EXPECT_STREQ(definition->tokens[0].name, "A");
    EXPECT_STREQ(definition->tokens[1].name, "Comment");

    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a", 1, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 1u);

    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, validates_structure_and_names) {
    struct Case { const char *grammar; int expected; } cases[] = {
        {"[]", TEXTPARSER_JSON_GRAMMAR_NOT_OBJECT},
        {R"({"productions":{"Root":{"token":"A"}}})", TEXTPARSER_JSON_GRAMMAR_START_NOT_FOUND},
        {R"({"start":"Root"})", TEXTPARSER_JSON_GRAMMAR_PRODUCTIONS_NOT_OBJECT},
        {R"({"start":"Root","productions":{}})", TEXTPARSER_JSON_GRAMMAR_PRODUCTIONS_NOT_OBJECT},
        {R"({"start":"Missing","productions":{"Root":{"token":"A"}}})", TEXTPARSER_JSON_GRAMMAR_UNDEFINED_REFERENCE},
        {R"({"start":"Root","productions":{"Root":{"token":"Missing"}}})", TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN},
        {R"({"start":"Root","productions":{"Root":{"ref":"Missing"}}})", TEXTPARSER_JSON_GRAMMAR_UNDEFINED_REFERENCE},
        {R"({"start":"Root","productions":{"Root":{"token":"A","ref":"Root"}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"token":"A","lookahead":{"token":"A"}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"sequence":{}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"optional":[]}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
    };
    for (const auto &item : cases) {
        textparser_language_definition *definition = nullptr;
        EXPECT_EQ(load(item.grammar, &definition), item.expected) << item.grammar;
        EXPECT_EQ(definition, nullptr);
        EXPECT_STRNE(textparser_json_strerror(item.expected), "Unknown JSON parser error");
    }
}

TEST(json_grammar, rejects_nullable_repeat_and_left_recursion) {
    const char *nullable_repeat = R"({
      "start":"Root","productions":{
        "Root":{"repeat":{"optional":{"token":"A"}}}
      }
    })";
    textparser_language_definition *definition = nullptr;
    EXPECT_EQ(load(nullable_repeat, &definition), TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT);
    EXPECT_EQ(definition, nullptr);

    const char *left_recursive = R"({
      "start":"Root","productions":{
        "Root":{"choice":[{"ref":"Root"},{"token":"A"}]}
      }
    })";
    EXPECT_EQ(load(left_recursive, &definition), TEXTPARSER_JSON_GRAMMAR_LEFT_RECURSION);
    EXPECT_EQ(definition, nullptr);
}

TEST(json_grammar, accepts_recursion_after_consumption) {
    const char *right_recursive = R"({
      "start":"Root","productions":{
        "Root":{"choice":[
          {"token":"A"},
          {"sequence":[{"token":"A"},{"ref":"Root"}]}
        ]}
      }
    })";
    textparser_language_definition *definition = nullptr;
    EXPECT_EQ(load(right_recursive, &definition), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(definition, nullptr);
    textparser_free_language_definition(definition);
}

TEST(json_grammar, language_executor_rejects_missing_grammar) {
    textparser_language_definition *definition = nullptr;
    const std::string json = language_with_grammar("null");
    EXPECT_EQ(textparser_json_load_language_definition_from_string(json.c_str(), &definition),
              TEXTPARSER_JSON_GRAMMAR_NOT_OBJECT);
    EXPECT_EQ(definition, nullptr);

    textparser_match_result result{};
    EXPECT_EQ(textparser_execute_language_grammar(nullptr, nullptr, &result), -1);
}

namespace {
bool json_context_predicate(textparser_t parser,
                            const textparser_predicate_context *,
                            void *) {
    int64_t first = 0;
    int64_t second = 0;
    return textparser_context_get(parser, "First", &first) == 0 && first == 1 &&
           textparser_context_get(parser, "Second", &second) == 0 && second == 2;
}
} // namespace

TEST(json_grammar, loads_lookahead_predicates_contexts_and_commit) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{
        "Root":{"sequence":[
          {"lookahead":{"token":"A"}},
          {"not":{"token":"B"}},
          {"token":"A"},
          {"withContext":{"set":{"First":1,"Second":2},"sequence":[
            {"when":{"native":"test.context"}},
            {"commit":true},
            {"token":"B"}
          ]}}
        ]}
      }
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a b", 3, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(textparser_register_parser_predicate(
                  parser.get(), "test.context", json_context_predicate, nullptr), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 2u);
    EXPECT_TRUE(result.committed);
    int64_t value = 0;
    EXPECT_NE(textparser_context_get(parser.get(), "First", &value), 0);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, validates_advanced_construct_shapes_and_nullable_repeat) {
    struct Case { const char *grammar; int expected; } cases[] = {
        {R"({"start":"Root","productions":{"Root":{"lookahead":[]}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"commit":false}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"when":{"native":""}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"withContext":{"set":{},"ref":"Root"}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"withContext":{"set":{"X":"yes"},"sequence":[]}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"repeat":{"lookahead":{"token":"A"}}}}})",
         TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT},
    };
    for (const auto &item : cases) {
        textparser_language_definition *definition = nullptr;
        EXPECT_EQ(load(item.grammar, &definition), item.expected) << item.grammar;
        EXPECT_EQ(definition, nullptr);
    }
}

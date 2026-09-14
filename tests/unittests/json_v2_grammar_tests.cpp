#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <cstring>
#include <string>

namespace {

struct JSONGrammarFixture : testing::Test {
    textparser_language_definition *definition = nullptr;

    void SetUp() override {
        ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                      "definitions/json_definition.json", &definition),
                  TEXTPARSER_JSON_NO_ERROR);
        ASSERT_NE(definition, nullptr);
        ASSERT_NE(definition->grammar, nullptr);
    }

    void TearDown() override {
        if (definition != nullptr) {
            textparser_free_language_definition(definition);
            definition = nullptr;
        }
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(definition), 0);
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(definition, &result), 0);
        if (result.status == TEXTPARSER_MATCH_OK) {
            const textparser_lex_token *remaining = nullptr;
            int peek = textparser_lexer_peek(
                parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
            if (peek == 0 && remaining != nullptr) {
                result = {};
                result.status = TEXTPARSER_MATCH_NO;
            }
        }
        EXPECT_EQ(result.status, expected) << source;
        return result.node;
    }

    const textparser_node *find(const textparser_node *node, const char *kind) {
        if (!node) return nullptr;
        const char *name = textparser_grammar_node_name(parser.get(), node);
        if (name && std::strcmp(name, kind) == 0) return node;
        for (auto *child = node->child; child; child = child->next) {
            if (auto *found = find(child, kind)) return found;
        }
        return nullptr;
    }

    textparser::Parser parser;
    textparser_match_result result = {};
};

TEST_F(JSONGrammarFixture, parses_valid_documents) {
    for (const char *source : {
             "{}",
             "[]",
             "{\"a\": 1}",
             "[1, 2, 3]",
             "{\"a\": [true, false, null], \"b\": {\"c\": \"d\"}}",
             "{\"s\": \"hello \\\"world\\\"\"}",
             "{\"n\": -1.5e3}",
             "{\"empty\": {}, \"list\": []}",
             "  {\"x\": {}}  ",
             "[[[[1]]]]"})
        EXPECT_NE(parse_source(source), nullptr);
}

TEST_F(JSONGrammarFixture, rejects_invalid_documents) {
    for (const char *source : {
             "{\"a\":}",
             "{\"a\" 1}",
             "{,}",
             "[1 2]",
             "{\"a\":01}",
             "{\"a\":-}",
             "{\"a\":tru}",
             "[1,]",
             "{\"a\":1,}",
             "{\"a\":1",
             "[1, 2",
             "{\"a\"::1}",
             ""})
        parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_F(JSONGrammarFixture, builds_object_and_array_cst) {
    auto *root = parse_source("{\"a\": [1, true]}");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "Object");
    EXPECT_NE(find(root, "ObjectMembers"), nullptr);
    EXPECT_NE(find(root, "ObjectMember"), nullptr);
    EXPECT_NE(find(root, "Array"), nullptr);
    EXPECT_NE(find(root, "Number"), nullptr);
    EXPECT_NE(find(root, "Bool"), nullptr);
    EXPECT_NE(find(root, "String"), nullptr);

    auto *array = parse_source("[1, 2]");
    ASSERT_NE(array, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), array), "Array");
    EXPECT_NE(find(array, "ArrayElements"), nullptr);
}

TEST_F(JSONGrammarFixture, nested_values_are_grouped) {
    auto *root = parse_source("{\"a\": {\"b\": [1]}}");
    ASSERT_NE(root, nullptr);
    // Outer object contains the inner object and array as descendants.
    EXPECT_NE(find(root, "Array"), nullptr);
}

} // namespace

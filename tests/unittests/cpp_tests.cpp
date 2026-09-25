#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <cstring>

#include <cpp_definition.json.h>

TEST(parse_CPP, basic_cpp_program) {
    auto tokens = TextParser(R"(
// A LineComment to verify parsing
/* A BlockComment to
   verify parsing multi-line block comments */
#include <iostream>
using namespace std;

template<typename T>
class Hello {
public:
    void print() {
        std::cout << "Hello \n \"World\"!" << std::endl;
        char single = '\n'; // SingleString with nested StringEscape
        bool active = true; // Boolean
        bool inactive = false; // Boolean
        double pi = 3.14159; // Number (decimal float)
        int hexVal = 0x2A; // Number (hexadecimal integer)
        int binVal = 0b1010; // Number (binary integer)
        double sciVal = 1e-9; // Number (scientific notation)
    }
};
)", &cpp_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (tokens[i].type) {
            found.insert(tokens[i].type);
        }
    }

    EXPECT_TRUE(found.contains("Preprocessor"));
    EXPECT_TRUE(found.contains("UsingKeyword"));
    EXPECT_TRUE(found.contains("NamespaceKeyword"));
    EXPECT_TRUE(found.contains("TemplateKeyword"));
    EXPECT_TRUE(found.contains("TypenameKeyword"));
    EXPECT_TRUE(found.contains("ClassKeyword"));
    EXPECT_TRUE(found.contains("PublicKeyword"));
    EXPECT_TRUE(found.contains("VoidKeyword"));
    EXPECT_TRUE(found.contains("BoolKeyword"));
    EXPECT_TRUE(found.contains("DoubleKeyword"));
    EXPECT_TRUE(found.contains("IntKeyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("CharacterLiteral"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("ScopeResolution"));
    EXPECT_TRUE(found.contains("StandardType"));
    EXPECT_TRUE(found.contains("Identifier"));
}

static const textparser_node *find_node(textparser_t handle, const textparser_node *node, const char *kind) {
    if (!node) return nullptr;
    const char *name = textparser_grammar_node_name(handle, node);
    if (name && std::strcmp(name, kind) == 0) return node;
    for (auto *child = node->child; child; child = child->next) {
        if (auto *found = find_node(handle, child, kind)) return found;
    }
    return nullptr;
}

TEST(parse_CPP, template_vs_relational_disambiguation) {
    // 1. Template argument list should be parsed into TemplateArgumentList
    {
        textparser::Parser parser;
        const char *code = "std::vector<int> numbers;";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&cpp_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&cpp_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "TemplateArgumentList"), nullptr);
    }

    // 2. Relational comparisons should NOT have TemplateArgumentList
    {
        textparser::Parser parser;
        const char *code = "void f() { if (a < b && c > d) { return; } }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&cpp_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&cpp_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(find_node(parser.get(), result.node, "TemplateArgumentList"), nullptr);
    }
}

TEST(parse_CPP, type_cast_vs_call_disambiguation) {
    // 1. (int)(x) should be parsed as CastExpression
    {
        textparser::Parser parser;
        const char *code = "void f() { (int)(x); }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&cpp_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&cpp_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "CastExpression"), nullptr);
    }

    // 2. (uint32_t)(*ptr) should be parsed as CastExpression
    {
        textparser::Parser parser;
        const char *code = "void f() { (uint32_t)(*ptr); }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&cpp_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&cpp_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "CastExpression"), nullptr);
    }

    // 3. (my_func)(x) should remain a call expression, not a CastExpression
    {
        textparser::Parser parser;
        const char *code = "void f() { (my_func)(x); }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&cpp_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&cpp_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(find_node(parser.get(), result.node, "CastExpression"), nullptr);
    }
}

#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <cstring>

#include <javascript_definition.json.h>

TEST(parse_JavaScript, basic_js_program) {
    auto tokens = TextParser(R"(
import { helper } from './utils.js';
export class Calculator {
    async multiplyAsync(x, y) {
        // A simple async delay
        await helper.delay(100);
        const result = x * y;
        const $element = null;
        const displayValue = result ?? 0;
        const msg = `Output is: ${displayValue}\n`;
        const hasFinished = true;
        const hexVal = 0xff;
        return hasFinished;
    }
}
)", &javascript_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (tokens[i].type) {
            found.insert(tokens[i].type);
        }
    }

    EXPECT_TRUE(found.contains("ImportKeyword"));
    EXPECT_TRUE(found.contains("FromKeyword"));
    EXPECT_TRUE(found.contains("ExportKeyword"));
    EXPECT_TRUE(found.contains("ClassKeyword"));
    EXPECT_TRUE(found.contains("AsyncKeyword"));
    EXPECT_TRUE(found.contains("AwaitKeyword"));
    EXPECT_TRUE(found.contains("ConstKeyword"));
    EXPECT_TRUE(found.contains("ReturnKeyword"));
    EXPECT_TRUE(found.contains("TrueKeyword"));
    EXPECT_TRUE(found.contains("NullKeyword"));
    EXPECT_TRUE(found.contains("NumericLiteral"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("TemplateHead"));
    EXPECT_TRUE(found.contains("TemplateTail"));
    EXPECT_TRUE(found.contains("NullishCoalesce"));
    EXPECT_TRUE(found.contains("Identifier"));
}

TEST(parse_JavaScript, scientific_notation) {
    auto tokens = TextParser("var a = 1e-9; var b = 2e+5; var c = 1.5E-3;", &javascript_definition);

    bool found_neg = false;
    bool found_pos = false;
    bool found_upper = false;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "NumericLiteral") == 0) {
            if (item.value == "1e-9") found_neg = true;
            if (item.value == "2e+5") found_pos = true;
            if (item.value == "1.5E-3") found_upper = true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };
    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }
    EXPECT_TRUE(found_neg);
    EXPECT_TRUE(found_pos);
    EXPECT_TRUE(found_upper);
}

static const textparser_node *find_node(textparser_t handle, const textparser_node *node, const char *kind) {
    if (!node) return nullptr;
    const char *name = textparser_grammar_node_name(handle, node);
    if (name && std::strcmp(name, kind) == 0) return node;
    const char *type_str = textparser_get_token_type_str(textparser_get_language(handle), node);
    if (type_str && std::strcmp(type_str, kind) == 0) return node;
    for (auto *child = node->child; child; child = child->next) {
        if (auto *found = find_node(handle, child, kind)) return found;
    }
    return nullptr;
}

static int count_nodes(textparser_t handle, const textparser_node *node, const char *kind) {
    if (!node) return 0;
    int count = 0;
    const char *name = textparser_grammar_node_name(handle, node);
    if (name && std::strcmp(name, kind) == 0) count++;
    else {
        const char *type_str = textparser_get_token_type_str(textparser_get_language(handle), node);
        if (type_str && std::strcmp(type_str, kind) == 0) count++;
    }
    for (auto *child = node->child; child; child = child->next) {
        count += count_nodes(handle, child, kind);
    }
    return count;
}

TEST(parse_JavaScript, regex_vs_division_disambiguation) {
    // 1. Regex literal after assignment operator
    {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem("const re = /abc[0-9]+/gi;", (int)strlen("const re = /abc[0-9]+/gi;"), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&javascript_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&javascript_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "RegularExpressionLiteral"), nullptr);
    }

    // 2. Division expression (multiple slashes in operand context)
    {
        textparser::Parser parser;
        const char *code = "const result = a / b / c;";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&javascript_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&javascript_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(find_node(parser.get(), result.node, "RegularExpressionLiteral"), nullptr);
        EXPECT_GE(count_nodes(parser.get(), result.node, "Slash"), 2);
    }

    // 3. Regex literal after return keyword
    {
        textparser::Parser parser;
        const char *code = "function f() { return /hello\\/world/i; }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&javascript_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&javascript_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "RegularExpressionLiteral"), nullptr);
    }

    // 4. Division after parenthesized expression
    {
        textparser::Parser parser;
        const char *code = "const res = (x + y) / 2;";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&javascript_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&javascript_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(find_node(parser.get(), result.node, "RegularExpressionLiteral"), nullptr);
        EXPECT_NE(find_node(parser.get(), result.node, "Slash"), nullptr);
    }

    // 5. Regex after control statement condition (if)
    {
        textparser::Parser parser;
        const char *code = "if (flag) /foo/.test(str);";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&javascript_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&javascript_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "RegularExpressionLiteral"), nullptr);
    }

    // 6. Division after array index
    {
        textparser::Parser parser;
        const char *code = "const val = arr[0] / 4;";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&javascript_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&javascript_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(find_node(parser.get(), result.node, "RegularExpressionLiteral"), nullptr);
        EXPECT_NE(find_node(parser.get(), result.node, "Slash"), nullptr);
    }

    // 7. Regex inside array literal
    {
        textparser::Parser parser;
        const char *code = "const list = [/first/, /second/g];";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&javascript_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&javascript_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(count_nodes(parser.get(), result.node, "RegularExpressionLiteral"), 2);
    }
}

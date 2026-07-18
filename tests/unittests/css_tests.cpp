#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <css_definition.json.h>
#include <css.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_CSS, basic_css_program) {
    auto tokens = TextParser(R"(
/* Main Stylesheet */
@media (max-width: 768px) {
    body {
        margin: 0;
        padding: 10px 20px;
    }
}

#app-container .card > a:hover {
    background-color: #ffffff;
    font-family: 'Helvetica \'Neue\'', "Arial \"Sans\"", sans-serif;
    font: 12px/14px sans-serif;
    width: calc(100% - 2rem);
    display: flex !important;
}
)", &css_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("AtRule"));
    EXPECT_TRUE(found.contains("IdName"));
    EXPECT_TRUE(found.contains("ClassName"));
    EXPECT_TRUE(found.contains("PseudoClass"));
    EXPECT_TRUE(found.contains("TagName"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Declaration"));
    EXPECT_TRUE(found.contains("HexColor"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("FunctionCall"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Important"));
    EXPECT_TRUE(found.contains("Value"));
    EXPECT_TRUE(found.contains("DeclOperator"));
}

TEST(validate_CSS, properties_and_rules) {
    // 1. Correct CSS
    {
        textparser_t handle = nullptr;
        const char *code = "@media (max-width: 768px) { body { margin: 0; padding: 10px; --my-var: red; -webkit-transform: rotate(45deg); } } a:hover { color: blue; }";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &css_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_css(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }

    // 2. Unknown Property
    {
        textparser_t handle = nullptr;
        const char *code = "div { invalidpropertyname: 12px; }";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &css_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_css(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Unknown CSS property: [invalidpropertyname]");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 3. Unknown Pseudo-class
    {
        textparser_t handle = nullptr;
        const char *code = "a:invalidpseudoclass { color: red; }";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &css_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_css(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Unknown CSS pseudo-class: [:invalidpseudoclass]");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 4. Unknown At-Rule
    {
        textparser_t handle = nullptr;
        const char *code = "@invalidatrule { body { margin: 0; } }";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &css_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_css(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Unknown CSS At-Rule: [@invalidatrule]");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }
}


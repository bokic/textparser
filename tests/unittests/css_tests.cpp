#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
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
    EXPECT_TRUE(found.contains("AtKeyword"));
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("Hash"));
    EXPECT_TRUE(found.contains("ClassSelector"));
    EXPECT_TRUE(found.contains("Combinator"));
    EXPECT_TRUE(found.contains("Colon"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Slash"));
    EXPECT_TRUE(found.contains("Minus"));
    EXPECT_TRUE(found.contains("Important"));
    EXPECT_TRUE(found.contains("Comma"));
    EXPECT_TRUE(found.contains("SingleString_Start"));
    EXPECT_TRUE(found.contains("SingleString_End"));
    EXPECT_TRUE(found.contains("DoubleString_Start"));
    EXPECT_TRUE(found.contains("DoubleString_End"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("LParen"));
    EXPECT_TRUE(found.contains("RParen"));
    EXPECT_TRUE(found.contains("LBrace"));
    EXPECT_TRUE(found.contains("RBrace"));
    EXPECT_TRUE(found.contains("Semicolon"));
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

TEST(validate_CSS, corner_cases_and_edge_conditions) {
    // 1. Empty input
    {
        textparser_t handle = nullptr;
        const char *code = "";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &css_definition), 0);
        textparser_validation *val = textparser_validate_css(handle);
        EXPECT_EQ(val, nullptr);
        textparser_close(handle);
    }

    // 2. Only comments and whitespace
    {
        textparser_t handle = nullptr;
        const char *code = "   /* comment 1 */ \n\t /* comment 2 */  ";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &css_definition), 0);
        textparser_validation *val = textparser_validate_css(handle);
        EXPECT_EQ(val, nullptr);
        textparser_close(handle);
    }

    // 3. Vendor prefixed properties and custom properties
    {
        textparser_t handle = nullptr;
        const char *code = "div { -moz-box-sizing: border-box; -ms-flex: 1; -o-transform: none; --theme-color: #333; }";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &css_definition), 0);
        textparser_validation *val = textparser_validate_css(handle);
        EXPECT_EQ(val, nullptr);
        textparser_close(handle);
    }

    // 4. Valid pseudo-element with :: and vendor pseudo-classes
    {
        textparser_t handle = nullptr;
        const char *code = "p::before { content: 'x'; } input::-webkit-input-placeholder { color: gray; } button:-moz-focusring { outline: none; }";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &css_definition), 0);
        textparser_validation *val = textparser_validate_css(handle);
        EXPECT_EQ(val, nullptr);
        textparser_close(handle);
    }

    // 5. Unknown pseudo-element
    {
        textparser_t handle = nullptr;
        const char *code = "div::unknownpseudoelement { display: block; }";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &css_definition), 0);
        textparser_validation *val = textparser_validate_css(handle);
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(val->len, 1);
        EXPECT_EQ(val->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(val->items[0]->text, "Unknown CSS pseudo-element: [::unknownpseudoelement]");
        textparser_validation_clear(val);
        textparser_close(handle);
    }

    // 6. Missing final semicolon in declaration block
    {
        textparser_t handle = nullptr;
        const char *code = "span { color: red; font-size: 14px }";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &css_definition), 0);
        textparser_validation *val = textparser_validate_css(handle);
        EXPECT_EQ(val, nullptr);
        textparser_close(handle);
    }

    // 7. Unknown property with missing final semicolon
    {
        textparser_t handle = nullptr;
        const char *code = "span { badprop: 10px }";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        ASSERT_EQ(textparser_parse(handle, &css_definition), 0);
        textparser_validation *val = textparser_validate_css(handle);
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(val->len, 1);
        EXPECT_STREQ(val->items[0]->text, "Unknown CSS property: [badprop]");
        textparser_validation_clear(val);
        textparser_close(handle);
    }
}


#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <html_definition.json.h>
#include <html.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_HTML, basic_html_program) {
    auto tokens = TextParser(R"(
<!DOCTYPE html>
<!-- HTML Main Document -->
<html lang="en">
<head>
    <meta charset="utf-8" />
    <title>Hello HTML</title>
</head>
<body>
    <div class='container' id="app">
        <h1>Welcome to AntiGravity Code!</h1>
        <img src="logo.png" alt='AntiGravity Logo' />
    </div>
</body>
</html>
)", &html_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Doctype"));
    EXPECT_TRUE(found.contains("Comment"));
    EXPECT_TRUE(found.contains("Tag"));
    EXPECT_TRUE(found.contains("ClosingTag"));
    EXPECT_TRUE(found.contains("AttributeName"));
    EXPECT_TRUE(found.contains("Equal"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("SingleString"));
}

TEST(validate_HTML, tag_and_attributes) {
    // 1. Correct HTML
    {
        textparser_t handle = nullptr;
        const char *code = "<!DOCTYPE html><html lang=\"en\"><head><title>Test</title></head><body><div class=\"container\"><img src=\"a.jpg\" alt=\"test\" /></div></body></html>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }

    // 2. Unknown HTML tag
    {
        textparser_t handle = nullptr;
        const char *code_invalid_tag = "<nonexistenttag></nonexistenttag>";
        int res = textparser_openmem(code_invalid_tag, strlen(code_invalid_tag), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Unknown HTML tag: [nonexistenttag]");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 3. Mismatched tag nesting
    {
        textparser_t handle = nullptr;
        const char *code = "<div><span></div></span>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 2);
        EXPECT_STREQ(validation->items[0]->text, "HTML tag [span] requires a closing tag </span>");
        EXPECT_STREQ(validation->items[1]->text, "Ending tag </span> has no matching start tag");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 4. Invalid attribute
    {
        textparser_t handle = nullptr;
        const char *code = "<div classs=\"invalid_class\"></div>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_STREQ(validation->items[0]->text, "Unknown attribute [classs] for HTML tag <div>");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 5. Void element closing tag violation
    {
        textparser_t handle = nullptr;
        const char *code = "<img src=\"a.jpg\" alt=\"test\"></img>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_STREQ(validation->items[0]->text, "Ending tag </img> is forbidden for void element");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 6. Missing mandatory attributes
    {
        textparser_t handle = nullptr;
        const char *code = "<img>"; // missing src (error) and alt (warning)
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 2);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "HTML tag <img> is missing mandatory attribute [src]");
        EXPECT_EQ(validation->items[1]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING);
        EXPECT_STREQ(validation->items[1]->text, "HTML tag <img> is missing mandatory attribute [alt]");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }
}


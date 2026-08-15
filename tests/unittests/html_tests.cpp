#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
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

TEST(validate_HTML, many_attributes_no_stack_overflow) {
    // Regression test for Critical 1: alloca in attribute validation loop
    // caused stack memory to accumulate for every attribute, leading to
    // stack overflow on files with many attributes.
    // Generate HTML with 2000 attributes across 100 tags
    std::string html = "<html><body>";
    for (int t = 0; t < 100; t++) {
        html += "<div";
        for (int a = 0; a < 20; a++) {
            html += " data-attr-" + std::to_string(t) + "-" + std::to_string(a) + "=\"val\"";
        }
        html += "></div>";
    }
    html += "</body></html>";

    textparser_t handle = nullptr;
    int res = textparser_openmem(html.c_str(), html.size(), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(res, 0);
    res = textparser_parse(handle, &html_definition);
    ASSERT_EQ(res, 0);

    // Should not crash (the old code would stack overflow here)
    textparser_validation *validation = textparser_validate_html(handle);
    // We don't care about the validation results, just that it didn't crash
    if (validation != nullptr) {
        textparser_validation_clear(validation);
    }
    textparser_close(handle);
}

TEST(validate_HTML, framework_attributes_not_flagged) {
    // is_framework_attribute_token (heap/defer path) must accept all
    // framework-style attributes: Angular/Vue/React bindings, data-/aria-,
    // event handlers, namespaces and custom-element attributes.
    {
        textparser_t handle = nullptr;
        const char *code =
            "<div (click)=\"a\" [class]=\"b\" *ngIf=\"c\" @enter=\"d\" :href=\"e\" "
            "data-x=\"g\" aria-label=\"h\" onclick=\"i\" v-on:click=\"j\"></div>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }

    // Framework attribute token at the very start/end of the document
    // (boundary checks on position +/- 1) must not crash or misfire.
    {
        textparser_t handle = nullptr;
        const char *code = "<div :href=\"a\"></div>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }

    // Regular unknown attribute is still reported as an error.
    {
        textparser_t handle = nullptr;
        const char *code = "<div badattr=\"x\"></div>";
        int res = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &html_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_html(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Unknown attribute [badattr] for HTML tag <div>");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }
}

TEST(validate_HTML, oversized_attribute_name_skipped) {
    // Edge case: attribute name longer than 255 chars should be gracefully
    // skipped rather than causing a buffer overflow
    std::string long_attr(300, 'x');
    std::string html = "<div " + long_attr + "=\"val\"></div>";

    textparser_t handle = nullptr;
    int res = textparser_openmem(html.c_str(), html.size(), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(res, 0);
    res = textparser_parse(handle, &html_definition);
    ASSERT_EQ(res, 0);

    // Should not crash - the oversized attribute is simply skipped
    textparser_validation *validation = textparser_validate_html(handle);
    if (validation != nullptr) {
        textparser_validation_clear(validation);
    }
    textparser_close(handle);
}

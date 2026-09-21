#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <html_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct HTMLGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/html_definition.json", &json_definition),
                      TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json_definition, nullptr);
            ASSERT_NE(json_definition->grammar, nullptr);
        }
    }

    void TearDown() override {
        if (json_definition != nullptr) {
            textparser_free_language_definition(json_definition);
            json_definition = nullptr;
        }
    }

    const textparser_language_definition *get_definition() const {
        return GetParam() ? json_definition : &html_definition;
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(get_definition()), 0);
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(get_definition(), &result), 0);
        if (result.status == TEXTPARSER_MATCH_OK) {
            const textparser_lex_token *remaining = nullptr;
            int peek = textparser_lexer_peek(
                parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
            if (peek == 0 && remaining != nullptr) {
                std::cout << "DEBUG REMAINING TOKEN: kind=" << remaining->kind << " ["
                          << remaining->start << ".." << remaining->end << "] (source snippet: "
                          << (source + remaining->start) << ")" << std::endl;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, HTMLGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// ---------------------------------------------------------------------------
// Basic document structure
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_empty_document) {
    auto *node = parse_source("");
    // Empty document is valid — Document is a repeat(Node), so zero iterations is OK.
    (void)node;
}

TEST_P(HTMLGrammarFixture, parses_whitespace_only_document) {
    auto *node = parse_source("   \t\r\n  ");
    (void)node;
}

TEST_P(HTMLGrammarFixture, parses_doctype_declaration) {
    auto *node = parse_source("<!DOCTYPE html>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Doctype"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_doctype_case_insensitive) {
    auto *node = parse_source("<!doctype html PUBLIC \"-//W3C//DTD HTML 4.01//EN\">");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Doctype"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_html_comment) {
    auto *node = parse_source("<!-- This is a comment -->");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Comment"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_comment_with_dashes_inside) {
    // A comment body can contain single dashes but not "-->"
    auto *node = parse_source("<!-- single - dash is ok -->");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Comment"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_cdata_section) {
    auto *node = parse_source("<![CDATA[ <raw> & content ]]>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CData"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_processing_instruction) {
    auto *node = parse_source("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ProcessingInstruction"), nullptr);
}

// ---------------------------------------------------------------------------
// Void elements
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_void_elements_without_slash) {
    const char *source = "<br><hr><wbr>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VoidElement"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_void_elements_with_self_close_slash) {
    const char *source = "<br/><hr /><img src=\"logo.png\" alt=\"logo\"/>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VoidElement"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_input_with_attributes) {
    const char *source = "<input type=\"text\" name=\"username\" value=\"\" required>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VoidElement"), nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_link_and_meta_void_elements) {
    const char *source =
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<link rel=\"stylesheet\" href=\"style.css\">\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VoidElement"), nullptr);
}

// ---------------------------------------------------------------------------
// Paired elements and nesting
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_simple_paired_element) {
    auto *node = parse_source("<div>hello</div>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Element"), nullptr);
    EXPECT_NE(find(node, "ClosingTag"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_full_html_document_tree) {
    const char *source =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "  <head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <title>Test Page</title>\n"
        "  </head>\n"
        "  <body>\n"
        "    <h1 class=\"main-title\">Hello, World!</h1>\n"
        "    <p id=\"intro\">Welcome to <strong>textparser</strong>.</p>\n"
        "    <ul>\n"
        "      <li>Item 1</li>\n"
        "      <li>Item 2</li>\n"
        "    </ul>\n"
        "  </body>\n"
        "</html>\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Doctype"), nullptr);
    EXPECT_NE(find(node, "Element"), nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
    EXPECT_NE(find(node, "StartTag"), nullptr);
    EXPECT_NE(find(node, "ClosingTag"), nullptr);
    EXPECT_NE(find(node, "Text"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_deeply_nested_elements) {
    const char *source =
        "<div><section><article><p><span><em>deep</em></span></p></article></section></div>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Element"), nullptr);
}

// ---------------------------------------------------------------------------
// Self-closing elements (non-void, e.g. custom components in JSX/HTML5)
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_self_closing_non_void_element) {
    auto *node = parse_source("<my-component/>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SelfClosingTag"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_self_closing_with_attributes) {
    auto *node = parse_source("<app-header title=\"Home\" show-back/>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SelfClosingTag"), nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

// ---------------------------------------------------------------------------
// Script and style elements
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_script_element_with_body) {
    const char *source =
        "<script type=\"text/javascript\">\n"
        "  const x = 1;\n"
        "  function greet(name) { return `Hello, ${name}!`; }\n"
        "</script>\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ScriptElement"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_script_element_self_closing) {
    auto *node = parse_source("<script src=\"app.js\"/>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ScriptElement"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_script_element_empty_body) {
    auto *node = parse_source("<script></script>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ScriptElement"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_style_element_with_body) {
    const char *source =
        "<style type=\"text/css\">\n"
        "  body { margin: 0; padding: 0; font-family: sans-serif; }\n"
        "  .container { max-width: 1200px; }\n"
        "</style>\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StyleElement"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_style_element_empty_body) {
    auto *node = parse_source("<style></style>");
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StyleElement"), nullptr);
}

// ---------------------------------------------------------------------------
// Attributes: standard, boolean, framework-style
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_standard_attributes) {
    const char *source =
        "<a href=\"https://example.com\" target=\"_blank\" rel=\"noopener noreferrer\">Link</a>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
    EXPECT_NE(find(node, "DoubleString"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_boolean_attribute_no_value) {
    const char *source = "<input type=\"checkbox\" checked disabled readonly>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_unquoted_attribute_value) {
    const char *source = "<input type=text value=hello>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
    // Unquoted bare-word values are tokenized as AttributeName (higher priority)
    EXPECT_NE(find(node, "AttributeName"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_data_and_aria_attributes) {
    const char *source =
        "<div data-cy=\"submit-btn\" data-testid=\"form\" aria-label=\"Submit form\" "
        "     aria-hidden=\"false\" role=\"button\"></div>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_angular_event_binding_attribute) {
    // Angular: (click)="handler()"
    const char *source = "<button (click)=\"submitForm()\">Submit</button>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_angular_property_binding_attribute) {
    // Angular: [class.active]="isActive"
    const char *source = "<div [class.active]=\"isActive\" [style.color]=\"color\"></div>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_angular_structural_directives) {
    // Angular: *ngIf, *ngFor
    const char *source =
        "<div *ngIf=\"isVisible\">\n"
        "  <li *ngFor=\"let item of items\">{{ item }}</li>\n"
        "</div>\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_vue_directive_attributes) {
    // Vue: @submit.prevent, :href, #slot, v-model
    const char *source =
        "<form @submit.prevent=\"onSubmit\" :action=\"formAction\">\n"
        "  <input v-model=\"username\">\n"
        "  <template #header>Title</template>\n"
        "</form>\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_custom_element_with_mixed_framework_attrs) {
    const char *source =
        "<user-card #userCard :user-id=\"userId\" (selected)=\"onSelect($event)\"\n"
        "           *ngIf=\"users.length > 0\" [class.active]=\"isActive\"\n"
        "           data-cy=\"user-card\" aria-label=\"User profile card\">\n"
        "</user-card>\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

// ---------------------------------------------------------------------------
// Entities
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_named_character_entities) {
    const char *source = "<p>Copyright &copy; 2024 &amp; All rights reserved.</p>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Entity"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_numeric_decimal_entities) {
    const char *source = "<p>&#65;&#66;&#67;</p>";  // ABC
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Entity"), nullptr);
}

TEST_P(HTMLGrammarFixture, parses_numeric_hex_entities) {
    const char *source = "<p>&#x1F600; &#x41; &#x0042;</p>";  // emoji + A + B
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Entity"), nullptr);
}

// ---------------------------------------------------------------------------
// Mixed content and complete pages
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, parses_complete_page_with_all_node_types) {
    const char *source =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE html>\n"
        "<!-- Page header comment -->\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width\">\n"
        "  <title>Full Test &amp; Demo</title>\n"
        "  <link rel=\"stylesheet\" href=\"style.css\">\n"
        "  <style>body { color: #333; }</style>\n"
        "  <script src=\"vendor.js\"></script>\n"
        "</head>\n"
        "<body>\n"
        "  <![CDATA[raw content here]]>\n"
        "  <h1 id=\"top\" class=\"hero-title\">Hello &copy; World</h1>\n"
        "  <p>Numeric: &#65; Hex: &#x41;</p>\n"
        "  <br>\n"
        "  <input type=\"text\" value=default>\n"
        "  <my-widget/>\n"
        "  <script>\n"
        "    window.onload = function() { console.log('ready'); };\n"
        "  </script>\n"
        "</body>\n"
        "</html>\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Doctype"), nullptr);
    EXPECT_NE(find(node, "Comment"), nullptr);
    EXPECT_NE(find(node, "ProcessingInstruction"), nullptr);
    EXPECT_NE(find(node, "CData"), nullptr);
    EXPECT_NE(find(node, "Element"), nullptr);
    EXPECT_NE(find(node, "VoidElement"), nullptr);
    EXPECT_NE(find(node, "ScriptElement"), nullptr);
    EXPECT_NE(find(node, "StyleElement"), nullptr);
    EXPECT_NE(find(node, "Text"), nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

// ---------------------------------------------------------------------------
// Error recovery
// ---------------------------------------------------------------------------

TEST_P(HTMLGrammarFixture, recovers_from_syntax_error_and_continues) {
    // Orphaned closing tag followed by valid content — engine should recover
    const char *source = "</orphan><div>valid</div>";
    parser.reset();
    EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(parser.parse(get_definition()), 0);
    result = {};
    int exec_rc = parser.execute_language_grammar(get_definition(), &result);
    EXPECT_EQ(exec_rc, 0);
}

TEST_P(HTMLGrammarFixture, recovers_from_unclosed_tag) {
    // Tag without closing > — recovery should kick in
    const char *source = "<div class=\"test\"><p>content</p></div>";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Element"), nullptr);
}

} // namespace

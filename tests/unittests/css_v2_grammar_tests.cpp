#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <css_definition.json.h>
#include <cstring>

namespace {
struct CSSGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json = nullptr;
    textparser::Parser parser;
    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                "definitions/css_definition.json", &json), TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json, nullptr);
        }
    }
    void TearDown() override { if (json) textparser_free_language_definition(json); }
    const textparser_language_definition *definition() { return GetParam() ? json : &css_definition; }
    bool accepts(const char *source) {
        parser.reset();
        if (parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8) != 0)
            return false;
        if (parser.parse(definition()) != 0) return false;
        textparser_match_result result = {};
        if (parser.execute_language_grammar(definition(), &result) != 0 ||
            result.status != TEXTPARSER_MATCH_OK || textparser_get_diagnostic_count(parser.get()))
            return false;
        const textparser_lex_token *remaining = nullptr;
        return textparser_lexer_peek(parser.get(), 0,
            textparser_get_lexical_goal(parser.get()), &remaining) == 1 && !remaining;
    }
    const textparser_node *find(const textparser_node *node, const char *kind) {
        if (!node) return nullptr;
        const char *name = textparser_grammar_node_name(parser.get(), node);
        if (name && std::strcmp(name, kind) == 0) return node;
        for (auto *child = node->child; child; child = child->next)
            if (auto *found = find(child, kind)) return found;
        return nullptr;
    }
};
INSTANTIATE_TEST_SUITE_P(DefinitionSources, CSSGrammarFixture, testing::Bool(),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "JSON" : "Static"; });

TEST_P(CSSGrammarFixture, empty_comments_and_legacy_markers) {
    for (const char *s : {"", " \t\r\n\f", "/**/ /* * / **/", "<!-- body{} -->"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(CSSGrammarFixture, declarations_without_spacing_and_optional_final_semicolon) {
    for (const char *s : {"a{color:red}", "a{color:red;padding:0;}",
        "A { COLOR: RED; ; margin: +1px -.5em 1e2px 50%; }",
        "a{--x:;--theme: {color:red; nested:[1,2]}; color:var(--x, blue)}",
        "a{font:12px/1.5 sans-serif; display:flex !important}",
        "a{color:red ! /*comment*/ IMPORTANT}"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(CSSGrammarFixture, selectors_and_attributes) {
    for (const char *s : {"#abc.card>a:hover::before, * + p {color:#abcdef}",
        "a:is(.a, #b, :not([hidden])):nth-child(2n + 1) {margin:0}",
        "svg|a[xlink|href^='https' i][id][s=value s] {}",
        "*|a[|href][*|title=word] {}", "a[x=y I] {}", ".\\31 23, .caf\xc3\xa9 {color:red}"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(CSSGrammarFixture, nesting_and_at_rules) {
    for (const char *s : {"@charset \"UTF-8\"; @import url(style.css) screen;",
        "@media screen and (min-width:768px) { body {margin:0} }",
        "@supports (display:grid) { @media (width >= 10px) {a{display:grid}} }",
        "@font-face {font-family:'Demo';src:url(font.woff2) format('woff2')}",
        "@keyframes fade {from{opacity:0}50%,100%{opacity:1}}",
        "@layer reset, base; @layer base {a{color:red}}",
        "a{color:red;&:hover{color:blue} @media (width > 1px){color:green} b:hover{margin:0}}",
        "@container card (width > 30em) {.card {display:grid}}",
        "@page :first { margin:1cm; @top-left { content:'Title' } }",
        "@property --accent {syntax:'<color>'; inherits:false; initial-value:red;}"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(CSSGrammarFixture, strings_urls_and_balanced_functions) {
    for (const char *s : {"a{content:\"it's fine\";font-family:'a\"b'}",
        "a{content:'a\\'b';other:\"a\\\"b\"}",
        "a{content:'\\41 B';other:'line\\\r\ncontinued'}",
        "a{background:url(https://example.test/a.png?q=1#x)}",
        "a{background:url(\"a (b).png\")}",
        "a{width:calc(100% - 2 * (1rem + var(--gap, 0px))); color:rgb(0 0 0 / 50%)}",
        "a{/*c*/color/**/:/**/red/**/;content:'/*not comment*/'}"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(CSSGrammarFixture, malformed_input_is_not_a_clean_complete_parse) {
    for (const char *s : {"a{", "a{color:red", "a{color red}", "a{color:}",
        "a{color:red margin:0}", "a{content:'unterminated}", "a{content:'raw\nnewline'}",
        "a{width:calc(1px + 2px]}", "a[href=]{}", "a,{}", "a > {}", "/* unfinished",
        "@media screen", "} a{}", "a{} garbage", "a{color:red} @", "a{color:red !nope}", "a[x=y invalid]{}"}) {
        SCOPED_TRACE(s); EXPECT_FALSE(accepts(s));
    }
}
TEST_P(CSSGrammarFixture, builds_structured_ast) {
    const char *source = "@media (width:1px){a:hover[data-x='v']{width:calc(1px + 2px)}}";
    ASSERT_TRUE(accepts(source));
    // Execute again to inspect the public grammar result independently of lexer state.
    textparser_match_result result = {};
    ASSERT_EQ(parser.execute_language_grammar(definition(), &result), 0);
    for (const char *kind : {"StyleSheet", "AtRule", "QualifiedRule", "SelectorList",
         "PseudoSelector", "AttributeSelector", "RuleBlock", "Declaration", "FunctionCall"}) {
        SCOPED_TRACE(kind); EXPECT_TRUE(find(result.node, kind));
    }
}
TEST_P(CSSGrammarFixture, recovery_preserves_following_declaration) {
    const char *source = "a{ broken ???; margin:0; }";
    ASSERT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition()), 0);
    textparser_match_result result = {};
    ASSERT_EQ(parser.execute_language_grammar(definition(), &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(result.node, "Declaration"), nullptr);
    const textparser_lex_token *remaining = nullptr;
    EXPECT_EQ(textparser_lexer_peek(parser.get(), 0,
        textparser_get_lexical_goal(parser.get()), &remaining), 1);
    EXPECT_EQ(remaining, nullptr);
}
TEST_P(CSSGrammarFixture, restores_lexer_mode_after_nested_blocks_and_strings) {
    ASSERT_TRUE(accepts("@media (width < 10px){a{content:'}])';width:calc((1px))}} b{}"));
    EXPECT_STREQ(textparser_get_current_mode(parser.get()), "default");
}
} // namespace

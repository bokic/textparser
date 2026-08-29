#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <vector>
#include <cstring>

#include <scratch_definition.json.h>
#include <cfml_definition.json.h>
#include <bash_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Scratch, basic_scratch_program) {
    auto tokens = TextParser(R"(
# Scratch cat animation
when green flag clicked
set size to (100)
show
forever
    move (10) steps
    if touching (mouse-pointer) then
        say Hello for 2 seconds
    else
        move (-10) steps
    end
    wait 0.5 seconds
end

when this sprite clicked
broadcast message1
)", &scratch_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Parenthesis"));
    EXPECT_TRUE(found.contains("Variable"));
}

struct CallbackTestData {
    int token_count = 0;
    std::vector<std::string> token_names;
    std::vector<std::string> token_texts;
    textparser_t handle;
};

static void test_callback(textparser_t handle, textparser_token_item *item, enum textparser_callback_type callback_type, void *user_data) {
    (void)callback_type;
    if (!user_data) return;
    CallbackTestData *data = (CallbackTestData *)user_data;
    data->token_count++;

    const textparser_language_definition *lang = textparser_get_language(handle);
    const char *type_str = textparser_get_token_type_str(lang, item);
    if (type_str) {
        data->token_names.push_back(type_str);
    }

    char *text = textparser_get_token_text(handle, item);
    if (text) {
        data->token_texts.push_back(text);
        free(text);
    }
}

TEST(parse_Callback, basic_callback) {
    textparser_t handle = nullptr;
    const char *text = "10 + 20";
    int err = textparser_openmem(text, strlen(text), scratch_definition.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);

    CallbackTestData data;
    data.handle = handle;
    textparser_set_callback(handle, test_callback, &data);

    err = textparser_parse(handle, &scratch_definition);
    EXPECT_EQ(err, 0);

    EXPECT_EQ(data.token_count, 3);
    ASSERT_EQ(data.token_names.size(), 3);
    EXPECT_EQ(data.token_names[0], "Number");
    EXPECT_EQ(data.token_names[1], "Operator");
    EXPECT_EQ(data.token_names[2], "Number");

    ASSERT_EQ(data.token_texts.size(), 3);
    EXPECT_EQ(data.token_texts[0], "10");
    EXPECT_EQ(data.token_texts[1], "+");
    EXPECT_EQ(data.token_texts[2], "20");

    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(textparser_get_token_text_color(first), scratch_definition.tokens[first->token_id].text_color);

    textparser_close(handle);
}

TEST(parse_Callback, get_token_text_overflow_protection) {
    textparser_t handle = nullptr;
    const char *text = "10";
    int err = textparser_openmem(text, strlen(text), scratch_definition.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);

    textparser_token_item item;
    memset(&item, 0, sizeof(item));
    item.len = SIZE_MAX;

    char *txt = textparser_get_token_text(handle, &item);
    EXPECT_EQ(txt, nullptr);

    uint16_t *txt16 = textparser_get_token_text16(handle, &item);
    EXPECT_EQ(txt16, nullptr);

    uint32_t *txt32 = textparser_get_token_text32(handle, &item);
    EXPECT_EQ(txt32, nullptr);

    textparser_close(handle);
}

static void verify_prev_links_recursive(const textparser_token_item *token) {
    if (!token) return;

    const textparser_token_item *current = token;
    const textparser_token_item *prev = nullptr;
    while (current) {
        EXPECT_EQ(textparser_get_token_prev(current), prev);
        const textparser_token_item *child = textparser_get_token_child(current);
        if (child) {
            verify_prev_links_recursive(child);
        }
        prev = current;
        current = textparser_get_token_next(current);
    }
}

TEST(parse_Callback, verify_prev_pointers) {
    textparser_t handle = nullptr;
    const char *text = R"(
# Scratch cat animation
when green flag clicked
set size to (100)
show
forever
    move (10) steps
    if touching (mouse-pointer) then
        say Hello for 2 seconds
    else
        move (-10) steps
    end
    wait 0.5 seconds
end

when this sprite clicked
broadcast message1
)";
    int err = textparser_openmem(text, strlen(text), scratch_definition.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);

    err = textparser_parse(handle, &scratch_definition);
    EXPECT_EQ(err, 0);

    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);

    verify_prev_links_recursive(first);

    textparser_close(handle);
}

TEST(parse_MultiLine, token_multiline_validation) {
    // SingleLineToken non-multiline token fails on newline
    textparser_token tokens[2] = {};
    tokens[0].name = "SingleLineToken";
    tokens[0].type = TEXTPARSER_TOKEN_TYPE_START_STOP;
    tokens[0].start_regex = "'";
    tokens[0].end_regex = "'";
    tokens[0].other_text_inside = true;
    tokens[0].multi_line = false;

    static const int start_tokens[] = { 0, TextParser_END };
    textparser_language_definition lang = {};
    lang.name = "test_multiline";
    lang.version = 1.0;
    lang.case_sensitivity = true;
    lang.default_text_encoding = TEXTPARSER_ENCODING_UTF_8;
    lang.starts_with = (int *)start_tokens;
    lang.tokens = tokens;

    const char *invalid_text = "'first line\nsecond line'";
    textparser_t handle = nullptr;
    int err = textparser_openmem(invalid_text, strlen(invalid_text), lang.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);

    err = textparser_parse(handle, &lang);
    EXPECT_NE(err, 0);
    EXPECT_STREQ(textparser_parse_error(handle), "Token spans multiple lines but multi_line flag is not set!");
    EXPECT_EQ(textparser_parse_error_position(handle), 0);
    textparser_close(handle);

    // MultiLineToken allowed on newline
    tokens[0].multi_line = true;
    err = textparser_openmem(invalid_text, strlen(invalid_text), lang.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);

    err = textparser_parse(handle, &lang);
    EXPECT_EQ(err, 0);
    textparser_close(handle);
}

// ---------------------------------------------------------------------------
// Same-line search scope for multi_line=false tokens (all encodings).
// The engine bounds anchored start-token searches to the end of the current
// line, so single-line tokens must parse identically across encodings and
// still report a "Token spans multiple lines" error when they cross a newline.
// ---------------------------------------------------------------------------

static textparser_language_definition make_single_line_string_lang(void) {
    static textparser_token tokens[2] = {};
    static const int start_tokens[] = { 0, TextParser_END };
    tokens[0].name = "SingleLineString";
    tokens[0].type = TEXTPARSER_TOKEN_TYPE_START_STOP;
    tokens[0].start_regex = "'";
    tokens[0].end_regex = "'";
    tokens[0].other_text_inside = true;
    tokens[0].multi_line = false;
    static textparser_language_definition lang = {};
    lang.name = "test_single_line";
    lang.version = 1.0;
    lang.case_sensitivity = true;
    lang.other_text_inside = true;
    lang.default_text_encoding = TEXTPARSER_ENCODING_UTF_8;
    lang.starts_with = (int *)start_tokens;
    lang.tokens = tokens;
    return lang;
}

static std::vector<char> to_utf16le(const std::string &s) {
    std::vector<char> out;
    for (unsigned char c : s) {
        out.push_back((char)c);
        out.push_back(0);
    }
    return out;
}

static std::vector<char> to_utf32le(const std::string &s) {
    std::vector<char> out;
    for (unsigned char c : s) {
        out.push_back((char)c);
        out.push_back(0);
        out.push_back(0);
        out.push_back(0);
    }
    return out;
}

static textparser_language_definition single_line_lang = make_single_line_string_lang();

static void expect_first_token_len(const char *text, enum textparser_encoding enc, size_t expected_len) {
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), enc, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, 0);
    EXPECT_EQ(first->len, expected_len);
    textparser_close(handle);
}

static void expect_first_token_len(const std::vector<char> &buf, enum textparser_encoding enc, size_t expected_len) {
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(buf.data(), (int)buf.size(), enc, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, 0);
    EXPECT_EQ(first->len, expected_len);
    textparser_close(handle);
}

TEST(parse_SameLine, closes_on_same_line_utf8) {
    expect_first_token_len("'hello'", TEXTPARSER_ENCODING_UTF_8, 7);
}

TEST(parse_SameLine, closes_on_same_line_utf16) {
    expect_first_token_len(to_utf16le("'hello'"), TEXTPARSER_ENCODING_UTF_16, 7);
}

TEST(parse_SameLine, closes_on_same_line_utf32) {
    expect_first_token_len(to_utf32le("'hello'"), TEXTPARSER_ENCODING_UTF_32, 7);
}

TEST(parse_SameLine, next_line_becomes_unprocessed_utf8) {
    const char *text = "'hello'\nworld";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, 0);
    EXPECT_EQ(first->len, 7);
    const textparser_token_item *ws = textparser_get_token_next(first);
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->token_id, TEXTPARSER_TOKEN_ID_WHITESPACE);
    EXPECT_EQ(ws->len, 1);
    const textparser_token_item *next = textparser_get_token_next(ws);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->token_id, TEXTPARSER_TOKEN_ID_UNPROCESSED);
    EXPECT_EQ(next->len, 5);
    textparser_close(handle);
}

TEST(parse_SameLine, next_line_becomes_unprocessed_utf16) {
    textparser_t handle = nullptr;
    auto buf = to_utf16le("'hello'\nworld");
    ASSERT_EQ(textparser_openmem(buf.data(), (int)buf.size(), TEXTPARSER_ENCODING_UTF_16, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, 0);
    EXPECT_EQ(first->len, 7);
    const textparser_token_item *ws = textparser_get_token_next(first);
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->token_id, TEXTPARSER_TOKEN_ID_WHITESPACE);
    EXPECT_EQ(ws->len, 1);
    const textparser_token_item *next = textparser_get_token_next(ws);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->token_id, TEXTPARSER_TOKEN_ID_UNPROCESSED);
    EXPECT_EQ(next->len, 5);
    textparser_close(handle);
}

TEST(parse_SameLine, crlf_line_ending_utf8) {
    const char *text = "'hello'\r\nworld";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, 0);
    EXPECT_EQ(first->len, 7);
    const textparser_token_item *ws = textparser_get_token_next(first);
    ASSERT_NE(ws, nullptr);
    EXPECT_EQ(ws->token_id, TEXTPARSER_TOKEN_ID_WHITESPACE);
    EXPECT_EQ(ws->len, 2);
    const textparser_token_item *next = textparser_get_token_next(ws);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->token_id, TEXTPARSER_TOKEN_ID_UNPROCESSED);
    EXPECT_EQ(next->len, 5);
    textparser_close(handle);
}

TEST(parse_SameLine, end_of_file_no_trailing_newline_utf8) {
    const char *text = "'hello'world";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, 0);
    EXPECT_EQ(first->len, 7);
    const textparser_token_item *next = textparser_get_token_next(first);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->token_id, TEXTPARSER_TOKEN_ID_UNPROCESSED);
    EXPECT_EQ(next->len, 5);
    textparser_close(handle);
}

static void expect_multiline_error(const std::vector<char> &buf, enum textparser_encoding enc) {
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(buf.data(), (int)buf.size(), enc, &handle), 0);
    int err = textparser_parse(handle, &single_line_lang);
    EXPECT_NE(err, 0);
    EXPECT_STREQ(textparser_parse_error(handle), "Token spans multiple lines but multi_line flag is not set!");
    EXPECT_EQ(textparser_parse_error_position(handle), 0);
    textparser_close(handle);
}

TEST(parse_SameLine, spanning_newline_still_errors_utf8) {
    const char text[] = "'hello\nworld'";
    expect_multiline_error(std::vector<char>(text, text + strlen(text)), TEXTPARSER_ENCODING_UTF_8);
}

TEST(parse_SameLine, spanning_newline_still_errors_utf16) {
    expect_multiline_error(to_utf16le("'hello\nworld'"), TEXTPARSER_ENCODING_UTF_16);
}

TEST(parse_SameLine, spanning_newline_still_errors_utf32) {
    expect_multiline_error(to_utf32le("'hello\nworld'"), TEXTPARSER_ENCODING_UTF_32);
}

TEST(parse_SameLine, closing_quote_on_next_line_errors_utf8) {
    textparser_t handle = nullptr;
    const char *text = "'abc\n'";
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    int err = textparser_parse(handle, &single_line_lang);
    EXPECT_NE(err, 0);
    EXPECT_STREQ(textparser_parse_error(handle), "Token spans multiple lines but multi_line flag is not set!");
    textparser_close(handle);
}

TEST(parse_SameLine, unclosed_on_same_line_errors_cant_find_end) {
    textparser_t handle = nullptr;
    const char *text = "'abc";
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    int err = textparser_parse(handle, &single_line_lang);
    EXPECT_NE(err, 0);
    EXPECT_STREQ(textparser_parse_error(handle), "Can't find end of the token!");
    EXPECT_EQ(textparser_parse_error_position(handle), 1);
    textparser_close(handle);
}

TEST(parse_SameLine, unclosed_spanning_newline_errors_cant_find_end) {
    // Closing quote missing entirely: even though a newline is spanned, the
    // fallback must report "Can't find end of the token!" (not a span error)
    // because there is no end token anywhere.
    textparser_t handle = nullptr;
    const char *text = "'abc\ndef";
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    int err = textparser_parse(handle, &single_line_lang);
    EXPECT_NE(err, 0);
    EXPECT_STREQ(textparser_parse_error(handle), "Can't find end of the token!");
    textparser_close(handle);
}

TEST(parse_SameLine, valid_utf8_multibyte_content) {
    // UTF-8 multibyte content inside a single-line string must parse correctly
    // on the fast (PCRE2_NO_UTF_CHECK) path: "caf\xC3\xA9" = c,a,f,é(2B) = 5 bytes.
    const char *text = "'caf\xC3\xA9'";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, 0);
    EXPECT_EQ(first->len, 7);
    textparser_close(handle);
}

TEST(parse_SameLine, invalid_utf8_falls_back_to_checked_path) {
    // Invalid UTF-8 (lone 0xFF, truncated 0xE2 0x82) must take the checked
    // (non-NO_UTF_CHECK) path and keep the pre-optimization behavior: pcre2
    // rejects the invalid subject, so no string token matches and the whole
    // buffer becomes unprocessed text (no crash, err == 0).
    const char buf[] = { '\'', (char)0xFF, '\'', (char)0xE2, (char)0x82 };
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(buf, (int)sizeof(buf), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &single_line_lang), 0);
    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->token_id, TEXTPARSER_TOKEN_ID_UNPROCESSED);
    EXPECT_EQ(first->len, 6);
    EXPECT_EQ(textparser_get_token_next(first), nullptr);
    textparser_close(handle);
}

TEST(parse_MustHaveOneChild, token_must_have_one_child_validation) {
    // Child token definition
    textparser_token tokens[3] = {};
    tokens[0].name = "ChildToken";
    tokens[0].type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN;
    tokens[0].start_regex = "abc";

    // Parent group token requiring exactly one child
    tokens[1].name = "ParentGroup";
    tokens[1].type = TEXTPARSER_TOKEN_TYPE_START_STOP;
    tokens[1].start_regex = "\\(";
    tokens[1].end_regex = "\\)";
    tokens[1].other_text_inside = true;
    tokens[1].must_have_one_child = true;
    tokens[1].multi_line = true;
    static const int parent_nested[] = { 0, TextParser_END };
    tokens[1].nested_tokens = (int *)parent_nested;

    static const int start_tokens[] = { 1, TextParser_END };
    textparser_language_definition lang = {};
    lang.name = "test_must_have_one_child";
    lang.version = 1.0;
    lang.case_sensitivity = true;
    lang.default_text_encoding = TEXTPARSER_ENCODING_UTF_8;
    lang.starts_with = (int *)start_tokens;
    lang.tokens = tokens;

    // 0 children should fail
    const char *no_child_text = "()";
    textparser_t handle = nullptr;
    int err = textparser_openmem(no_child_text, strlen(no_child_text), lang.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &lang);
    EXPECT_NE(err, 0);
    EXPECT_STREQ(textparser_parse_error(handle), "Token must have exactly one child token!");
    EXPECT_EQ(textparser_parse_error_position(handle), 0);
    textparser_close(handle);

    // 2 children should fail
    const char *two_children_text = "(abcabc)";
    err = textparser_openmem(two_children_text, strlen(two_children_text), lang.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &lang);
    EXPECT_NE(err, 0);
    EXPECT_STREQ(textparser_parse_error(handle), "Token must have exactly one child token!");
    EXPECT_EQ(textparser_parse_error_position(handle), 0);
    textparser_close(handle);

    // Exactly 1 child should succeed
    const char *one_child_text = "(abc)";
    err = textparser_openmem(one_child_text, strlen(one_child_text), lang.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &lang);
    EXPECT_EQ(err, 0);
    textparser_close(handle);
}

TEST(parse_DeleteIfOnlyOneChild, token_delete_if_only_one_child_validation) {
    textparser_token tokens[3] = {};
    tokens[0].name = "ChildToken";
    tokens[0].type = TEXTPARSER_TOKEN_TYPE_SIMPLE_TOKEN;
    tokens[0].start_regex = "abc";

    tokens[1].name = "ParentGroup";
    tokens[1].type = TEXTPARSER_TOKEN_TYPE_START_STOP;
    tokens[1].start_regex = "\\(";
    tokens[1].end_regex = "\\)";
    tokens[1].other_text_inside = true;
    tokens[1].delete_if_only_one_child = true;
    tokens[1].multi_line = true;
    static const int parent_nested[] = { 0, TextParser_END };
    tokens[1].nested_tokens = (int *)parent_nested;

    static const int start_tokens[] = { 1, TextParser_END };
    textparser_language_definition lang = {};
    lang.name = "test_delete_if_only_one_child";
    lang.version = 1.0;
    lang.case_sensitivity = true;
    lang.default_text_encoding = TEXTPARSER_ENCODING_UTF_8;
    lang.starts_with = (int *)start_tokens;
    lang.tokens = tokens;

    // Single child: ParentGroup token deletes itself and child is returned as top-level token
    const char *one_child_text = "(abc)";
    textparser_t handle = nullptr;
    int err = textparser_openmem(one_child_text, strlen(one_child_text), lang.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &lang);
    EXPECT_EQ(err, 0);

    textparser_token_item *root = textparser_get_first_token(handle);
    textparser_post_process(&root, &lang);

    const textparser_token_item *first = root;
    while (first && first->token_id < 0) first = first->next;
    ASSERT_NE(first, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&lang, first), "ChildToken");
    EXPECT_EQ(first->parent, nullptr);
    textparser_close(handle);

    // Multiple children: ParentGroup token is kept
    const char *two_children_text = "(abcabc)";
    err = textparser_openmem(two_children_text, strlen(two_children_text), lang.default_text_encoding, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &lang);
    EXPECT_EQ(err, 0);

    first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&lang, first), "ParentGroup");
    size_t semantic_children = 0;
    for (const textparser_token_item *c = first->child; c; c = c->next) {
        if (c->token_id >= 0) semantic_children++;
    }
    EXPECT_EQ(semantic_children, 2);
    textparser_close(handle);
}

TEST(parse_Consistency, defensive_checks) {
    // 1. Check null handle in textparser_parse_error and textparser_parse_error_position
    EXPECT_EQ(textparser_parse_error(nullptr), nullptr);
    EXPECT_EQ(textparser_parse_error_position(nullptr), 0);

    // 2. Check null token in textparser_get_token_children_count
    EXPECT_EQ(textparser_get_token_children_count(nullptr), 0);

    // 3. Check invalid encoding validation in textparser_openmem
    textparser_t handle = nullptr;
    const char *text = "hello";
    EXPECT_NE(textparser_openmem(text, strlen(text), 9999, &handle), 0);

    // 4. Check textparser_free_token_text wrapper
    int err = textparser_openmem(text, strlen(text), TEXTPARSER_ENCODING_UTF_8, &handle);
    ASSERT_EQ(err, 0);
    textparser_token_item item = {};
    item.len = 5;
    char *buf = textparser_get_token_text(handle, &item);
    ASSERT_NE(buf, nullptr);
    EXPECT_STREQ(buf, "hello");
    textparser_free_token_text(buf);
    textparser_free_token_text(nullptr); // safely handles null
    textparser_close(handle);
}

TEST(parse_CFML_Precedence, verified_coldfusion_spec_cases) {
    // Rank 12: AssignOperator (=, +=, -=, *=, /=, %=, &=)
    {
        auto tokens = TextParser(R"(<cfset a = b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "AssignOperator"));
    }

    // Rank 11: LogicalImpOperator (IMP)
    {
        auto tokens = TextParser(R"(<cfset res = a imp b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "LogicalImpOperator"));
    }

    // Rank 10: LogicalEqvOperator (EQV)
    {
        auto tokens = TextParser(R"(<cfset res = a eqv b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "LogicalEqvOperator"));
    }

    // Rank 9: LogicalXorOperator (XOR)
    {
        auto tokens = TextParser(R"(<cfset res = a xor b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "LogicalXorOperator"));
    }

    // Rank 8: LogicalOrOperator (OR, ||)
    {
        auto tokens = TextParser(R"(<cfset res = a or b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "LogicalOrOperator"));
    }

    // Rank 7: LogicalAndOperator (AND, &&)
    {
        auto tokens = TextParser(R"(<cfset res = a and b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "LogicalAndOperator"));
    }

    // Rank 6: LogicalNotOperator (NOT, !)
    {
        auto tokens = TextParser(R"(<cfset res = not a />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "LogicalNotOperator"));
    }

    // Rank 5: CompareOperator (EQ, NEQ, LT, LTE, GT, GTE, CONTAINS, DOES NOT CONTAIN, IS, IS NOT, ==, !=, <, <=, >, >=)
    {
        auto tokens = TextParser(R"(<cfset res = a eq b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "CompareOperator"));
    }

    // Rank 4: ConcatOperator (&)
    {
        auto tokens = TextParser(R"(<cfset res = a & b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "ConcatOperator"));
    }

    // Rank 3: Additive (AddOperator: +, -)
    {
        auto tokens = TextParser(R"(<cfset res = a + b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "AddOperator"));
    }

    // Rank 2: Multiplicative (MulOperator: *, /, \, %, MOD)
    {
        auto tokens = TextParser(R"(<cfset res = a mod b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "MulOperator"));
    }

    // Rank 13: TernaryOperator (?, :, ?:)
    {
        auto tokens = TextParser(R"(<cfset res = a ? b : c />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "TernaryOperator"));
    }

    // Rank 1: PowerOperator (^)
    {
        auto tokens = TextParser(R"(<cfset res = a ^ b />)", &cfml_definition);
        ASSERT_EQ(tokens.count, 1);
        EXPECT_TRUE(has_token_type(tokens, "PowerOperator"));
    }
}

TEST(parse_UTF8, truncated_utf8_sequences) {
    // Truncated 2-byte, 3-byte, and 4-byte sequence headers at string boundary
    const char *truncated_inputs[] = {
        "\xC2",             // Truncated 2-byte sequence
        "\xE0\xA0",         // Truncated 3-byte sequence (1 continuation byte)
        "\xF0\x90\x80",     // Truncated 4-byte sequence (2 continuation bytes)
        "test\xC2",         // Valid ASCII followed by truncated 2-byte sequence
        "hello\xE0\xA0"     // Valid ASCII followed by truncated 3-byte sequence
    };

    for (const char *input : truncated_inputs) {
        // Attempt parsing UTF-16/32 encoding conversions via adv_regex_find_pattern_ctx
        TextParser parser(input, &cfml_definition);
        // Ensure execution completes safely without reading out-of-bounds or crashing
    }
}

TEST(get_token_text, utf16_and_utf32_to_utf8_transcoding) {
    // 1. UTF-16: Greek "αβγ" (U+03B1 U+03B2 U+03B3) + Emoji "😀" (U+1F600 = D83D DE00)
    // UTF-8 bytes for "αβγ😀": \xCE\xB1 \xCE\xB2 \xCE\xB3 \xF0\x9F\x98\x80
    uint16_t u16_data[] = { 0x03B1, 0x03B2, 0x03B3, 0xD83D, 0xDE00, 0 };
    textparser_t handle16 = nullptr;
    int err16 = textparser_openmem((const char *)u16_data, 5 * sizeof(uint16_t), TEXTPARSER_ENCODING_UTF_16, &handle16);
    ASSERT_EQ(err16, 0);

    textparser_token_item item16;
    memset(&item16, 0, sizeof(item16));
    item16.len = 5; // 5 UTF-16 units (3 bmp + 2 surrogate)

    char *txt16 = textparser_get_token_text(handle16, &item16);
    ASSERT_NE(txt16, nullptr);
    EXPECT_STREQ(txt16, "αβγ😀");
    textparser_free_token_text(txt16);
    textparser_close(handle16);

    // 2. UTF-32: Greek "αβγ" + Emoji "😀"
    uint32_t u32_data[] = { 0x03B1, 0x03B2, 0x03B3, 0x1F600, 0 };
    textparser_t handle32 = nullptr;
    int err32 = textparser_openmem((const char *)u32_data, 4 * sizeof(uint32_t), TEXTPARSER_ENCODING_UTF_32, &handle32);
    ASSERT_EQ(err32, 0);

    textparser_token_item item32;
    memset(&item32, 0, sizeof(item32));
    item32.len = 4; // 4 UTF-32 code points

    char *txt32 = textparser_get_token_text(handle32, &item32);
    ASSERT_NE(txt32, nullptr);
    EXPECT_STREQ(txt32, "αβγ😀");
    textparser_free_token_text(txt32);
    textparser_close(handle32);
}

TEST(textparser_error, strerror_messages) {
    EXPECT_STREQ(textparser_strerror(TEXTPARSER_OK), "Success");
    EXPECT_STREQ(textparser_strerror(TEXTPARSER_ERROR_FILE_OPEN), "Failed to open or map file");
    EXPECT_STREQ(textparser_strerror(TEXTPARSER_ERROR_FILE_TOO_LARGE), "File exceeds maximum parse size (16 MB)");
    EXPECT_STREQ(textparser_strerror(TEXTPARSER_ERROR_UNSUPPORTED_BOM), "Unsupported byte order mark (BOM)");
    EXPECT_STREQ(textparser_strerror(TEXTPARSER_ERROR_INVALID_UTF16_SIZE), "Invalid UTF-16 size (not a multiple of 2 bytes)");
    EXPECT_STREQ(textparser_strerror(TEXTPARSER_ERROR_INVALID_UTF32_SIZE), "Invalid UTF-32 size (not a multiple of 4 bytes)");
    EXPECT_STREQ(textparser_strerror(9999), "Unknown error");
}

static size_t verify_cst_subtree(textparser_t handle, const textparser_token_item *node, size_t expected_start_pos) {
    size_t total_len = 0;
    const textparser_token_item *curr = node;
    while (curr != nullptr) {
        size_t computed_pos = textparser_get_token_position(curr);
        EXPECT_EQ(computed_pos, expected_start_pos + total_len);

        if (curr->child != nullptr) {
            size_t child_len_sum = verify_cst_subtree(handle, curr->child, computed_pos);
            EXPECT_EQ(child_len_sum, curr->len);
        }

        total_len += curr->len;
        curr = curr->next;
    }
    return total_len;
}

TEST(parse_CST, gapless_tree_and_dynamic_offsets) {
    const char *code = R"(
        <cfoutput>
            <!--- Comment --->
            <cfset x = 10 + 20 />
            <cfset str = "Hello " & name />
            #DateFormat(Now(), "yyyy-mm-dd")#
        </cfoutput>
    )";

    textparser_t handle = nullptr;
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &cfml_definition);
    EXPECT_EQ(err, 0);

    const textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);

    size_t total_len = verify_cst_subtree(handle, first, 0);
    EXPECT_EQ(total_len, strlen(code));

    textparser_close(handle);
}

TEST(parse_Delimiters, container_start_end_delimiters) {
    const char *bash_code = R"(${BUILD_TYPE:-Release})";
    textparser_t handle = nullptr;
    int err = textparser_openmem(bash_code, strlen(bash_code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &bash_definition);
    EXPECT_EQ(err, 0);

    const textparser_token_item *root = textparser_get_first_token(handle);
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_get_token_type_str(&bash_definition, root), "ParameterExpansion");

    const textparser_token_item *c0 = root->child;
    ASSERT_NE(c0, nullptr);
    EXPECT_EQ(c0->token_id, TEXTPARSER_TOKEN_ID_START_DELIMITER);
    EXPECT_STREQ(textparser_get_token_type_str(&bash_definition, c0), "StartDelimiter");
    EXPECT_EQ(c0->len, 2u); // "${"

    const textparser_token_item *last = c0;
    while (last->next) last = last->next;
    EXPECT_EQ(last->token_id, TEXTPARSER_TOKEN_ID_END_DELIMITER);
    EXPECT_STREQ(textparser_get_token_type_str(&bash_definition, last), "EndDelimiter");
    EXPECT_EQ(last->len, 1u); // "}"

    textparser_close(handle);
}



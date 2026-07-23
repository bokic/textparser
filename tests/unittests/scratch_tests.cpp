#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <scratch_definition.json.h>

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
    item.position = 0;
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

    const textparser_token_item *first = textparser_get_first_token(handle);
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
    EXPECT_EQ(textparser_get_token_children_count(first), 2);
    textparser_close(handle);
}


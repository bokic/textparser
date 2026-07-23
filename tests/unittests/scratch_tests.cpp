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

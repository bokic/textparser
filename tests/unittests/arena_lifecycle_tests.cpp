#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>

struct TestUserData {
    int value;
    bool *freed_flag;
};

static void free_test_user_data(void *ptr) {
    auto *ud = static_cast<TestUserData *>(ptr);
    if (ud && ud->freed_flag) {
        *ud->freed_flag = true;
    }
    delete ud;
}

static textparser_action sample_commit_handler(
    textparser_t parser,
    const textparser_event *event,
    void *user_data)
{
    (void)parser;
    (void)user_data;
    if (event && event->node) {
        textparser_node_set_flags(event->node, TEXTPARSER_NODE_SYNTHETIC);
        return TEXTPARSER_ACTION_ACCEPT;
    }
    return TEXTPARSER_ACTION_REJECT;
}

TEST(arena_lifecycle, node_properties_and_user_data) {
    textparser_t handle = nullptr;
    const char *json_text = R"({"name": "test", "num": 123})";
    int err = textparser_openmem(json_text, strlen(json_text), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    err = textparser_parse(handle, &json_definition);
    ASSERT_EQ(err, 0);

    textparser_node *root = textparser_get_first_token(handle);
    ASSERT_NE(root, nullptr);

    // Verify stable 64-bit ID
    uint64_t root_id = textparser_node_get_id(root);
    EXPECT_GT(root_id, 0u);

    // Verify node flags
    EXPECT_EQ(textparser_node_get_flags(root), TEXTPARSER_NODE_NONE);
    textparser_node_set_flags(root, TEXTPARSER_NODE_SYNTHETIC | TEXTPARSER_NODE_RECOVERED);
    EXPECT_EQ(textparser_node_get_flags(root), (TEXTPARSER_NODE_SYNTHETIC | TEXTPARSER_NODE_RECOVERED));

    // Verify decoded value
    EXPECT_EQ(textparser_node_get_decoded_value(root), nullptr);
    textparser_node_set_decoded_value(root, "decoded_root");
    EXPECT_STREQ(textparser_node_get_decoded_value(root), "decoded_root");

    // Verify user_data attachment and destruction
    bool freed = false;
    auto *ud = new TestUserData{42, &freed};
    textparser_node_set_user_data(root, ud, free_test_user_data);

    EXPECT_EQ(static_cast<TestUserData *>(textparser_node_get_user_data(root))->value, 42);

    // Replaced user_data triggers free_fn
    bool freed_second = false;
    auto *ud2 = new TestUserData{99, &freed_second};
    textparser_node_set_user_data(root, ud2, free_test_user_data);
    EXPECT_TRUE(freed);
    EXPECT_FALSE(freed_second);

    // Freeing explicitly
    textparser_node_set_user_data(root, nullptr, nullptr);
    EXPECT_TRUE(freed_second);

    textparser_close(handle);
}

TEST(arena_lifecycle, handler_registration_and_event_dispatch) {
    textparser_t handle = nullptr;
    const char *text = R"([1, 2, 3])";
    int err = textparser_openmem(text, strlen(text), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    int reg_err = textparser_register_handler(handle, "test.onCommit", sample_commit_handler, nullptr);
    EXPECT_EQ(reg_err, 0);

    err = textparser_parse(handle, &json_definition);
    EXPECT_EQ(err, 0);

    textparser_node *root = textparser_get_first_token(handle);
    ASSERT_NE(root, nullptr);

    textparser_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = TEXTPARSER_EVENT_COMMIT;
    ev.node = root;
    ev.start = 0;
    ev.end = strlen(text);

    textparser_action action = textparser_dispatch_event(handle, "test.onCommit", &ev);
    EXPECT_EQ(action, TEXTPARSER_ACTION_ACCEPT);
    EXPECT_TRUE(textparser_node_get_flags(root) & TEXTPARSER_NODE_SYNTHETIC);

    textparser_close(handle);
}

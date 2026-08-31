#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>

static bool dummy_predicate_check(textparser_t parser, const char *name, void *user_data) {
    (void)parser;
    (void)user_data;
    if (name && strcmp(name, "test.isIdentifierAllowed") == 0) {
        return true;
    }
    return false;
}

TEST(speculative_grammar, context_scopes_and_predicates) {
    textparser_t handle = nullptr;
    const char *code = "{}";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    // Scoped Context values
    EXPECT_FALSE(textparser_context_is(handle, "AllowAwait"));
    EXPECT_EQ(textparser_context_set(handle, "AllowAwait", 1), 0);
    EXPECT_TRUE(textparser_context_is(handle, "AllowAwait"));

    int64_t val = 0;
    EXPECT_EQ(textparser_context_get(handle, "AllowAwait", &val), 0);
    EXPECT_EQ(val, 1);

    EXPECT_EQ(textparser_context_set(handle, "InType", 42), 0);
    EXPECT_EQ(textparser_context_get(handle, "InType", &val), 0);
    EXPECT_EQ(val, 42);

    // Predicates
    EXPECT_EQ(textparser_register_predicate(handle, "test.isIdentifierAllowed", dummy_predicate_check, nullptr), 0);
    EXPECT_TRUE(textparser_eval_predicate(handle, "test.isIdentifierAllowed"));
    EXPECT_FALSE(textparser_eval_predicate(handle, "test.unknown"));

    textparser_close(handle);
}

TEST(speculative_grammar, speculation_and_rollback) {
    textparser_t handle = nullptr;
    const char *code = "{\"key\": 123}";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    textparser_push_mode(handle, "InitialMode");
    textparser_set_lexical_goal(handle, "ExpressionStart");

    void *spec_cp = nullptr;
    textparser_speculate_begin(handle, &spec_cp);
    ASSERT_NE(spec_cp, nullptr);

    // Mutate state during speculative branch
    textparser_push_mode(handle, "SpeculativeBranchMode");
    textparser_set_lexical_goal(handle, "SpeculativeGoal");
    EXPECT_STREQ(textparser_get_current_mode(handle), "SpeculativeBranchMode");
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "SpeculativeGoal");

    // Speculation fails: Roll back
    textparser_speculate_rollback(handle, spec_cp);

    EXPECT_STREQ(textparser_get_current_mode(handle), "InitialMode");
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "ExpressionStart");

    // Speculation succeeds: Commit
    void *spec_cp2 = nullptr;
    textparser_speculate_begin(handle, &spec_cp2);
    ASSERT_NE(spec_cp2, nullptr);

    textparser_push_mode(handle, "CommittedMode");
    textparser_speculate_commit(handle, spec_cp2);

    EXPECT_STREQ(textparser_get_current_mode(handle), "CommittedMode");

    textparser_close(handle);
}

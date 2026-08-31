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

TEST(speculative_grammar, rollback_restores_complete_transactional_state) {
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem("{}", 2, TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_push_mode(handle, "OuterMode"), 0);
    textparser_set_lexical_goal(handle, "ExpressionStart");
    ASSERT_EQ(textparser_context_set(handle, "AllowAwait", 1), 0);
    ASSERT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_WARNING,
                                           "BASE", "baseline", 0, 1), 0);

    void *checkpoint = nullptr;
    textparser_speculate_begin(handle, &checkpoint);
    ASSERT_NE(checkpoint, nullptr);

    textparser_parser_state_view state{};
    ASSERT_EQ(textparser_get_parser_state(handle, &state), 0);
    EXPECT_EQ(state.speculation_depth, 1u);
    EXPECT_EQ(state.mode_depth, 1u);
    EXPECT_EQ(state.context_depth, 1u);
    EXPECT_EQ(state.diagnostic_count, 1u);

    ASSERT_EQ(textparser_pop_mode(handle), 0);
    ASSERT_EQ(textparser_push_mode(handle, "BranchMode"), 0);
    textparser_set_lexical_goal(handle, "BranchGoal");
    ASSERT_EQ(textparser_context_set(handle, "AllowAwait", 0), 0);
    ASSERT_EQ(textparser_context_set(handle, "InType", 1), 0);
    textparser_clear_diagnostics(handle);
    ASSERT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR,
                                           "BRANCH", "branch only", 1, 1), 0);

    textparser_speculate_rollback(handle, checkpoint);
    EXPECT_STREQ(textparser_get_current_mode(handle), "OuterMode");
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "ExpressionStart");
    EXPECT_TRUE(textparser_context_is(handle, "AllowAwait"));
    EXPECT_FALSE(textparser_context_is(handle, "InType"));
    ASSERT_EQ(textparser_get_diagnostic_count(handle), 1u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(handle, 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "BASE");
    EXPECT_STREQ(diagnostic.message, "baseline");
    ASSERT_EQ(textparser_get_parser_state(handle, &state), 0);
    EXPECT_EQ(state.speculation_depth, 0u);
    EXPECT_EQ(state.mode_depth, 1u);
    EXPECT_EQ(state.context_depth, 1u);
    EXPECT_EQ(state.diagnostic_count, 1u);

    textparser_close(handle);
}

TEST(speculative_grammar, commit_keeps_changes_and_balances_depth) {
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem("x", 1, TEXTPARSER_ENCODING_LATIN1, &handle), 0);

    void *outer = nullptr;
    void *inner = nullptr;
    textparser_speculate_begin(handle, &outer);
    textparser_speculate_begin(handle, &inner);
    ASSERT_NE(outer, nullptr);
    ASSERT_NE(inner, nullptr);
    ASSERT_EQ(textparser_context_set(handle, "Committed", 7), 0);

    textparser_parser_state_view state{};
    ASSERT_EQ(textparser_get_parser_state(handle, &state), 0);
    EXPECT_EQ(state.speculation_depth, 2u);
    textparser_speculate_commit(handle, inner);
    ASSERT_EQ(textparser_get_parser_state(handle, &state), 0);
    EXPECT_EQ(state.speculation_depth, 1u);
    textparser_speculate_commit(handle, outer);
    ASSERT_EQ(textparser_get_parser_state(handle, &state), 0);
    EXPECT_EQ(state.speculation_depth, 0u);
    EXPECT_TRUE(textparser_context_is(handle, "Committed"));

    textparser_close(handle);
}

TEST(speculative_grammar, parser_state_tracks_completed_parse_cursor) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("{\"a\": 1}", 8, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&json_definition), 0);

    size_t token_count = 0;
    ASSERT_NE(parser.lexer_tokens(&token_count), nullptr);
    textparser_parser_state_view state{};
    ASSERT_EQ(parser.parser_state(&state), 0);
    EXPECT_EQ(state.source_offset, 8u);
    EXPECT_EQ(state.token_index, token_count);
    EXPECT_EQ(state.speculation_depth, 0u);
    EXPECT_EQ(textparser_get_parser_state(nullptr, &state), -1);
    EXPECT_EQ(textparser_get_parser_state(parser.get(), nullptr), -1);
}

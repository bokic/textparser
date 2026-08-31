/**
 * Phase 7.2 Comprehensive Conformance Tests
 * Tests nested speculation, mode/context rollback, trivia/ASI handling,
 * multi-error recovery loops, operator precedence corner cases, and
 * parser memory safety.
 */

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>

// ────────────────────────────────────────────────────────────────────────────
// SECTION 1: Nested Speculation & Rollback
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_nested_speculation, double_nested_commit_rollback) {
    textparser_t handle = nullptr;
    const char *code = "{}";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    void *outer_cp = nullptr;
    textparser_speculate_begin(handle, &outer_cp);
    EXPECT_NE(outer_cp, nullptr);

    void *inner_cp = nullptr;
    textparser_speculate_begin(handle, &inner_cp);
    EXPECT_NE(inner_cp, nullptr);

    // Set context inside inner speculation
    EXPECT_EQ(textparser_context_set(handle, "InnerScope", 99), 0);
    int64_t val = 0;
    EXPECT_EQ(textparser_context_get(handle, "InnerScope", &val), 0);
    EXPECT_EQ(val, 99);

    // Rollback inner — must not crash
    textparser_speculate_rollback(handle, inner_cp);

    // Commit outer — must not crash
    textparser_speculate_commit(handle, outer_cp);

    textparser_close(handle);
}

TEST(conformance_nested_speculation, rollback_restores_mode) {
    textparser_t handle = nullptr;
    const char *code = "x + y";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_EQ(textparser_push_mode(handle, "template"), 0);
    EXPECT_STREQ(textparser_get_current_mode(handle), "template");

    void *cp = nullptr;
    textparser_speculate_begin(handle, &cp);
    EXPECT_NE(cp, nullptr);

    EXPECT_EQ(textparser_push_mode(handle, "expression"), 0);
    EXPECT_STREQ(textparser_get_current_mode(handle), "expression");

    // Rollback should restore mode to "template"
    textparser_speculate_rollback(handle, cp);
    EXPECT_STREQ(textparser_get_current_mode(handle), "template");

    EXPECT_EQ(textparser_pop_mode(handle), 0);
    textparser_close(handle);
}

TEST(conformance_nested_speculation, three_level_nested_speculation) {
    textparser_t handle = nullptr;
    const char *code = "a b c";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    void *cp1 = nullptr, *cp2 = nullptr, *cp3 = nullptr;
    textparser_speculate_begin(handle, &cp1);
    EXPECT_EQ(textparser_context_set(handle, "L1", 1), 0);

    textparser_speculate_begin(handle, &cp2);
    EXPECT_EQ(textparser_context_set(handle, "L2", 2), 0);

    textparser_speculate_begin(handle, &cp3);
    EXPECT_EQ(textparser_context_set(handle, "L3", 3), 0);

    textparser_speculate_rollback(handle, cp3);
    textparser_speculate_rollback(handle, cp2);
    textparser_speculate_commit(handle, cp1);

    textparser_close(handle);
}

// ────────────────────────────────────────────────────────────────────────────
// SECTION 2: Mode Stack Edge Cases
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_mode_stack, multiple_push_pop) {
    textparser_t handle = nullptr;
    const char *code = "test";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    const char *modes[] = {"mode1","mode2","mode3","mode4","mode5"};
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(textparser_push_mode(handle, modes[i]), 0);
        EXPECT_STREQ(textparser_get_current_mode(handle), modes[i]);
    }
    for (int i = 3; i >= 0; i--) {
        EXPECT_EQ(textparser_pop_mode(handle), 0);
        EXPECT_STREQ(textparser_get_current_mode(handle), modes[i]);
    }
    EXPECT_EQ(textparser_pop_mode(handle), 0);

    textparser_close(handle);
}

TEST(conformance_mode_stack, pop_empty_stack_is_error) {
    textparser_t handle = nullptr;
    const char *code = "";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    int rc = textparser_pop_mode(handle);
    EXPECT_NE(rc, 0);

    textparser_close(handle);
}

TEST(conformance_mode_stack, goal_persists_across_mode_changes) {
    textparser_t handle = nullptr;
    const char *code = "x / y";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    textparser_set_lexical_goal(handle, "ExpressionStart");
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "ExpressionStart");

    EXPECT_EQ(textparser_push_mode(handle, "regex"), 0);
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "ExpressionStart");

    EXPECT_EQ(textparser_pop_mode(handle), 0);
    EXPECT_STREQ(textparser_get_lexical_goal(handle), "ExpressionStart");

    // Clear goal
    textparser_set_lexical_goal(handle, nullptr);
    EXPECT_EQ(textparser_get_lexical_goal(handle), nullptr);

    textparser_close(handle);
}

// ────────────────────────────────────────────────────────────────────────────
// SECTION 3: Multi-Error Recovery Loop Termination
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_recovery, multiple_diagnostic_rounds) {
    textparser_t handle = nullptr;
    const char *code = "line1\nline2\nline3\nline4\nline5";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    struct DiagCase {
        size_t pos;
        textparser_diagnostic_severity sev;
        const char *code_str;
        const char *msg;
    } cases[] = {
        {0,  TEXTPARSER_SEVERITY_ERROR,   "E001", "Unexpected token at start"},
        {6,  TEXTPARSER_SEVERITY_WARNING, "W010", "Possible ASI insertion"},
        {12, TEXTPARSER_SEVERITY_INFO,    "I100", "Unreachable statement"},
        {18, TEXTPARSER_SEVERITY_ERROR,   "E002", "Missing closing brace"},
        {24, TEXTPARSER_SEVERITY_HINT,    "H001", "Consider rewriting"},
    };

    for (auto &c : cases)
        EXPECT_EQ(textparser_report_diagnostic(handle, c.sev, c.code_str, c.msg, c.pos, 1), 0);

    EXPECT_EQ(textparser_get_diagnostic_count(handle), 5u);

    for (size_t i = 0; i < 5; i++) {
        textparser_diagnostic d;
        EXPECT_EQ(textparser_get_diagnostic(handle, i, &d), 0);
        EXPECT_EQ(d.severity, cases[i].sev);
        EXPECT_STREQ(d.code, cases[i].code_str);
        EXPECT_STREQ(d.message, cases[i].msg);
        EXPECT_EQ(d.start_pos, cases[i].pos);
    }

    textparser_clear_diagnostics(handle);
    EXPECT_EQ(textparser_get_diagnostic_count(handle), 0u);

    textparser_diagnostic bogus;
    EXPECT_NE(textparser_get_diagnostic(handle, 0, &bogus), 0);

    textparser_close(handle);
}

TEST(conformance_recovery, sync_recovery_at_eof_always_terminates) {
    textparser_t handle = nullptr;
    const char *code = "x";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    textparser_parse(handle, &json_definition);

    // Passing NULL sync list with count=0 — must not loop forever or crash
    size_t new_offset = 0;
    int rc = textparser_recover_until_token(handle, nullptr, 0, &new_offset);
    EXPECT_TRUE(rc != 0 || new_offset == strlen(code));

    textparser_close(handle);
}

TEST(conformance_recovery, null_code_diagnostic_accepted) {
    textparser_t handle = nullptr;
    const char *code = "foo";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, nullptr, "No code", 0, 1), 0);
    textparser_diagnostic d;
    EXPECT_EQ(textparser_get_diagnostic(handle, 0, &d), 0);
    EXPECT_EQ(d.code, nullptr);
    EXPECT_STREQ(d.message, "No code");

    textparser_close(handle);
}

// ────────────────────────────────────────────────────────────────────────────
// SECTION 4: Operator Precedence Corner Cases
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_operator_precedence, unknown_token_not_in_table) {
    textparser_t handle = nullptr;
    const char *code = "1+2";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    textparser_operator_def out_op = {};
    int rc = textparser_get_operator(handle, 9999, -1, &out_op);
    EXPECT_NE(rc, 0); // Not found

    textparser_close(handle);
}

TEST(conformance_operator_precedence, register_all_roles) {
    textparser_t handle = nullptr;
    const char *code = "a";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    struct OpCase {
        int token_id;
        textparser_operator_role role;
        int precedence;
        enum textparser_associativity op_assoc;
    } ops[] = {
        {200, TEXTPARSER_OP_PREFIX,   15, TEXTPARSER_ASSOC_RIGHT},
        {201, TEXTPARSER_OP_INFIX,    10, TEXTPARSER_ASSOC_LEFT},
        {202, TEXTPARSER_OP_POSTFIX,  16, TEXTPARSER_ASSOC_LEFT},
        {203, TEXTPARSER_OP_TERNARY,   3, TEXTPARSER_ASSOC_RIGHT},
    };

    for (auto &op : ops) {
        textparser_operator_def def = {
            .token_id = op.token_id, .role = op.role,
            .precedence = op.precedence, .associativity = op.op_assoc,
            .secondary_token_id = 0
        };
        EXPECT_EQ(textparser_register_operator(handle, &def), 0);
    }

    for (auto &op : ops) {
        textparser_operator_def out = {};
        EXPECT_EQ(textparser_get_operator(handle, op.token_id, (int)op.role, &out), 0);
        EXPECT_EQ(out.token_id, op.token_id);
        EXPECT_EQ(out.role, op.role);
        EXPECT_EQ(out.precedence, op.precedence);
        EXPECT_EQ(out.associativity, op.op_assoc);
    }

    textparser_close(handle);
}

TEST(conformance_operator_precedence, re_register_overwrites) {
    textparser_t handle = nullptr;
    const char *code = "x";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    textparser_operator_def def1 = {.token_id=300,.role=TEXTPARSER_OP_INFIX,.precedence=5,.associativity=TEXTPARSER_ASSOC_LEFT,.secondary_token_id=0};
    EXPECT_EQ(textparser_register_operator(handle, &def1), 0);

    textparser_operator_def def2 = {.token_id=300,.role=TEXTPARSER_OP_INFIX,.precedence=20,.associativity=TEXTPARSER_ASSOC_RIGHT,.secondary_token_id=0};
    EXPECT_EQ(textparser_register_operator(handle, &def2), 0);

    textparser_operator_def out = {};
    EXPECT_EQ(textparser_get_operator(handle, 300, (int)TEXTPARSER_OP_INFIX, &out), 0);
    EXPECT_EQ(out.precedence, 20);
    EXPECT_EQ(out.associativity, TEXTPARSER_ASSOC_RIGHT);

    textparser_close(handle);
}

// ────────────────────────────────────────────────────────────────────────────
// SECTION 5: Decoder & Validator Edge Cases
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_decoders, validator_rejects_invalid) {
    textparser_t handle = nullptr;
    const char *code = "hello";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    // validator_fn: (handle, raw_text, length, out_error, user_data) -> bool
    auto validator = [](textparser_t, const char *input, size_t, const char **out_error, void *) -> bool {
        if (!input || input[0] == 'z') {
            if (out_error) *out_error = "starts with z";
            return false;
        }
        return true;
    };

    EXPECT_EQ(textparser_register_validator(handle, "test.noZValidator", validator, nullptr), 0);
    const char *err = nullptr;
    EXPECT_TRUE(textparser_validate_token(handle, "test.noZValidator", "hello", 5, &err));
    EXPECT_FALSE(textparser_validate_token(handle, "test.noZValidator", "zebra", 5, &err));
    EXPECT_FALSE(textparser_validate_token(handle, "test.noZValidator", "zoo", 3, &err));

    textparser_close(handle);
}

TEST(conformance_decoders, unknown_decoder_returns_null) {
    textparser_t handle = nullptr;
    const char *code = "x";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    // Calling an unregistered decoder should return nullptr
    char *result = textparser_decode_token(handle, "test.notRegistered", "input", 5);
    EXPECT_EQ(result, nullptr);

    textparser_close(handle);
}

// ────────────────────────────────────────────────────────────────────────────
// SECTION 6: Trivia / Line-Terminator Predicates
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_trivia, no_line_terminator_within_single_line) {
    textparser_t handle = nullptr;
    const char *code = "a b c";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_FALSE(textparser_has_line_terminator_between(handle, 0, 4));

    textparser_close(handle);
}

TEST(conformance_trivia, line_terminator_across_newline) {
    textparser_t handle = nullptr;
    const char *code = "a\nb";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_TRUE(textparser_has_line_terminator_between(handle, 0, 2));
    EXPECT_FALSE(textparser_has_line_terminator_between(handle, 0, 0));

    textparser_close(handle);
}

TEST(conformance_trivia, multiple_newlines_detected) {
    textparser_t handle = nullptr;
    const char *code = "a\n\n\nb";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_TRUE(textparser_has_line_terminator_between(handle, 0, 4));

    textparser_close(handle);
}

TEST(conformance_trivia, crlf_counts_as_line_terminator) {
    textparser_t handle = nullptr;
    const char *code = "a\r\nb";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_TRUE(textparser_has_line_terminator_between(handle, 0, 3));

    textparser_close(handle);
}

// ────────────────────────────────────────────────────────────────────────────
// SECTION 7: Parser Memory Safety
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_memory_safety, null_handle_all_apis_safe) {
    EXPECT_EQ(textparser_get_diagnostic_count(nullptr), 0u);
    EXPECT_NE(textparser_get_diagnostic(nullptr, 0, nullptr), 0);
    textparser_clear_diagnostics(nullptr);

    EXPECT_NE(textparser_push_mode(nullptr, "x"), 0);
    EXPECT_NE(textparser_pop_mode(nullptr), 0);
    textparser_get_current_mode(nullptr); // Must not crash (may return default mode name)

    textparser_set_lexical_goal(nullptr, "ExpressionStart"); // void, must not crash
    EXPECT_EQ(textparser_get_lexical_goal(nullptr), nullptr);

    EXPECT_FALSE(textparser_has_line_terminator_between(nullptr, 0, 1));

    EXPECT_NE(textparser_context_set(nullptr, "K", 1), 0);
    int64_t v = 0;
    EXPECT_NE(textparser_context_get(nullptr, "K", &v), 0);
    EXPECT_FALSE(textparser_context_is(nullptr, "K"));

    EXPECT_NE(textparser_register_predicate(nullptr, "p", nullptr, nullptr), 0);
    EXPECT_FALSE(textparser_eval_predicate(nullptr, "p"));

    // speculate_begin/commit/rollback are void — just must not crash with nullptr
    void *cp = nullptr;
    textparser_speculate_begin(nullptr, &cp);
    textparser_speculate_commit(nullptr, nullptr);
    textparser_speculate_rollback(nullptr, nullptr);

    textparser_operator_def out = {};
    EXPECT_NE(textparser_get_operator(nullptr, 1, -1, &out), 0);
    EXPECT_NE(textparser_register_operator(nullptr, nullptr), 0);

    textparser_node *node_out = nullptr;
    EXPECT_NE(textparser_parse_pratt_expression(nullptr, 0, &node_out), 0);

    textparser_close(nullptr);
}

TEST(conformance_memory_safety, open_close_repeated) {
    for (int i = 0; i < 100; i++) {
        textparser_t handle = nullptr;
        const char *code = "{\"k\": 1}";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        textparser_parse(handle, &json_definition);
        EXPECT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, "E1", "err", 0, 1), 0);
        textparser_close(handle);
    }
}

TEST(conformance_memory_safety, large_diagnostic_batch) {
    textparser_t handle = nullptr;
    const char *code = "x";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    for (int i = 0; i < 1000; i++) {
        std::string msg = "Error number " + std::to_string(i);
        EXPECT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, "E999", msg.c_str(), 0, 1), 0);
    }
    EXPECT_EQ(textparser_get_diagnostic_count(handle), 1000u);

    textparser_diagnostic d;
    EXPECT_EQ(textparser_get_diagnostic(handle, 999, &d), 0);
    EXPECT_STREQ(d.message, "Error number 999");

    textparser_clear_diagnostics(handle);
    EXPECT_EQ(textparser_get_diagnostic_count(handle), 0u);

    textparser_close(handle);
}

// ────────────────────────────────────────────────────────────────────────────
// SECTION 8: Definition Migration Parity
// ────────────────────────────────────────────────────────────────────────────

TEST(conformance_migration_parity, json_definition_has_tokens) {
    const textparser_language_definition *def = &json_definition;
    ASSERT_NE(def, nullptr);
    EXPECT_STREQ(def->name, "json");

    int count = 0;
    while (def->tokens[count].name != nullptr) count++;
    EXPECT_GT(count, 5);
}

TEST(conformance_migration_parity, json_definition_parses_valid_document) {
    textparser_t handle = nullptr;
    const char *code = "{\"key\": [1, 2, 3]}";
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_EQ(textparser_parse(handle, &json_definition), 0);
    EXPECT_NE(textparser_get_first_token(handle), nullptr);
    EXPECT_EQ(textparser_get_diagnostic_count(handle), 0u);

    textparser_close(handle);
}

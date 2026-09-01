#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <cstring>

namespace {

const char *grammar_lexer_json = R"json({
  "name": "grammar_executor_test",
  "version": 1,
  "caseSensitivity": true,
  "defaultFileExtensions": ["txt"],
  "defaultTextEncoding": "utf-8",
  "startTokens": ["A", "B", "C"],
  "otherTextInside": true,
  "tokens": {
    "A": {"type": "SimpleToken", "startRegex": "a"},
    "B": {"type": "SimpleToken", "startRegex": "b"},
    "C": {"type": "SimpleToken", "startRegex": "c"}
  }
})json";

int token_id(const textparser_language_definition *definition, const char *name) {
    for (int i = 0; definition->tokens[i].name != nullptr; i++) {
        if (std::strcmp(definition->tokens[i].name, name) == 0) return i;
    }
    return -1;
}

struct GrammarFixture : testing::Test {
    textparser_language_definition *definition = nullptr;
    textparser::Parser parser;

    void SetUp() override {
        ASSERT_EQ(textparser_json_load_language_definition_from_string(grammar_lexer_json, &definition), 0);
    }

    void TearDown() override {
        parser.reset();
        textparser_free_language_definition(definition);
    }

    void parse(const char *source) {
        ASSERT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(definition), 0);
    }
};

} // namespace

TEST_F(GrammarFixture, token_ref_sequence_optional_and_repeat) {
    parse("a b b c");
    int a = token_id(definition, "A");
    int b = token_id(definition, "B");
    int c = token_id(definition, "C");
    ASSERT_GE(a, 0);
    ASSERT_GE(b, 0);
    ASSERT_GE(c, 0);

    const int ref_b_child[] = {3};
    const int repeat_b_child[] = {4};
    const int optional_c_child[] = {2};
    const int root_children[] = {0, 5, 6};
    const textparser_production productions[] = {
        {0, "A", TEXTPARSER_PROD_TOKEN, nullptr, 0, a, -1},
        {1, "B", TEXTPARSER_PROD_TOKEN, nullptr, 0, b, -1},
        {2, "C", TEXTPARSER_PROD_TOKEN, nullptr, 0, c, -1},
        {3, "BRef", TEXTPARSER_PROD_REF, nullptr, 0, -1, 1},
        {4, "BRefSequence", TEXTPARSER_PROD_SEQUENCE, ref_b_child, 1, -1, -1},
        {5, "RepeatB", TEXTPARSER_PROD_REPEAT, repeat_b_child, 1, -1, -1},
        {6, "OptionalC", TEXTPARSER_PROD_OPTIONAL, optional_c_child, 1, -1, -1},
        {7, "Root", TEXTPARSER_PROD_SEQUENCE, root_children, 3, -1, -1},
    };

    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 7, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 4u);
    ASSERT_NE(result.node, nullptr);
    EXPECT_EQ(result.node->token_id, 7);
    EXPECT_EQ(result.node->len, 7u);
    EXPECT_NE(result.node->node_flags & TEXTPARSER_NODE_SYNTHETIC, 0u);
    ASSERT_NE(result.node->child, nullptr);
    EXPECT_EQ(result.node->child->token_id, a);

    textparser_parser_state_view state{};
    ASSERT_EQ(parser.parser_state(&state), 0);
    EXPECT_EQ(state.token_index, 4u);
    EXPECT_EQ(state.source_offset, 7u);
    EXPECT_EQ(state.speculation_depth, 0u);
}

TEST_F(GrammarFixture, choice_rolls_back_failed_alternative) {
    parse("a");
    int a = token_id(definition, "A");
    int c = token_id(definition, "C");
    const int failed_children[] = {0, 1};
    const int choices[] = {2, 0};
    const textparser_production productions[] = {
        {0, "A", TEXTPARSER_PROD_TOKEN, nullptr, 0, a, -1},
        {1, "C", TEXTPARSER_PROD_TOKEN, nullptr, 0, c, -1},
        {2, "AC", TEXTPARSER_PROD_SEQUENCE, failed_children, 2, -1, -1},
        {3, "Choice", TEXTPARSER_PROD_CHOICE, choices, 2, -1, -1},
    };

    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 3, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 1u);
    ASSERT_NE(result.node, nullptr);
    EXPECT_EQ(result.node->token_id, a);
}

TEST_F(GrammarFixture, failed_sequence_restores_cursor) {
    parse("a");
    int a = token_id(definition, "A");
    int c = token_id(definition, "C");
    const int children[] = {0, 1};
    const textparser_production productions[] = {
        {0, "A", TEXTPARSER_PROD_TOKEN, nullptr, 0, a, -1},
        {1, "C", TEXTPARSER_PROD_TOKEN, nullptr, 0, c, -1},
        {2, "AC", TEXTPARSER_PROD_SEQUENCE, children, 2, -1, -1},
    };

    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 2, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_NO);
    EXPECT_EQ(result.node, nullptr);
    EXPECT_EQ(result.consumed_tokens, 0u);
    textparser_parser_state_view state{};
    ASSERT_EQ(parser.parser_state(&state), 0);
    EXPECT_EQ(state.token_index, 0u);
    EXPECT_EQ(state.source_offset, 0u);
}

TEST_F(GrammarFixture, optional_mismatch_and_empty_repeat_succeed) {
    parse("a");
    int b = token_id(definition, "B");
    const int optional_child[] = {0};
    const int repeat_child[] = {1};
    const int root_children[] = {1, 2};
    const textparser_production productions[] = {
        {0, "B", TEXTPARSER_PROD_TOKEN, nullptr, 0, b, -1},
        {1, "OptionalB", TEXTPARSER_PROD_OPTIONAL, optional_child, 1, -1, -1},
        {2, "RepeatOptional", TEXTPARSER_PROD_REPEAT, repeat_child, 1, -1, -1},
        {3, "Root", TEXTPARSER_PROD_SEQUENCE, root_children, 2, -1, -1},
    };

    textparser_match_result optional{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 1, &optional), 0);
    EXPECT_EQ(optional.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(optional.consumed_tokens, 0u);
    EXPECT_EQ(optional.node, nullptr);

    textparser_match_result repeat{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 2, &repeat), 0);
    EXPECT_EQ(repeat.status, TEXTPARSER_MATCH_ERROR);
    EXPECT_EQ(repeat.consumed_tokens, 0u);
}

TEST_F(GrammarFixture, invalid_references_shapes_and_arguments_are_errors) {
    parse("a");
    const int too_many_optional_children[] = {0, 0};
    const textparser_production productions[] = {
        {0, "BadRef", TEXTPARSER_PROD_REF, nullptr, 0, -1, 99},
        {1, "BadOptional", TEXTPARSER_PROD_OPTIONAL, too_many_optional_children, 2, -1, -1},
        {2, "EmptySequence", TEXTPARSER_PROD_SEQUENCE, nullptr, 0, -1, -1},
        {3, "EmptyChoice", TEXTPARSER_PROD_CHOICE, nullptr, 0, -1, -1},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 0, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_ERROR);
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 1, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_ERROR);
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 2, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 0u);
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 3, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_NO);
    EXPECT_EQ(textparser_execute_production(nullptr, productions, std::size(productions), 0, &result), -1);
    EXPECT_EQ(parser.execute_production(nullptr, 0, 0, &result), -1);
    EXPECT_EQ(parser.execute_production(productions, std::size(productions), 0, nullptr), -1);
}

TEST_F(GrammarFixture, recursive_ref_cycle_is_bounded) {
    parse("a");
    const textparser_production productions[] = {
        {0, "CycleA", TEXTPARSER_PROD_REF, nullptr, 0, -1, 1},
        {1, "CycleB", TEXTPARSER_PROD_REF, nullptr, 0, -1, 0},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 0, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_ERROR);
    EXPECT_EQ(result.consumed_tokens, 0u);
}

namespace {
struct PredicateObservation {
    int a;
    int b;
    bool called;
};

bool parser_predicate_observer(textparser_t parser,
                               const textparser_predicate_context *context,
                               void *user_data) {
    auto *observation = static_cast<PredicateObservation *>(user_data);
    observation->called = true;
    textparser_context_set(parser, "PredicateLeak", 99);
    int64_t enabled = 0;
    return context != nullptr && context->production_id == 2 &&
           context->previous != nullptr && context->previous->kind == observation->a &&
           context->current != nullptr && context->current->kind == observation->b &&
           context->has_preceding_line_terminator &&
           textparser_context_get(parser, "Enabled", &enabled) == 0 && enabled == 7;
}
} // namespace

TEST_F(GrammarFixture, lookahead_and_not_are_zero_width) {
    parse("a b");
    int a = token_id(definition, "A");
    int b = token_id(definition, "B");
    int c = token_id(definition, "C");
    const int look_a[] = {0};
    const int not_c[] = {2};
    const int root[] = {3, 0, 4, 1};
    const textparser_production productions[] = {
        {0, "A", TEXTPARSER_PROD_TOKEN, nullptr, 0, a, -1, nullptr, nullptr, 0},
        {1, "B", TEXTPARSER_PROD_TOKEN, nullptr, 0, b, -1, nullptr, nullptr, 0},
        {2, "C", TEXTPARSER_PROD_TOKEN, nullptr, 0, c, -1, nullptr, nullptr, 0},
        {3, "AheadA", TEXTPARSER_PROD_LOOKAHEAD, look_a, 1, -1, -1, nullptr, nullptr, 0},
        {4, "NotC", TEXTPARSER_PROD_NOT, not_c, 1, -1, -1, nullptr, nullptr, 0},
        {5, "Root", TEXTPARSER_PROD_SEQUENCE, root, 4, -1, -1, nullptr, nullptr, 0},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 5, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 2u);
}

TEST_F(GrammarFixture, parser_predicate_observes_tokens_trivia_and_scoped_context) {
    parse("a\nb");
    int a = token_id(definition, "A");
    int b = token_id(definition, "B");
    PredicateObservation observation{a, b, false};
    ASSERT_EQ(textparser_register_parser_predicate(
                  parser.get(), "test.beforeB", parser_predicate_observer, &observation), 0);
    const int scoped_sequence[] = {2, 1};
    const int context_child[] = {3};
    const int root[] = {0, 4};
    const textparser_production productions[] = {
        {0, "A", TEXTPARSER_PROD_TOKEN, nullptr, 0, a, -1, nullptr, nullptr, 0},
        {1, "B", TEXTPARSER_PROD_TOKEN, nullptr, 0, b, -1, nullptr, nullptr, 0},
        {2, "BeforeB", TEXTPARSER_PROD_PREDICATE, nullptr, 0, -1, -1, "test.beforeB", nullptr, 0},
        {3, "GuardedB", TEXTPARSER_PROD_SEQUENCE, scoped_sequence, 2, -1, -1, nullptr, nullptr, 0},
        {4, "Context", TEXTPARSER_PROD_CONTEXT, context_child, 1, -1, -1, nullptr, "Enabled", 7},
        {5, "Root", TEXTPARSER_PROD_SEQUENCE, root, 2, -1, -1, nullptr, nullptr, 0},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 5, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_TRUE(observation.called);
    int64_t value = 0;
    EXPECT_NE(textparser_context_get(parser.get(), "Enabled", &value), 0);
    EXPECT_NE(textparser_context_get(parser.get(), "PredicateLeak", &value), 0);
}

TEST_F(GrammarFixture, commit_stops_choice_rollback_after_prefix) {
    parse("a c");
    int a = token_id(definition, "A");
    int b = token_id(definition, "B");
    int c = token_id(definition, "C");
    const int committed_alt[] = {0, 3, 1};
    const int fallback_alt[] = {0, 2};
    const int choices[] = {4, 5};
    const textparser_production productions[] = {
        {0, "A", TEXTPARSER_PROD_TOKEN, nullptr, 0, a, -1, nullptr, nullptr, 0},
        {1, "B", TEXTPARSER_PROD_TOKEN, nullptr, 0, b, -1, nullptr, nullptr, 0},
        {2, "C", TEXTPARSER_PROD_TOKEN, nullptr, 0, c, -1, nullptr, nullptr, 0},
        {3, "Commit", TEXTPARSER_PROD_COMMIT, nullptr, 0, -1, -1, nullptr, nullptr, 0},
        {4, "AB", TEXTPARSER_PROD_SEQUENCE, committed_alt, 3, -1, -1, nullptr, nullptr, 0},
        {5, "AC", TEXTPARSER_PROD_SEQUENCE, fallback_alt, 2, -1, -1, nullptr, nullptr, 0},
        {6, "Choice", TEXTPARSER_PROD_CHOICE, choices, 2, -1, -1, nullptr, nullptr, 0},
    };
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_production(productions, std::size(productions), 6, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_ERROR);
    EXPECT_TRUE(result.committed);
    textparser_parser_state_view state{};
    ASSERT_EQ(parser.parser_state(&state), 0);
    EXPECT_EQ(state.token_index, 1u);
}

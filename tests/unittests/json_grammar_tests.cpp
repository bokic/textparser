#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <cstring>
#include <string>
#include <vector>

namespace {

std::string language_with_grammar(const std::string &grammar) {
    return R"json({
      "name":"json_grammar", "version":2, "caseSensitivity":true,
      "defaultFileExtensions":["txt"], "defaultTextEncoding":"utf-8",
      "startTokens":["A","B","C"], "otherTextInside":true,
      "tokens":{
        "A":{"type":"SimpleToken","startRegex":"a"},
        "B":{"type":"SimpleToken","startRegex":"b"},
        "C":{"type":"SimpleToken","startRegex":"c"}
      },
      "grammar":)json" + grammar + "}";
}

int load(const std::string &grammar, textparser_language_definition **definition) {
    std::string json = language_with_grammar(grammar);
    return textparser_json_load_language_definition_from_string(json.c_str(), definition);
}

struct LifecycleRecord {
    std::vector<std::string> entries;
    bool reject_validation = false;
    bool reject_commit = false;
};

struct LifecycleBinding {
    LifecycleRecord *record;
    const char *name;
};

textparser_action lifecycle_handler(textparser_t,
                                    const textparser_event *event,
                                    void *user_data) {
    auto *binding = static_cast<LifecycleBinding *>(user_data);
    std::string entry = binding->name;
    entry += ":" + std::to_string(static_cast<int>(event->type));
    if (event->configuration) {
        entry += ":" + std::string(static_cast<const char *>(event->configuration));
    }
    binding->record->entries.push_back(entry);
    if (event->type == TEXTPARSER_EVENT_VALIDATE && binding->record->reject_validation)
        return TEXTPARSER_ACTION_REJECT;
    if (event->type == TEXTPARSER_EVENT_COMMIT && binding->record->reject_commit)
        return TEXTPARSER_ACTION_REJECT;
    return TEXTPARSER_ACTION_ACCEPT;
}

} // namespace

TEST(json_grammar, loads_flattens_resolves_and_executes) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{
        "Root":{"sequence":[
          {"token":"A"},
          {"repeat":{"ref":"BToken"}},
          {"optional":{"token":"C"}}
        ]},
        "BToken":{"token":"B"}
      }
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(definition, nullptr);
    ASSERT_NE(definition->grammar, nullptr);
    EXPECT_EQ(definition->grammar->start_production, 0);
    EXPECT_EQ(definition->grammar->production_count, 7u);
    EXPECT_STREQ(definition->grammar->productions[0].name, "Root");
    EXPECT_EQ(definition->grammar->productions[0].kind, TEXTPARSER_PROD_SEQUENCE);

    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a b b c", 7, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 4u);
    ASSERT_NE(result.node, nullptr);
    EXPECT_EQ(result.node->len, 7u);

    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, loads_schema_v2_lexer_and_trivia_names) {
    const char *json = R"json({
      "formatVersion":2,
      "name":"schema_v2_minimal", "version":2, "caseSensitivity":true,
      "defaultFileExtensions":["txt"], "defaultTextEncoding":"utf-8",
      "otherTextInside":true,
      "lexer":{
        "tokens":{"A":{"regex":"a"}},
        "trivia":{"Comment":{"regex":"//[^\\r\\n]*"}}
      },
      "grammar":{
        "start":"Root",
        "productions":{"Root":{"choice":[{"token":"A"},{"token":"Comment"}]}}
      }
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(json, &definition), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(definition, nullptr);
    ASSERT_NE(definition->grammar, nullptr);
    ASSERT_NE(definition->tokens, nullptr);
    EXPECT_STREQ(definition->tokens[0].name, "A");
    EXPECT_STREQ(definition->tokens[1].name, "Comment");

    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a", 1, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 1u);

    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, validates_structure_and_names) {
    struct Case { const char *grammar; int expected; } cases[] = {
        {"[]", TEXTPARSER_JSON_GRAMMAR_NOT_OBJECT},
        {R"({"productions":{"Root":{"token":"A"}}})", TEXTPARSER_JSON_GRAMMAR_START_NOT_FOUND},
        {R"({"start":"Root"})", TEXTPARSER_JSON_GRAMMAR_PRODUCTIONS_NOT_OBJECT},
        {R"({"start":"Root","productions":{}})", TEXTPARSER_JSON_GRAMMAR_PRODUCTIONS_NOT_OBJECT},
        {R"({"start":"Missing","productions":{"Root":{"token":"A"}}})", TEXTPARSER_JSON_GRAMMAR_UNDEFINED_REFERENCE},
        {R"({"start":"Root","productions":{"Root":{"token":"Missing"}}})", TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN},
        {R"({"start":"Root","productions":{"Root":{"ref":"Missing"}}})", TEXTPARSER_JSON_GRAMMAR_UNDEFINED_REFERENCE},
        {R"({"start":"Root","productions":{"Root":{"token":"A","ref":"Root"}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"token":"A","lookahead":{"token":"A"}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"sequence":{}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"optional":[]}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"token":"A","allowASI":"yes"}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"sequence":[],"allowASI":true}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"token":"A","recover":{"skip":true}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"token":"A","recoverUntil":["Missing"]}}})", TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN},
        {R"({"start":"Root","productions":{"Root":{"token":"A","events":{"onCommit":""}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"token":"A","events":{"onCommit":{"configuration":{}}}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","events":{"onSourceComplete":{"handler":"done","extra":1}},"productions":{"Root":{"token":"A"}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
    };
    for (const auto &item : cases) {
        textparser_language_definition *definition = nullptr;
        EXPECT_EQ(load(item.grammar, &definition), item.expected) << item.grammar;
        EXPECT_EQ(definition, nullptr);
        EXPECT_STRNE(textparser_json_strerror(item.expected), "Unknown JSON parser error");
    }
}

TEST(json_grammar, inserts_missing_tokens_for_automatic_semicolon_recovery) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"sequence":[
        {"token":"A"},
        {"token":"B","allowASI":true,"expect":"semicolon"}
      ]}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a", 1, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_NE(result.node, nullptr);
    ASSERT_NE(result.node->child, nullptr);
    textparser_node *missing = result.node->child->next;
    ASSERT_NE(missing, nullptr);
    EXPECT_EQ(missing->len, 0u);
    EXPECT_NE(missing->node_flags & TEXTPARSER_NODE_MISSING, 0u);
    EXPECT_NE(missing->node_flags & TEXTPARSER_NODE_SYNTHETIC, 0u);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TEXTPARSER_EXPECTED");
    EXPECT_STREQ(diagnostic.message, "Expected semicolon.");
    EXPECT_EQ(diagnostic.start_pos, 1u);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, skips_unexpected_tokens_and_stops_before_synchronization_token) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"sequence":[
        {"token":"A"},
        {"token":"C","expect":"C token","recover":{"skip":true,"synchronize":["B"]}},
        {"token":"B"}
      ]}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a a b", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 3u);
    textparser_node *recovered = result.node->child->next;
    ASSERT_NE(recovered, nullptr);
    EXPECT_NE(recovered->node_flags & TEXTPARSER_NODE_RECOVERED, 0u);
    EXPECT_NE(recovered->node_flags & TEXTPARSER_NODE_SYNTHETIC, 0u);
    ASSERT_NE(recovered->child, nullptr);
    EXPECT_EQ(recovered->child->len, 1u);
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, uses_validated_global_synchronization_tokens) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"sequence":[
        {"token":"A"},
        {"token":"C","recover":{"skip":true}},
        {"token":"B"}
      ]}}
    })json";
    std::string json = language_with_grammar(grammar);
    const size_t grammar_key = json.find("\"grammar\":");
    ASSERT_NE(grammar_key, std::string::npos);
    json.insert(grammar_key, "\"recovery\":{\"synchronizationTokens\":[\"B\"]},");
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(json.c_str(), &definition),
              TEXTPARSER_JSON_NO_ERROR);
    ASSERT_EQ(definition->recovery_sync_token_count, 1u);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a a b", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 3u);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, reports_only_the_furthest_failed_alternative) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"choice":[
        {"sequence":[{"token":"A"},{"token":"C","expect":"C after A"}]},
        {"token":"C","expect":"leading C"}
      ]}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a b", 3, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_NO);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.message, "Expected C after A.");
    EXPECT_EQ(diagnostic.start_pos, 1u);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, queues_bottom_up_commits_discards_rejected_branches_and_completes_at_eof) {
    const std::string grammar = R"json({
      "start":"Root",
      "events":{"onSourceComplete":"source"},
      "productions":{"Root":{
        "choice":[
          {"sequence":[
            {"token":"A","events":{"onCommit":"abandoned"}},
            {"token":"B"}
          ]},
          {"sequence":[
            {"token":"A"},
            {"token":"C","events":{"onCommit":{
              "handler":"leaf","configuration":{"role":"leaf"}
            }}}
          ]}
        ],
        "events":{"onCommit":"root"}
      }}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    LifecycleRecord record;
    LifecycleBinding abandoned{&record, "abandoned"};
    LifecycleBinding leaf{&record, "leaf"};
    LifecycleBinding root{&record, "root"};
    LifecycleBinding source{&record, "source"};
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a c", 3, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "abandoned", lifecycle_handler, &abandoned), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "leaf", lifecycle_handler, &leaf), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "root", lifecycle_handler, &root), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "source", lifecycle_handler, &source), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_EQ(record.entries.size(), 3u);
    EXPECT_EQ(record.entries[0], "leaf:1:{\"role\":\"leaf\"}");
    EXPECT_EQ(record.entries[1], "root:1");
    EXPECT_EQ(record.entries[2], "source:3");
    textparser_parser_state_view state{};
    ASSERT_EQ(parser.parser_state(&state), 0);
    EXPECT_EQ(state.pending_event_count, 0u);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, validation_can_reject_an_alternative_without_publishing_its_commit) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"choice":[
        {"token":"A","events":{"onValidate":"validate","onCommit":"rejected"}},
        {"token":"A","events":{"onCommit":"accepted"}}
      ]}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    LifecycleRecord record;
    record.reject_validation = true;
    LifecycleBinding validate{&record, "validate"};
    LifecycleBinding rejected{&record, "rejected"};
    LifecycleBinding accepted{&record, "accepted"};
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a", 1, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "validate", lifecycle_handler, &validate), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "rejected", lifecycle_handler, &rejected), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "accepted", lifecycle_handler, &accepted), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_EQ(record.entries.size(), 2u);
    EXPECT_EQ(record.entries[0], "validate:0");
    EXPECT_EQ(record.entries[1], "accepted:1");
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, queues_recovery_before_commit_for_missing_nodes) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"token":"B","allowASI":true,"events":{
        "onRecovery":"recovery","onCommit":"commit"
      }}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    LifecycleRecord record;
    LifecycleBinding recovery{&record, "recovery"};
    LifecycleBinding commit{&record, "commit"};
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("", 0, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "recovery", lifecycle_handler, &recovery), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "commit", lifecycle_handler, &commit), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_EQ(record.entries.size(), 2u);
    EXPECT_EQ(record.entries[0], "recovery:2");
    EXPECT_EQ(record.entries[1], "commit:1");
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, does_not_publish_source_complete_with_unconsumed_tokens) {
    const std::string grammar = R"json({
      "start":"Root",
      "events":{"onSourceComplete":"source"},
      "productions":{"Root":{"token":"A","events":{"onCommit":"commit"}}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    LifecycleRecord record;
    LifecycleBinding commit{&record, "commit"};
    LifecycleBinding source{&record, "source"};
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a b", 3, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "commit", lifecycle_handler, &commit), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "source", lifecycle_handler, &source), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_EQ(record.entries.size(), 1u);
    EXPECT_EQ(record.entries[0], "commit:1");
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, commit_rejection_stops_publication_and_clears_the_queue) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"token":"A","events":{"onCommit":"commit"}}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    LifecycleRecord record;
    record.reject_commit = true;
    LifecycleBinding commit{&record, "commit"};
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a", 1, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(textparser_register_handler(parser.get(), "commit", lifecycle_handler, &commit), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_ERROR);
    ASSERT_EQ(record.entries.size(), 1u);
    textparser_parser_state_view state{};
    ASSERT_EQ(parser.parser_state(&state), 0);
    EXPECT_EQ(state.pending_event_count, 0u);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, rejects_nullable_repeat_and_left_recursion) {
    const char *nullable_repeat = R"({
      "start":"Root","productions":{
        "Root":{"repeat":{"optional":{"token":"A"}}}
      }
    })";
    textparser_language_definition *definition = nullptr;
    EXPECT_EQ(load(nullable_repeat, &definition), TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT);
    EXPECT_EQ(definition, nullptr);

    const char *left_recursive = R"({
      "start":"Root","productions":{
        "Root":{"choice":[{"ref":"Root"},{"token":"A"}]}
      }
    })";
    EXPECT_EQ(load(left_recursive, &definition), TEXTPARSER_JSON_GRAMMAR_LEFT_RECURSION);
    EXPECT_EQ(definition, nullptr);
}

TEST(json_grammar, accepts_recursion_after_consumption) {
    const char *right_recursive = R"({
      "start":"Root","productions":{
        "Root":{"choice":[
          {"token":"A"},
          {"sequence":[{"token":"A"},{"ref":"Root"}]}
        ]}
      }
    })";
    textparser_language_definition *definition = nullptr;
    EXPECT_EQ(load(right_recursive, &definition), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(definition, nullptr);
    textparser_free_language_definition(definition);
}

TEST(json_grammar, language_executor_rejects_missing_grammar) {
    textparser_language_definition *definition = nullptr;
    const std::string json = language_with_grammar("null");
    EXPECT_EQ(textparser_json_load_language_definition_from_string(json.c_str(), &definition),
              TEXTPARSER_JSON_GRAMMAR_NOT_OBJECT);
    EXPECT_EQ(definition, nullptr);

    textparser_match_result result{};
    EXPECT_EQ(textparser_execute_language_grammar(nullptr, nullptr, &result), -1);
}

namespace {
bool json_context_predicate(textparser_t parser,
                            const textparser_predicate_context *,
                            void *) {
    int64_t first = 0;
    int64_t second = 0;
    return textparser_context_get(parser, "First", &first) == 0 && first == 1 &&
           textparser_context_get(parser, "Second", &second) == 0 && second == 2;
}
} // namespace

TEST(json_grammar, loads_lookahead_predicates_contexts_and_commit) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{
        "Root":{"sequence":[
          {"lookahead":{"token":"A"}},
          {"not":{"token":"B"}},
          {"token":"A"},
          {"withContext":{"set":{"First":1,"Second":2},"sequence":[
            {"when":{"native":"test.context"}},
            {"commit":true},
            {"token":"B"}
          ]}}
        ]}
      }
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("a b", 3, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(textparser_register_parser_predicate(
                  parser.get(), "test.context", json_context_predicate, nullptr), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.consumed_tokens, 2u);
    EXPECT_TRUE(result.committed);
    int64_t value = 0;
    EXPECT_NE(textparser_context_get(parser.get(), "First", &value), 0);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, validates_advanced_construct_shapes_and_nullable_repeat) {
    struct Case { const char *grammar; int expected; } cases[] = {
        {R"({"start":"Root","productions":{"Root":{"lookahead":[]}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"commit":false}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"when":{"native":""}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"withContext":{"set":{},"ref":"Root"}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"withContext":{"set":{"X":"yes"},"sequence":[]}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"repeat":{"lookahead":{"token":"A"}}}}})",
         TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT},
        {R"({"start":"Root","productions":{"Root":{"pratt":{}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"pratt":{"primary":{"token":"A"},"minimumPrecedence":"high"}}}})",
         TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"repeat":{"pratt":{"primary":{"optional":{"token":"A"}}}}}}})",
         TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT},
    };
    for (const auto &item : cases) {
        textparser_language_definition *definition = nullptr;
        EXPECT_EQ(load(item.grammar, &definition), item.expected) << item.grammar;
        EXPECT_EQ(definition, nullptr);
    }
}

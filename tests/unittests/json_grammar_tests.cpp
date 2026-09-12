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

TEST(json_grammar, supported_bom_comma_separated_strings) {
    const struct {
        const char *value;
        int mask;
    } cases[] = {
        {"", 0},
        {",,,", 0},
        {" \t ", 0},
        {", \t ,,,", 0},
        {"unknown", 0},
        {"utf-8", TEXTPARSER_BOM_UTF_8},
        {", UTF-8 ,\tutf-16-LE\t,,unknown,utf-8, ",
         TEXTPARSER_BOM_UTF_8 | TEXTPARSER_BOM_UTF_16_LE},
        {"utf-8,utf-16-be,utf-16-le,utf-32-be,utf-32-le",
         TEXTPARSER_BOM_UTF_8 | TEXTPARSER_BOM_UTF_16_BE |
         TEXTPARSER_BOM_UTF_16_LE | TEXTPARSER_BOM_UTF_32_BE |
         TEXTPARSER_BOM_UTF_32_LE},
    };
    for (const char *key : {"supportedBom", "SupportedBom"}) {
        for (const auto &entry : cases) {
            SCOPED_TRACE(std::string(key) + ": " + entry.value);
            std::string escaped;
            for (char c : std::string(entry.value))
                escaped += c == '\t' ? "\\t" : std::string(1, c);
            std::string json = language_with_grammar(
                R"({"start":"Root","productions":{"Root":{"token":"A"}}})");
            json.insert(1, std::string("\"") + key + "\":\"" + escaped + "\",");
            textparser_language_definition *definition = nullptr;
            ASSERT_EQ(textparser_json_load_language_definition_from_string(
                          json.c_str(), &definition), TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(definition, nullptr);
            EXPECT_EQ(definition->supported_bom, entry.mask);
            textparser_free_language_definition(definition);
        }
    }
}

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
        {R"({"start":"Root","productions":{"Root":{"withGoal":{"production":{"token":"A"}}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"withGoal":{"name":"","production":{"token":"A"}}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"withGoal":{"name":"Type","production":[]}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"pratt":{"primary":{"token":"A"},"postfix":[]}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"capture":{"name":"","production":{"token":"A"},"then":{"token":"B"}}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"capture":{"name":"x","production":{"token":"A"}}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
        {R"({"start":"Root","productions":{"Root":{"matchCapture":{"name":"x","production":[]}}}})", TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION},
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

TEST(json_grammar, validates_pratt_operand_validator_shapes) {
    for (const char *operators : {
             R"([{"token":"A","role":"infix","precedence":1,"leftValidator":true}])",
             R"([{"token":"A","role":"prefix","precedence":1,"operandValidator":[]}])",
         }) {
        std::string json = R"({
          "name":"validators", "version":2, "caseSensitivity":true,
          "defaultFileExtensions":["txt"], "defaultTextEncoding":"utf-8",
          "otherTextInside":true,
          "lexer":{"tokens":{"A":{"regex":"a"}},"modes":{"default":{"tokens":["A"]}}},
          "operators":)" + std::string(operators) + R"(,
          "grammar":{"start":"Root","productions":{"Root":{"token":"A"}}}
        })";
        textparser_language_definition *definition = nullptr;
        EXPECT_EQ(textparser_json_load_language_definition_from_string(json.c_str(), &definition),
                  TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION);
        EXPECT_EQ(definition, nullptr);
    }
}

TEST(json_grammar, rejects_nullable_pratt_postfix_production) {
    textparser_language_definition *definition = nullptr;
    EXPECT_EQ(load(R"({
      "start":"Root",
      "productions":{"Root":{"pratt":{
        "primary":{"token":"A"}, "postfix":{"optional":{"token":"B"}}
      }}}
    })", &definition), TEXTPARSER_JSON_GRAMMAR_NULLABLE_REPEAT);
    EXPECT_EQ(definition, nullptr);
}

TEST(json_grammar, capture_equality_is_nested_and_transactional) {
    const std::string grammar = R"json({
      "start":"Root",
      "productions":{"Root":{"capture":{
        "name":"pair", "production":{"token":"A"},
        "then":{"sequence":[
          {"capture":{
            "name":"pair", "production":{"token":"B"},
            "then":{"matchCapture":{"name":"pair","production":{"choice":[{"token":"A"},{"token":"B"}]}}}
          }},
          {"matchCapture":{"name":"pair","production":{"choice":[{"token":"A"},{"token":"B"}]}}}
        ]}
      }}}
    })json";
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
    textparser::Parser parser;
    for (const char *source : {"abba", "abaa"}) {
        ASSERT_EQ(parser.openmem(source, 4, TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(definition), 0);
        textparser_match_result result{};
        ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
        if (std::strcmp(source, "abba") == 0) {
            EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
            EXPECT_EQ(result.consumed_tokens, 4u);
        } else {
            EXPECT_EQ(result.status, TEXTPARSER_MATCH_NO);
            textparser_parser_state_view state{};
            ASSERT_EQ(parser.parser_state(&state), 0);
            EXPECT_EQ(state.source_offset, 0u);
        }
        parser.reset();
    }
    textparser_free_language_definition(definition);
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
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);
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

TEST(json_grammar, category_metadata_is_language_and_kind_independent) {
    const char *names[] = {"unknown", "token", "source_file", "declaration", "statement",
                           "expression", "type", "jsx", "pattern", "other"};
    for (int i = 0; i < 10; ++i) {
        SCOPED_TRACE(names[i]);
        textparser_language_definition *definition = nullptr;
        std::string grammar = R"({"start":"UnrelatedName","productions":{"UnrelatedName":{
            "sequence":[{"token":"A"},{"token":"B"}],"category":")";
        grammar += names[i];
        grammar += R"("}}})";
        ASSERT_EQ(load(grammar, &definition), TEXTPARSER_JSON_NO_ERROR);
        EXPECT_EQ(definition->grammar->productions[0].category, i);
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem("ab", 2, TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(definition), 0);
        textparser_match_result result{};
        ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
        ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
        ASSERT_NE(result.node, nullptr);
        EXPECT_EQ(textparser_node_get_category(result.node),
                  i == 0 ? TEXTPARSER_CST_OTHER : i);
        EXPECT_EQ(textparser_node_get_category(result.node->child), TEXTPARSER_CST_TOKEN);
        parser.reset();
        textparser_free_language_definition(definition);
    }
}

TEST(json_grammar, categories_follow_emitted_nodes_and_choice_renaming) {
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(R"({"start":"Root","productions":{
        "Root":{"ref":"Selected","category":"statement"},
        "Selected":{"choice":[{"sequence":[{"token":"A"},
            {"sequence":[{"token":"B"}],"category":"type"}],"category":"other"}],
            "category":"declaration"}
    }})", &definition), TEXTPARSER_JSON_NO_ERROR);
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("ab", 2, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_NE(result.node, nullptr);
    EXPECT_STREQ(result.node->cst_kind, "Selected");
    EXPECT_EQ(textparser_node_get_category(result.node), TEXTPARSER_CST_DECLARATION);
    ASSERT_NE(result.node->child, nullptr);
    ASSERT_NE(result.node->child->next, nullptr);
    EXPECT_EQ(textparser_node_get_category(result.node->child->next), TEXTPARSER_CST_TYPE);
    parser.reset();
    textparser_free_language_definition(definition);
}

TEST(json_grammar, rejects_invalid_category_metadata) {
    for (const char *value : {"null", "false", "1", "[]", "{}", "\"\"",
                              "\"Declaration\"", "\"custom\"", "\"type\\u0000suffix\""}) {
        SCOPED_TRACE(value);
        textparser_language_definition *definition = nullptr;
        std::string grammar = R"({"start":"Root","productions":{"Root":{
            "sequence":[{"token":"A"}],"category":)";
        grammar += value;
        grammar += "}}}";
        EXPECT_EQ(load(grammar, &definition), TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION);
        if (definition) textparser_free_language_definition(definition);
    }
}

TEST(json_grammar, category_fallbacks_do_not_infer_from_kind_names) {
    EXPECT_EQ(textparser_node_get_category(nullptr), TEXTPARSER_CST_UNKNOWN);
    textparser_node node{}, child{};
    node.cst_kind = "SourceFile";
    EXPECT_EQ(textparser_node_get_category(&node), TEXTPARSER_CST_TOKEN);
    node.node_flags = TEXTPARSER_NODE_MISSING;
    EXPECT_EQ(textparser_node_get_category(&node), TEXTPARSER_CST_OTHER);
    node.child = &child;
    node.node_flags = TEXTPARSER_NODE_SYNTHETIC;
    EXPECT_EQ(textparser_node_get_category(&node), TEXTPARSER_CST_OTHER);
    node.node_flags = 0;
    EXPECT_EQ(textparser_node_get_category(&node), TEXTPARSER_CST_EXPRESSION);
    node.category = TEXTPARSER_CST_PATTERN;
    EXPECT_EQ(textparser_node_get_category(&node), TEXTPARSER_CST_PATTERN);
}

namespace {
std::string guarded_language(const std::string &guard,
                             const std::string &profiles = R"({"jsx":[".tsx",".jsx"],"script":[".js",".mjs",".cjs"],"declaration":[".d.ts"]})") {
    return R"({"formatVersion":2,"name":"generic_guards","version":2,
      "caseSensitivity":true,"defaultFileExtensions":[],"defaultTextEncoding":"utf-8",
      "otherTextInside":false,
      "lexer":{"initialMode":"default",
        "tokens":{"Word":{"regex":"[a-zA-Z]+"},"Mark":{"regex":";"}},
        "trivia":{"Space":{"regex":"[ \\t\\r\\n]+","detectLineTerminators":true},
                  "Comment":{"regex":"/\\*[^*]*\\*/","detectLineTerminators":true}},
        "modes":{"default":{"tokens":["Word","Mark"],"trivia":["Space","Comment"]}}},
      "grammar":{"start":"Root","sourceFileKinds":)" + profiles +
      R"(,"productions":{"Root":{"when":)" + guard + "}}}}";
}

void check_guard(const std::string &guard, const char *source, bool matches,
                 const char *filename = nullptr) {
    SCOPED_TRACE(guard + " source=" + source + " filename=" + (filename ? filename : "<none>"));
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(
        guarded_language(guard).c_str(), &definition), TEXTPARSER_JSON_NO_ERROR);
    {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem(source, std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        textparser_set_filename(parser.get(), filename);
        ASSERT_EQ(parser.parse(definition), 0);
        textparser_match_result result{};
        ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
        EXPECT_EQ(result.status, matches ? TEXTPARSER_MATCH_OK : TEXTPARSER_MATCH_NO);
        EXPECT_EQ(result.consumed_tokens, 0u);
        EXPECT_EQ(result.node, nullptr);
    }
    textparser_free_language_definition(definition);
}
} // namespace

TEST(json_grammar, generic_newline_guards) {
    for (const char *source : {"word", " word", "/*comment*/word", "", " \n", "/*\n*/"}) {
        check_guard(R"({"noLineTerminatorBefore":true})", source, true);
        check_guard(R"({"lineTerminatorBefore":true})", source, false);
    }
    for (const char *source : {"\nword", "\rword", "\r\nword", "/*\n*/word"}) {
        check_guard(R"({"noLineTerminatorBefore":true})", source, false);
        check_guard(R"({"lineTerminatorBefore":true})", source, true);
        check_guard(R"({"noLineTerminatorBefore":false})", source, true);
        check_guard(R"({"lineTerminatorBefore":false})", source, false);
    }
}

TEST(json_grammar, generic_next_token_guards_and_eof) {
    check_guard(R"({"nextTokenIn":["Word","Mark"]})", "word", true);
    check_guard(R"({"nextTokenIn":["Word","Mark"]})", ";", true);
    check_guard(R"({"nextTokenIn":["Word"]})", ";", false);
    check_guard(R"({"nextToken":"Word"})", "word", true);
    check_guard(R"({"nextToken":"Word"})", "", false);
    check_guard(R"({"nextTokenIn":["Word"],"allowEOF":true})", "", true);
    check_guard(R"({"nextTokenIn":["Word"],"allowEOF":false})", "", false);
    check_guard(R"({"nextTokenIn":["Word"],"allowEOF":true})", ";", false);
    check_guard(R"({"nextTokenText":"meta"})", "meta", true);
    check_guard(R"({"nextTokenText":"meta"})", "Meta", false);
    check_guard(R"({"nextTokenText":"meta"})", "metadata", false);
    check_guard(R"({"nextTokenText":"meta"})", "", false);
    check_guard(R"({"nextToken":"Word","nextTokenText":"meta","noLineTerminatorBefore":true})", "meta", true);
    check_guard(R"({"nextToken":"Mark","nextTokenText":"meta"})", "meta", false);
    check_guard(R"({"nextToken":"Word","nextTokenText":"meta","noLineTerminatorBefore":true})", "\nmeta", false);
    check_guard(R"({"nextToken":"Word","allowEOF":true,"nextTokenText":"meta"})", "", false);
}

TEST(json_grammar, generic_filename_profiles) {
    for (const char *name : {"component.tsx", "COMPONENT.TSX", "component.jsx", "dir.with.dots/file.JSX"})
        check_guard(R"({"sourceFileKind":"jsx"})", "", true, name);
    for (const char *name : {"", "component.ts", "component.tsx.bak", "dir.tsx/file", "tsx"})
        check_guard(R"({"sourceFileKind":"jsx"})", "", false, name);
    check_guard(R"({"sourceFileKind":"jsx"})", "", false);
    check_guard(R"({"sourceFileKind":"declaration"})", "", true, "API.D.TS");
    check_guard(R"({"sourceFileKind":"declaration"})", "", false, "api.ts");
    check_guard(R"({"sourceFileKind":"jsx","nextToken":"Word"})", "word", true, "view.tsx");
    check_guard(R"({"sourceFileKind":"jsx","nextToken":"Mark"})", "word", false, "view.tsx");
}

TEST(json_grammar, generic_guards_reject_invalid_configuration) {
    for (const char *guard : {"{}", "null", "[]", R"({"allowEOF":true})",
         R"({"noLineTerminatorBefore":1})", R"({"lineTerminatorBefore":null})",
         R"({"lineTerminatorBefore":true,"noLineTerminatorBefore":false})",
         R"({"nextTokenIn":[]})", R"({"nextTokenIn":"Word"})", R"({"nextTokenIn":[null]})",
         R"({"nextToken":"Word","nextTokenIn":["Mark"]})",
         R"({"nextToken":"Word","allowEOF":1})", R"({"nextTokenText":""})",
         R"({"nextTokenText":"meta\u0000suffix"})", R"({"sourceFileKind":"missing"})",
         R"({"sourceFileKind":true})", R"({"unexpected":true})",
         R"({"native":"callback","nextToken":"Word"})"}) {
        SCOPED_TRACE(guard);
        textparser_language_definition *definition = nullptr;
        EXPECT_EQ(textparser_json_load_language_definition_from_string(
            guarded_language(guard).c_str(), &definition), TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION);
        if (definition) textparser_free_language_definition(definition);
    }
    textparser_language_definition *definition = nullptr;
    EXPECT_EQ(textparser_json_load_language_definition_from_string(
        guarded_language(R"({"nextTokenIn":["Undefined"]})").c_str(), &definition),
        TEXTPARSER_JSON_GRAMMAR_UNDEFINED_TOKEN);
    if (definition) textparser_free_language_definition(definition);
    for (const char *profiles : {"null", "[]", R"({"jsx":[]})", R"({"jsx":[null]})",
                                R"({"jsx":["tsx"]})", R"({"jsx":["."]})",
                                R"({"jsx":[".tsx/file"]})", R"({"":[".tsx"]})"}) {
        SCOPED_TRACE(profiles);
        definition = nullptr;
        EXPECT_EQ(textparser_json_load_language_definition_from_string(
            guarded_language(R"({"nextToken":"Word"})", profiles).c_str(), &definition),
            TEXTPARSER_JSON_GRAMMAR_INVALID_PRODUCTION);
        if (definition) textparser_free_language_definition(definition);
    }
}

TEST(json_grammar, generic_guards_support_legacy_token_streams) {
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(R"({"start":"Root","productions":{"Root":{"sequence":[
        {"when":{"nextToken":"A","nextTokenText":"a"}},{"token":"A"},
        {"not":{"when":{"nextToken":"A"}}},{"token":"B"}
    ]}}})", &definition), TEXTPARSER_JSON_NO_ERROR);
    {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem("ab", 2, TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(definition), 0);
        textparser_match_result result{};
        ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(result.consumed_tokens, 2u);
    }
    textparser_free_language_definition(definition);
}

TEST(json_grammar, generic_token_text_matches_wide_source_encodings) {
    const char16_t source16[] = u" meta";
    const char32_t source32[] = U" meta";
    for (bool wide32 : {false, true}) {
        SCOPED_TRACE(wide32);
        textparser_language_definition *definition = nullptr;
        ASSERT_EQ(textparser_json_load_language_definition_from_string(
            guarded_language(R"({"nextTokenText":"meta"})").c_str(), &definition), TEXTPARSER_JSON_NO_ERROR);
        {
            textparser::Parser parser;
            const char *bytes = wide32 ? reinterpret_cast<const char *>(source32) : reinterpret_cast<const char *>(source16);
            size_t length = wide32 ? sizeof(source32) - sizeof(char32_t) : sizeof(source16) - sizeof(char16_t);
            ASSERT_EQ(parser.openmem(bytes, length, wide32 ? TEXTPARSER_ENCODING_UTF_32 : TEXTPARSER_ENCODING_UTF_16), 0);
            ASSERT_EQ(parser.parse(definition), 0);
            textparser_match_result result{};
            ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
            EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
            EXPECT_EQ(result.consumed_tokens, 0u);
        }
        textparser_free_language_definition(definition);
    }
}

TEST(json_grammar, generic_guards_preserve_choice_rollback) {
    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(load(R"({"start":"Root","productions":{"Root":{"choice":[
        {"sequence":[{"token":"A"},{"when":{"nextToken":"C"}},{"token":"C"}]},
        {"sequence":[{"when":{"nextTokenText":"a"}},{"token":"A"},{"token":"B"}]}
    ]}}})", &definition), TEXTPARSER_JSON_NO_ERROR);
    {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem("ab", 2, TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(definition), 0);
        textparser_match_result result{};
        ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(result.consumed_tokens, 2u);
    }
    textparser_free_language_definition(definition);
}

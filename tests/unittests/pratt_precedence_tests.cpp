#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>

TEST(pratt_precedence, register_and_query_operators) {
    textparser_t handle = nullptr;
    const char *code = "{}";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    // Register binary addition
    textparser_operator_def add_op{};
    add_op.token_id = 100;
    add_op.role = TEXTPARSER_OP_INFIX;
    add_op.precedence = 10;
    add_op.associativity = TEXTPARSER_ASSOC_LEFT;
    add_op.secondary_token_id = 0;

    EXPECT_EQ(textparser_register_operator(handle, &add_op), 0);

    // Register unary negation
    textparser_operator_def neg_op{};
    neg_op.token_id = 101;
    neg_op.role = TEXTPARSER_OP_PREFIX;
    neg_op.precedence = 15;
    neg_op.associativity = TEXTPARSER_ASSOC_RIGHT;
    neg_op.secondary_token_id = 0;

    EXPECT_EQ(textparser_register_operator(handle, &neg_op), 0);

    // Register ternary conditional
    textparser_operator_def cond_op{};
    cond_op.token_id = 102;
    cond_op.role = TEXTPARSER_OP_TERNARY;
    cond_op.precedence = 4;
    cond_op.associativity = TEXTPARSER_ASSOC_RIGHT;
    cond_op.secondary_token_id = 103;

    EXPECT_EQ(textparser_register_operator(handle, &cond_op), 0);

    // Query operators
    textparser_operator_def query_op{};
    EXPECT_EQ(textparser_get_operator(handle, 100, TEXTPARSER_OP_INFIX, &query_op), 0);
    EXPECT_EQ(query_op.precedence, 10);
    EXPECT_EQ(query_op.associativity, TEXTPARSER_ASSOC_LEFT);
    EXPECT_EQ(query_op.left_validator, nullptr);
    EXPECT_EQ(query_op.operand_validator, nullptr);

    EXPECT_EQ(textparser_get_operator(handle, 101, TEXTPARSER_OP_PREFIX, &query_op), 0);
    EXPECT_EQ(query_op.precedence, 15);
    EXPECT_EQ(query_op.associativity, TEXTPARSER_ASSOC_RIGHT);

    EXPECT_EQ(textparser_get_operator(handle, 102, TEXTPARSER_OP_TERNARY, &query_op), 0);
    EXPECT_EQ(query_op.precedence, 4);
    EXPECT_EQ(query_op.secondary_token_id, 103);

    // Query non-existent operator
    EXPECT_NE(textparser_get_operator(handle, 999, -1, &query_op), 0);

    textparser_close(handle);
}

namespace {
const char *pratt_language_json = R"json({
  "name":"pratt", "version":2, "caseSensitivity":true,
  "defaultFileExtensions":["txt"], "defaultTextEncoding":"utf-8",
  "otherTextInside":true,
  "lexer":{
    "tokens":{
      "Number":{"regex":"[0-9]+"},
      "Plus":{"regex":"\\+"},
      "Star":{"regex":"\\*"},
      "Minus":{"regex":"-"},
      "Bang":{"regex":"!"},
      "Question":{"regex":"\\?"},
      "Colon":{"regex":":"},
      "Equal":{"regex":"="},
      "LParen":{"regex":"\\("},
      "RParen":{"regex":"\\)"}
      ,"Name":{"regex":"[a-z]+"}
      ,"Dot":{"regex":"\\."}
    },
    "trivia":{"Space":{"regex":"[ \\t\\r\\n]+"}}
  },
  "operators":[
    {"token":"Plus","role":"infix","precedence":10,"associativity":"left"},
    {"token":"Star","role":"infix","precedence":20,"associativity":"left"},
    {"token":"Minus","roles":["prefix","infix"],"prefixPrecedence":30,"infixPrecedence":10,"associativity":"left"},
    {"token":"Bang","role":"postfix","precedence":40},
    {"token":"Equal","role":"infix","precedence":2,"associativity":"right"},
    {"token":"Question","role":"ternary","precedence":5,"associativity":"right","middleTerminator":"Colon"}
  ],
  "grammar":{
    "start":"Expression",
    "productions":{
      "Expression":{"pratt":{
        "primary":{"ref":"Primary"},
        "postfix":{"sequence":[{"token":"Dot"},{"token":"Name"}]}
      }},
      "Primary":{"choice":[
        {"token":"Number"},
        {"token":"Name"},
        {"sequence":[{"token":"LParen"},{"ref":"Expression"},{"token":"RParen"}]}
      ]}
    }
  }
})json";

int pratt_token_id(const textparser_language_definition *definition, const char *name) {
    for (int i = 0; definition->tokens[i].name != nullptr; i++)
        if (strcmp(definition->tokens[i].name, name) == 0) return i;
    return -1;
}

struct PrattFixture : testing::Test {
    textparser_language_definition *definition = nullptr;
    void SetUp() override {
        ASSERT_EQ(textparser_json_load_language_definition_from_string(pratt_language_json, &definition), 0);
    }
    void TearDown() override { textparser_free_language_definition(definition); }
};
} // namespace

TEST_F(PrattFixture, multiplication_binds_more_tightly_than_addition) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("1+2*3", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_NE(result.node, nullptr);
    EXPECT_EQ(result.node->token_id, pratt_token_id(definition, "Plus"));
    ASSERT_NE(result.node->child, nullptr);
    ASSERT_NE(result.node->child->next, nullptr);
    EXPECT_EQ(result.node->child->next->token_id, pratt_token_id(definition, "Star"));
}

TEST_F(PrattFixture, primary_grammar_supports_parenthesized_expressions) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("1*(2+3)", 7, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.node->token_id, pratt_token_id(definition, "Star"));
    textparser_node *parenthesized = result.node->child->next;
    ASSERT_NE(parenthesized, nullptr);
    ASSERT_NE(parenthesized->child, nullptr);
    ASSERT_NE(parenthesized->child->next, nullptr);
    EXPECT_EQ(parenthesized->child->next->token_id, pratt_token_id(definition, "Plus"));
}

TEST_F(PrattFixture, grammar_postfix_productions_bind_before_infix_operators) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("value.member+2", 14, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_NE(result.node, nullptr);
    EXPECT_EQ(result.node->token_id, pratt_token_id(definition, "Plus"));
    ASSERT_NE(result.node->child, nullptr);
    EXPECT_NE(result.node->child->node_flags & TEXTPARSER_NODE_SYNTHETIC, 0u);
    EXPECT_EQ(result.node->child->len, 12u);
}

TEST_F(PrattFixture, left_associative_infix_groups_left) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("1-2-3", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.node->token_id, pratt_token_id(definition, "Minus"));
    ASSERT_NE(result.node->child, nullptr);
    EXPECT_EQ(result.node->child->token_id, pratt_token_id(definition, "Minus"));
}

TEST_F(PrattFixture, right_associative_infix_groups_right) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("1=2=3", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.node->token_id, pratt_token_id(definition, "Equal"));
    ASSERT_NE(result.node->child, nullptr);
    ASSERT_NE(result.node->child->next, nullptr);
    EXPECT_EQ(result.node->child->next->token_id, pratt_token_id(definition, "Equal"));
}

TEST_F(PrattFixture, prefix_and_postfix_roles_are_contextual) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("-1!", 3, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_node *root = nullptr;
    ASSERT_EQ(textparser_parse_pratt_expression(parser.get(), 0, &root), 0);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->token_id, pratt_token_id(definition, "Minus"));
    ASSERT_NE(root->child, nullptr);
    EXPECT_EQ(root->child->token_id, pratt_token_id(definition, "Bang"));
}

TEST_F(PrattFixture, ternary_is_right_associative) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("1?2:3?4:5", 9, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    textparser_match_result result{};
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(result.node->token_id, pratt_token_id(definition, "Question"));
    ASSERT_NE(result.node->child, nullptr);
    ASSERT_NE(result.node->child->next, nullptr);
    ASSERT_NE(result.node->child->next->next, nullptr);
    EXPECT_EQ(result.node->child->next->next->token_id, pratt_token_id(definition, "Question"));
}

TEST_F(PrattFixture, missing_operand_or_ternary_separator_is_error) {
    for (const char *source : {"1+", "1?2"}) {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem(source, (int)strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(definition), 0);
        textparser_match_result result{};
        ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_ERROR) << source;
        textparser_parser_state_view state{};
        ASSERT_EQ(parser.parser_state(&state), 0);
        EXPECT_EQ(state.token_index, 0u);
    }
}

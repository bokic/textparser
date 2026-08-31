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
    textparser_operator_def add_op;
    add_op.token_id = 100;
    add_op.role = TEXTPARSER_OP_INFIX;
    add_op.precedence = 10;
    add_op.associativity = TEXTPARSER_ASSOC_LEFT;
    add_op.secondary_token_id = 0;

    EXPECT_EQ(textparser_register_operator(handle, &add_op), 0);

    // Register unary negation
    textparser_operator_def neg_op;
    neg_op.token_id = 101;
    neg_op.role = TEXTPARSER_OP_PREFIX;
    neg_op.precedence = 15;
    neg_op.associativity = TEXTPARSER_ASSOC_RIGHT;
    neg_op.secondary_token_id = 0;

    EXPECT_EQ(textparser_register_operator(handle, &neg_op), 0);

    // Register ternary conditional
    textparser_operator_def cond_op;
    cond_op.token_id = 102;
    cond_op.role = TEXTPARSER_OP_TERNARY;
    cond_op.precedence = 4;
    cond_op.associativity = TEXTPARSER_ASSOC_RIGHT;
    cond_op.secondary_token_id = 103;

    EXPECT_EQ(textparser_register_operator(handle, &cond_op), 0);

    // Query operators
    textparser_operator_def query_op;
    EXPECT_EQ(textparser_get_operator(handle, 100, TEXTPARSER_OP_INFIX, &query_op), 0);
    EXPECT_EQ(query_op.precedence, 10);
    EXPECT_EQ(query_op.associativity, TEXTPARSER_ASSOC_LEFT);

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

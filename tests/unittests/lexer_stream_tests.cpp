#include <gtest/gtest.h>
#include <textparser.hpp>
#include <c_definition.json.h>
#include <cstring>

TEST(lexer_streams, separates_tokens_and_leading_trivia) {
    textparser::Parser parser;
    const char *code = "  int\r\n value";
    ASSERT_EQ(parser.openmem(code, (int)std::strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t token_count = 0;
    const textparser_lex_token *tokens = parser.lexer_tokens(&token_count);
    ASSERT_NE(tokens, nullptr);
    ASSERT_GE(token_count, 2u);

    size_t trivia_count = 0;
    const textparser_lex_trivia *trivia = parser.lexer_trivia(&trivia_count);
    ASSERT_NE(trivia, nullptr);
    ASSERT_EQ(trivia_count, 2u);
    EXPECT_EQ(trivia[0].start, 0u);
    EXPECT_EQ(trivia[0].end, 2u);
    EXPECT_EQ(trivia[0].flags, 0u);
    EXPECT_EQ(tokens[0].start, 2u);
    EXPECT_EQ(tokens[0].leading_trivia_start, 0u);
    EXPECT_EQ(tokens[0].leading_trivia_count, 1u);
    EXPECT_EQ(trivia[1].start, 5u);
    EXPECT_EQ(trivia[1].end, 8u);
    EXPECT_NE(trivia[1].flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR, 0u);
    EXPECT_EQ(tokens[1].start, 8u);
    EXPECT_EQ(tokens[1].leading_trivia_start, 1u);
    EXPECT_EQ(tokens[1].leading_trivia_count, 1u);
    EXPECT_NE(tokens[1].flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR, 0u);
}

TEST(lexer_streams, preserves_delimiters_and_trailing_trivia) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("\"\"  ", 4, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t token_count = 0;
    const textparser_lex_token *tokens = parser.lexer_tokens(&token_count);
    ASSERT_NE(tokens, nullptr);
    ASSERT_GE(token_count, 2u);
    EXPECT_EQ(tokens[0].kind, TEXTPARSER_TOKEN_ID_START_DELIMITER);
    EXPECT_EQ(tokens[1].kind, TEXTPARSER_TOKEN_ID_END_DELIMITER);

    size_t trivia_count = 0;
    const textparser_lex_trivia *trivia = parser.lexer_trivia(&trivia_count);
    ASSERT_NE(trivia, nullptr);
    ASSERT_EQ(trivia_count, 1u);
    EXPECT_EQ(trivia[0].start, 2u);
    EXPECT_EQ(trivia[0].end, 4u);
}

TEST(lexer_streams, empty_parse_and_invalid_queries) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("", 0, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t count = 99;
    EXPECT_EQ(parser.lexer_tokens(&count), nullptr);
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(parser.lexer_trivia(&count), nullptr);
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(textparser_get_lexer_tokens(parser.get(), nullptr), nullptr);
    EXPECT_EQ(textparser_get_lexer_trivia(nullptr, &count), nullptr);
    EXPECT_EQ(count, 0u);
}

TEST(lexer_streams, incremental_parse_replaces_snapshot) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("int a", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);
    size_t before_count = 0;
    ASSERT_NE(parser.lexer_tokens(&before_count), nullptr);

    textparser_dirty_range dirty{};
    ASSERT_EQ(parser.parse_incremental(&c_definition, 4, 1, "long_name", 9, &dirty), 0);
    size_t after_count = 0;
    const textparser_lex_token *after = parser.lexer_tokens(&after_count);
    ASSERT_NE(after, nullptr);
    EXPECT_GT(after_count, 0u);
    EXPECT_EQ(after[after_count - 1].end, 13u);
}

TEST(lexer_streams, set_text_invalidates_snapshot) {
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem("int a", 5, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);
    size_t count = 0;
    ASSERT_NE(parser.lexer_tokens(&count), nullptr);

    ASSERT_EQ(parser.set_text("int b"), 0);
    EXPECT_EQ(parser.lexer_tokens(&count), nullptr);
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(parser.lexer_trivia(&count), nullptr);
    EXPECT_EQ(count, 0u);
}

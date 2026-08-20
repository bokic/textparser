#include <gtest/gtest.h>
#include <textparser.hpp>
#include <cfml_definition.json.h>
#include <html_definition.json.h>

TEST(IncrementalParsing, StateGenerateBasic) {
    const char *code = "<cfset a = 1><cfset b = 2><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    // State at offset 0
    textparser_parser_state *st0 = textparser_state_generate(handle, 0);
    ASSERT_NE(st0, nullptr);
    EXPECT_EQ(st0->len, 0u);
    textparser_state_free(st0);

    // State in the middle of first token (offset 5)
    textparser_parser_state *st5 = textparser_state_generate(handle, 5);
    ASSERT_NE(st5, nullptr);
    EXPECT_EQ(st5->len, 5u);
    EXPECT_NE(st5->state[4], nullptr);
    textparser_state_free(st5);

    // State at offset 13 (boundary between token 1 and token 2)
    textparser_parser_state *st13 = textparser_state_generate(handle, 13);
    ASSERT_NE(st13, nullptr);
    EXPECT_EQ(st13->len, 13u);
    EXPECT_NE(st13->state[12], nullptr);
    textparser_state_free(st13);

    // State at EOF
    textparser_parser_state *stEOF = textparser_state_generate(handle, strlen(code));
    ASSERT_NE(stEOF, nullptr);
    EXPECT_EQ(stEOF->len, strlen(code));
    textparser_state_free(stEOF);

    textparser_close(handle);
}

TEST(IncrementalParsing, CppStateGenerateAndSetText) {
    const char *code1 = "<cfset a = 1><cfset b = 2>";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code1, (int)strlen(code1), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&cfml_definition), 0);

    auto state = textparser::State::generate(parser.get(), 13);
    ASSERT_TRUE(state);
    EXPECT_EQ(state.get()->len, 13u);

    // Update text
    const char *code2 = "<cfset a = 1><cfset b = 200>";
    ASSERT_EQ(parser.set_text(code2, (int)strlen(code2)), 0);
    ASSERT_EQ(parser.parse_incremental(&cfml_definition, state, 13, strlen(code2)), 0);

    textparser_token_item *first = parser.get_first_token();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(textparser_get_token_position(first), 0u);
    EXPECT_EQ(first->len, 13u);

    ASSERT_NE(first->next, nullptr);
    EXPECT_EQ(textparser_get_token_position(first->next), 13u);
    EXPECT_EQ(first->next->len, 15u);
}

TEST(IncrementalParsing, MiddleReplacementLongerTextPreservesTail) {
    // Old: "<cfset a = 1><cfset b = 2><cfset c = 3>" (len 39)
    // token 1: 0..13, token 2: 13..26, token 3: 26..39
    const char *old_code = "<cfset a = 1><cfset b = 2><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    textparser_parser_state *state = textparser_state_generate(handle, 13);
    ASSERT_NE(state, nullptr);

    // New: "<cfset a = 1><cfset b = 2000><cfset c = 3>" (len 42, +3 bytes)
    // token 1: 0..13, token 2: 13..29, token 3: 29..42
    const char *new_code = "<cfset a = 1><cfset b = 2000><cfset c = 3>";
    ASSERT_EQ(textparser_set_text(handle, new_code, (int)strlen(new_code)), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, state, 13, 29), 0);

    textparser_token_item *tok1 = textparser_get_first_token(handle);
    ASSERT_NE(tok1, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok1), 0u);
    EXPECT_EQ(tok1->len, 13u);

    textparser_token_item *tok2 = tok1->next;
    ASSERT_NE(tok2, nullptr);
    EXPECT_EQ(tok2->prev, tok1);
    EXPECT_EQ(textparser_get_token_position(tok2), 13u);
    EXPECT_EQ(tok2->len, 16u);

    textparser_token_item *tok3 = tok2->next;
    ASSERT_NE(tok3, nullptr);
    EXPECT_EQ(tok3->prev, tok2);
    EXPECT_EQ(textparser_get_token_position(tok3), 29u); // Shifted by +3 from 26
    EXPECT_EQ(tok3->len, 13u);
    EXPECT_EQ(tok3->next, nullptr);

    textparser_state_free(state);
    textparser_close(handle);
}

TEST(IncrementalParsing, MiddleReplacementShorterTextPreservesTail) {
    // Old: "<cfset a = 1><cfset b = 2000><cfset c = 3>" (len 42)
    const char *old_code = "<cfset a = 1><cfset b = 2000><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    textparser_parser_state *state = textparser_state_generate(handle, 13);
    ASSERT_NE(state, nullptr);

    // New: "<cfset a = 1><cfset b = 2><cfset c = 3>" (len 39, -3 bytes)
    const char *new_code = "<cfset a = 1><cfset b = 2><cfset c = 3>";
    ASSERT_EQ(textparser_set_text(handle, new_code, (int)strlen(new_code)), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, state, 13, 26), 0);

    textparser_token_item *tok1 = textparser_get_first_token(handle);
    ASSERT_NE(tok1, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok1), 0u);
    EXPECT_EQ(tok1->len, 13u);

    textparser_token_item *tok2 = tok1->next;
    ASSERT_NE(tok2, nullptr);
    EXPECT_EQ(tok2->prev, tok1);
    EXPECT_EQ(textparser_get_token_position(tok2), 13u);
    EXPECT_EQ(tok2->len, 13u);

    textparser_token_item *tok3 = tok2->next;
    ASSERT_NE(tok3, nullptr);
    EXPECT_EQ(tok3->prev, tok2);
    EXPECT_EQ(textparser_get_token_position(tok3), 26u); // Shifted by -3 from 29
    EXPECT_EQ(tok3->len, 13u);
    EXPECT_EQ(tok3->next, nullptr);

    textparser_state_free(state);
    textparser_close(handle);
}

TEST(IncrementalParsing, MiddleDeletionPreservesHeadAndTail) {
    // Old: "<cfset a = 1><cfset b = 2><cfset c = 3>" (len 39)
    const char *old_code = "<cfset a = 1><cfset b = 2><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    textparser_parser_state *state = textparser_state_generate(handle, 13);
    ASSERT_NE(state, nullptr);

    // Delete `<cfset b = 2>` -> New: "<cfset a = 1><cfset c = 3>" (len 26, -13 bytes)
    const char *new_code = "<cfset a = 1><cfset c = 3>";
    ASSERT_EQ(textparser_set_text(handle, new_code, (int)strlen(new_code)), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, state, 13, 13), 0);

    textparser_token_item *tok1 = textparser_get_first_token(handle);
    ASSERT_NE(tok1, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok1), 0u);
    EXPECT_EQ(tok1->len, 13u);

    textparser_token_item *tok2 = tok1->next;
    ASSERT_NE(tok2, nullptr);
    EXPECT_EQ(tok2->prev, tok1);
    EXPECT_EQ(textparser_get_token_position(tok2), 13u); // Shifted by -13 from 26
    EXPECT_EQ(tok2->len, 13u);
    EXPECT_EQ(tok2->next, nullptr);

    textparser_state_free(state);
    textparser_close(handle);
}

TEST(IncrementalParsing, LeadingEditPreservesTail) {
    // Old: "<cfset a = 1><cfset b = 2>" (len 26)
    const char *old_code = "<cfset a = 1><cfset b = 2>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    textparser_parser_state *state = textparser_state_generate(handle, 0);
    ASSERT_NE(state, nullptr);

    // New: "<cfset a = 100><cfset b = 2>" (len 28, +2 bytes)
    const char *new_code = "<cfset a = 100><cfset b = 2>";
    ASSERT_EQ(textparser_set_text(handle, new_code, (int)strlen(new_code)), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, state, 0, 15), 0);

    textparser_token_item *tok1 = textparser_get_first_token(handle);
    ASSERT_NE(tok1, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok1), 0u);
    EXPECT_EQ(tok1->len, 15u);

    textparser_token_item *tok2 = tok1->next;
    ASSERT_NE(tok2, nullptr);
    EXPECT_EQ(tok2->prev, tok1);
    EXPECT_EQ(textparser_get_token_position(tok2), 15u); // Shifted by +2 from 13
    EXPECT_EQ(tok2->len, 13u);
    EXPECT_EQ(tok2->next, nullptr);

    textparser_state_free(state);
    textparser_close(handle);
}

TEST(IncrementalParsing, AppendAtEOF) {
    // Old: "<cfset a = 1>" (len 13)
    const char *old_code = "<cfset a = 1>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    textparser_parser_state *state = textparser_state_generate(handle, 13);
    ASSERT_NE(state, nullptr);

    // New: "<cfset a = 1><cfset b = 2>" (len 26)
    const char *new_code = "<cfset a = 1><cfset b = 2>";
    ASSERT_EQ(textparser_set_text(handle, new_code, (int)strlen(new_code)), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, state, 13, 26), 0);

    textparser_token_item *tok1 = textparser_get_first_token(handle);
    ASSERT_NE(tok1, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok1), 0u);
    EXPECT_EQ(tok1->len, 13u);

    textparser_token_item *tok2 = tok1->next;
    ASSERT_NE(tok2, nullptr);
    EXPECT_EQ(tok2->prev, tok1);
    EXPECT_EQ(textparser_get_token_position(tok2), 13u);
    EXPECT_EQ(tok2->len, 13u);
    EXPECT_EQ(tok2->next, nullptr);

    textparser_state_free(state);
    textparser_close(handle);
}

TEST(IncrementalParsing, NullStateFallback) {
    // Incremental parse with NULL state should still splice and shift correctly
    const char *old_code = "<cfset a = 1><cfset b = 2><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    const char *new_code = "<cfset a = 1><cfset b = 999><cfset c = 3>";
    ASSERT_EQ(textparser_set_text(handle, new_code, (int)strlen(new_code)), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, nullptr, 13, 28), 0);

    textparser_token_item *tok1 = textparser_get_first_token(handle);
    ASSERT_NE(tok1, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok1), 0u);

    textparser_token_item *tok2 = tok1->next;
    ASSERT_NE(tok2, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok2), 13u);
    EXPECT_EQ(tok2->len, 15u);

    textparser_token_item *tok3 = tok2->next;
    ASSERT_NE(tok3, nullptr);
    EXPECT_EQ(textparser_get_token_position(tok3), 28u);
    EXPECT_EQ(tok3->len, 13u);

    textparser_close(handle);
}

TEST(IncrementalParsing, InvalidArguments) {
    ASSERT_EQ(textparser_parse_incremental(nullptr, &cfml_definition, nullptr, 0, 10), -1);
    
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem("test", 4, TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, nullptr, nullptr, 0, 4), -1);
    ASSERT_EQ(textparser_set_text(nullptr, "test", 4), -1);
    ASSERT_EQ(textparser_set_text(handle, nullptr, 4), -1);
    EXPECT_EQ(textparser_state_generate(nullptr, 0), nullptr);
    textparser_close(handle);
}

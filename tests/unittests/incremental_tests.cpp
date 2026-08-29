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

TEST(IncrementalParsing, CppDeltaEditWrapper) {
    const char *code1 = "<cfset a = 1><cfset b = 2>";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code1, (int)strlen(code1), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&cfml_definition), 0);

    // Replace `<cfset b = 2>` (at offset 13, len 13) with `<cfset b = 200>` (len 15)
    const char *replacement = "<cfset b = 200>";
    textparser_dirty_range dirty = {};
    ASSERT_EQ(parser.parse_incremental(&cfml_definition, 13, 13, replacement, strlen(replacement), &dirty), 0);
    EXPECT_EQ(dirty.dirty_start, 13u);
    EXPECT_EQ(dirty.dirty_end, 28u);

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

    // Replace `<cfset b = 2>` (at offset 13, len 13) with `<cfset b = 2000>` (len 16)
    const char *replacement = "<cfset b = 2000>";
    textparser_dirty_range dirty = {};
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 13, 13, replacement, strlen(replacement), &dirty), 0);
    EXPECT_EQ(dirty.dirty_start, 13u);
    EXPECT_EQ(dirty.dirty_end, 29u);

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

    textparser_close(handle);
}

TEST(IncrementalParsing, MiddleReplacementShorterTextPreservesTail) {
    // Old: "<cfset a = 1><cfset b = 2000><cfset c = 3>" (len 42)
    const char *old_code = "<cfset a = 1><cfset b = 2000><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    // Replace `<cfset b = 2000>` (at offset 13, len 16) with `<cfset b = 2>` (len 13)
    const char *replacement = "<cfset b = 2>";
    textparser_dirty_range dirty = {};
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 13, 16, replacement, strlen(replacement), &dirty), 0);
    EXPECT_EQ(dirty.dirty_start, 13u);
    EXPECT_EQ(dirty.dirty_end, 26u);

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

    textparser_close(handle);
}

TEST(IncrementalParsing, MiddleDeletionPreservesHeadAndTail) {
    // Old: "<cfset a = 1><cfset b = 2><cfset c = 3>" (len 39)
    const char *old_code = "<cfset a = 1><cfset b = 2><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    // Delete `<cfset b = 2>` (offset 13, len 13) -> New: "<cfset a = 1><cfset c = 3>" (len 26)
    textparser_dirty_range dirty = {};
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 13, 13, nullptr, 0, &dirty), 0);

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

    textparser_close(handle);
}

TEST(IncrementalParsing, LeadingEditPreservesTail) {
    // Old: "<cfset a = 1><cfset b = 2>" (len 26)
    const char *old_code = "<cfset a = 1><cfset b = 2>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    // Replace `<cfset a = 1>` (offset 0, len 13) with `<cfset a = 100>` (len 15)
    const char *replacement = "<cfset a = 100>";
    textparser_dirty_range dirty = {};
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, 13, replacement, strlen(replacement), &dirty), 0);
    EXPECT_EQ(dirty.dirty_start, 0u);
    EXPECT_EQ(dirty.dirty_end, 15u);

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

    textparser_close(handle);
}

TEST(IncrementalParsing, AppendAtEOF) {
    // Old: "<cfset a = 1>" (len 13)
    const char *old_code = "<cfset a = 1>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(old_code, (int)strlen(old_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    // Append `<cfset b = 2>` at offset 13 (old_len 0, new_len 13)
    const char *addition = "<cfset b = 2>";
    textparser_dirty_range dirty = {};
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 13, 0, addition, strlen(addition), &dirty), 0);
    EXPECT_EQ(dirty.dirty_start, 13u);
    EXPECT_EQ(dirty.dirty_end, 26u);

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

    textparser_close(handle);
}

TEST(IncrementalParsing, InvalidArguments) {
    ASSERT_EQ(textparser_parse_incremental(nullptr, &cfml_definition, 0, 0, nullptr, 0, nullptr), -1);
    
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem("test", 4, TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse_incremental(handle, nullptr, 0, 0, nullptr, 0, nullptr), -1);
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 10, 0, nullptr, 0, nullptr), -1); // Out of bounds offset
    ASSERT_EQ(textparser_set_text(nullptr, "test", 4), -1);
    ASSERT_EQ(textparser_set_text(handle, nullptr, 4), -1);
    EXPECT_EQ(textparser_state_generate(nullptr, 0), nullptr);
    textparser_close(handle);
}

TEST(IncrementalParsing, IntegerOverflowProtection) {
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem("<cfset a = 1>", 13, TEXTPARSER_ENCODING_LATIN1, &handle), 0);

    // 1. old_len wrapping around SIZE_MAX
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, (size_t)-1, nullptr, 0, nullptr), -1);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 5, (size_t)-1, nullptr, 0, nullptr), -1);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 5, (size_t)-5, nullptr, 0, nullptr), -1);

    // 2. edit_offset near SIZE_MAX
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, (size_t)-1, 0, nullptr, 0, nullptr), -1);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, (size_t)-1, (size_t)-1, nullptr, 0, nullptr), -1);

    // 3. new_len near SIZE_MAX / overflow unit multiplication
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, 0, "x", (size_t)-1, nullptr), -1);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, 0, "x", (size_t)-1 / 2, nullptr), -1);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, 0, "x", 16 * 1024 * 1024, nullptr), -1);

    textparser_close(handle);

    // 4. Multi-byte encoding (UTF-16) overflow protection
    static const uint16_t u16_code[] = { '<', 'c', 'f', 's', 'e', 't', ' ', 'a', '=', '1', '>', 0 };
    ASSERT_EQ(textparser_openmem((const char *)u16_code, -1, TEXTPARSER_ENCODING_UTF_16, &handle), 0);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, (size_t)-1, nullptr, 0, nullptr), -1);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 2, (size_t)-1, nullptr, 0, nullptr), -1);
    EXPECT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, 0, "x", (size_t)-1 / sizeof(uint16_t) + 1, nullptr), -1);
    textparser_close(handle);
}

TEST(IncrementalParsing, SetTextAfterIncrementalParse) {
    const char *code1 = "<cfset a = 1>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code1, (int)strlen(code1), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    // Edit to create owned_buffer
    const char *edit1 = "<cfset a = 100>";
    textparser_dirty_range dirty = {};
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, strlen(code1), edit1, strlen(edit1), &dirty), 0);
    EXPECT_EQ(textparser_get_text_size(handle), strlen(edit1));

    // Now set brand new text
    const char *code2 = "<cfset b = 2000>";
    ASSERT_EQ(textparser_set_text(handle, code2, -1), 0);
    EXPECT_EQ(textparser_get_text_size(handle), strlen(code2));
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    textparser_token_item *tok = textparser_get_first_token(handle);
    ASSERT_NE(tok, nullptr);
    EXPECT_EQ(tok->len, strlen(code2));

    // Perform an incremental parse on code2
    const char *code2_replacement = "<cfset b = 9999>";
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, strlen(code2), code2_replacement, strlen(code2_replacement), &dirty), 0);
    EXPECT_EQ(textparser_get_text_size(handle), strlen(code2_replacement));

    tok = textparser_get_first_token(handle);
    ASSERT_NE(tok, nullptr);
    EXPECT_EQ(tok->len, strlen(code2_replacement));

    textparser_close(handle);
}

TEST(IncrementalParsing, SetTextEncodingsAndEdgeCases) {
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem("test", 4, TEXTPARSER_ENCODING_UTF_16, &handle), 0);

    // Invalid alignment for UTF-16
    EXPECT_EQ(textparser_set_text(handle, "123", 3), -1);

    // Valid UTF-16 null-terminated (len = -1)
    static const uint16_t u16_val[] = { '<', 'c', 'f', 's', 'e', 't', '>', 0 };
    EXPECT_EQ(textparser_set_text(handle, (const char *)u16_val, -1), 0);
    EXPECT_EQ(textparser_get_text_size(handle), 7 * sizeof(uint16_t));

    textparser_close(handle);

    ASSERT_EQ(textparser_openmem("test", 4, TEXTPARSER_ENCODING_UTF_32, &handle), 0);
    // Invalid alignment for UTF-32
    EXPECT_EQ(textparser_set_text(handle, "12345", 5), -1);

    // Valid UTF-32 null-terminated (len = -1)
    static const uint32_t u32_val[] = { '<', 'c', 'f', 's', 'e', 't', '>', 0 };
    EXPECT_EQ(textparser_set_text(handle, (const char *)u32_val, -1), 0);
    EXPECT_EQ(textparser_get_text_size(handle), 7 * sizeof(uint32_t));

    textparser_close(handle);
}



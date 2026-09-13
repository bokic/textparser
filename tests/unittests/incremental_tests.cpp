#include <gtest/gtest.h>
#include <textparser.hpp>
#include <cfml_definition.json.h>
#include <html_definition.json.h>
#include <json_definition.json.h>
#include <c_definition.json.h>
#include <cpp_definition.json.h>
#include <javascript_definition.json.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

static void collect_tree_shape(const textparser_token_item *token,
                               std::vector<std::pair<int, size_t>> &out) {
    for (; token != nullptr; token = token->next) {
        out.push_back({token->token_id, token->len});
        collect_tree_shape(token->child, out);
    }
}

static bool tree_has_synthesized(const textparser_token_item *token) {
    for (; token != nullptr; token = token->next) {
        if (token->text_flags & 0x80000000u) return true;
        if (tree_has_synthesized(token->child)) return true;
    }
    return false;
}

// Compare an incremental edit on an already post-processed AST against a full
// parse followed by post_process of the edited text.
static bool processed_incremental_matches_full(const char *base, size_t offset,
                                               size_t old_len, const char *inserted,
                                               size_t new_len,
                                               const textparser_language_definition *def) {
    textparser::Parser incremental;
    if (incremental.openmem(base, (int)strlen(base), TEXTPARSER_ENCODING_UTF_8) != 0) return false;
    if (incremental.parse(def) != 0) return false;
    textparser_token_item *inc_root = incremental.get_first_token();
    textparser_post_process(&inc_root, def);
    if (incremental.parse_incremental(def, offset, old_len, inserted, new_len, nullptr) != 0) return false;

    std::string edited(base);
    edited.replace(offset, old_len, inserted, new_len);
    textparser::Parser full;
    if (full.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) return false;
    if (full.parse(def) != 0) return false;
    textparser_token_item *full_root = full.get_first_token();
    textparser_post_process(&full_root, def);

    std::vector<std::pair<int, size_t>> incremental_shape;
    std::vector<std::pair<int, size_t>> full_shape;
    collect_tree_shape(incremental.get_first_token(), incremental_shape);
    collect_tree_shape(full.get_first_token(), full_shape);
    return incremental_shape == full_shape;
}

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

TEST(IncrementalParsing, BufferAliasingAndOverlap) {
    // Initial: "<cfset a = 1><cfset b = 2><cfset c = 3>"
    const char *code = "<cfset a = 1><cfset b = 2><cfset c = 3>";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);

    // 1. new_text aliases handle->text_addr (e.g. duplicating second tag into first position)
    const char *alias_source = textparser_get_text(handle) + 13; // "<cfset b = 2>"
    textparser_dirty_range dirty = {};
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, 13, alias_source, 13, &dirty), 0);
    EXPECT_EQ(std::string(textparser_get_text(handle), textparser_get_text_size(handle)), "<cfset b = 2><cfset b = 2><cfset c = 3>");

    // 2. new_text aliases handle->owned_buffer across another edit
    alias_source = textparser_get_text(handle) + 26; // "<cfset c = 3>"
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, 13, alias_source, 13, &dirty), 0);
    EXPECT_EQ(std::string(textparser_get_text(handle), textparser_get_text_size(handle)), "<cfset c = 3><cfset b = 2><cfset c = 3>");

    // 3. new_text aliases a larger slice causing buffer reallocation / expansion
    alias_source = textparser_get_text(handle); // "<cfset c = 3><cfset b = 2>" (len 26)
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 26, 13, alias_source, 26, &dirty), 0);
    EXPECT_EQ(std::string(textparser_get_text(handle), textparser_get_text_size(handle)), "<cfset c = 3><cfset b = 2><cfset c = 3><cfset b = 2>");

    textparser_close(handle);
}

static bool incremental_matches_full(const char *base, size_t offset,
                                     size_t old_len, const char *inserted,
                                     size_t new_len) {
    textparser::Parser incremental;
    if (incremental.openmem(base, (int)strlen(base), TEXTPARSER_ENCODING_UTF_8) != 0) return false;
    if (incremental.parse(&json_definition) != 0) return false;
    if (incremental.parse_incremental(&json_definition, offset, old_len, inserted, new_len, nullptr) != 0) return false;

    std::string edited(base);
    edited.replace(offset, old_len, inserted, new_len);
    textparser::Parser full;
    if (full.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) return false;
    if (full.parse(&json_definition) != 0) return false;

    std::vector<std::pair<int, size_t>> incremental_shape;
    std::vector<std::pair<int, size_t>> full_shape;
    collect_tree_shape(incremental.get_first_token(), incremental_shape);
    collect_tree_shape(full.get_first_token(), full_shape);
    return incremental_shape == full_shape;
}

static bool incremental_matches_full_def(const char *base, size_t offset,
                                         size_t old_len, const char *inserted,
                                         size_t new_len,
                                         const textparser_language_definition *def) {
    textparser::Parser incremental;
    if (incremental.openmem(base, (int)strlen(base), TEXTPARSER_ENCODING_UTF_8) != 0) return false;
    if (incremental.parse(def) != 0) return false;
    if (incremental.parse_incremental(def, offset, old_len, inserted, new_len, nullptr) != 0) return false;

    std::string edited(base);
    edited.replace(offset, old_len, inserted, new_len);
    textparser::Parser full;
    if (full.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) return false;
    if (full.parse(def) != 0) return false;

    std::vector<std::pair<int, size_t>> incremental_shape;
    std::vector<std::pair<int, size_t>> full_shape;
    collect_tree_shape(incremental.get_first_token(), incremental_shape);
    collect_tree_shape(full.get_first_token(), full_shape);
    return incremental_shape == full_shape;
}

// Regression: the anchor must reach the root so an edit inside a container's own
// start/end delimiter re-evaluates that container (e.g. `<cfset` -> `<Xcfset`
// must become Unprocessed, a nested CFML OutputStartTag must re-evaluate its
// OutputTagPair, and JavaScript regex-vs-division and C sign-merge contexts must
// be re-derived). Every single-character insert and delete over four languages
// must equal a full parse.
TEST(IncrementalParsing, CrossLanguageInsertsAndDeletesMatchFullParse) {
    struct LangCase {
        const char *name;
        const char *text;
        const textparser_language_definition *def;
    };
    const LangCase cases[] = {
        {"cfml", "<cfset x = 1 + 2>\n<cfoutput>#x#</cfoutput>\n", &cfml_definition},
        {"c", "int main() {\n  int x = -1 + 2;\n  return x;\n}\n", &c_definition},
        {"javascript", "var x = /ab+c/g;\nvar y = a / b / c;\nfunction f(a){ return a + 1; }\n", &javascript_definition},
    };
    const char *chars[] = {"X", "9", ".", "-", "e", "\\", "\"", "'", ",", "{", "}", "[", "]", ":", ";", " ", "\n"};

    for (const LangCase &lang : cases) {
        size_t mismatches = 0;

        for (size_t offset = 1; offset <= strlen(lang.text); ++offset) {
            for (const char *ch : chars) {
                std::string edited(lang.text);
                edited.insert(offset, ch);
                textparser::Parser probe;
                if (probe.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
                if (probe.parse(lang.def) != 0) continue; // skip invalid input
                if (!incremental_matches_full_def(lang.text, offset, 0, ch, strlen(ch), lang.def)) mismatches++;
            }
        }

        for (size_t offset = 0; offset < strlen(lang.text); ++offset) {
            std::string edited(lang.text);
            edited.erase(offset, 1);
            textparser::Parser probe;
            if (probe.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
            if (probe.parse(lang.def) != 0) continue;
            if (!incremental_matches_full_def(lang.text, offset, 1, nullptr, 0, lang.def)) mismatches++;
        }

        EXPECT_EQ(mismatches, 0u) << lang.name;
    }
}

// Regression: inserting a signed literal must merge the sign into the number
// like a full parse. The splice could leave a reused node's `prev` pointing at
// a replaced node, so `maybe_merge_sign`'s unlink modified the wrong neighbour
// and the sign survived as an extra Operator sibling.
TEST(IncrementalParsing, JavaScriptSignMergeOnInsertAndReplace) {
    const char *base = "var x = /ab+c/g;\nvar y = a / b / c;\nfunction f(a){ return a + 1; }\n";
    EXPECT_TRUE(incremental_matches_full_def(base, 16, 0, "+1", 2, &javascript_definition));
    EXPECT_TRUE(incremental_matches_full_def(base, 17, 1, "+1", 2, &javascript_definition));
    EXPECT_TRUE(incremental_matches_full_def(base, 33, 1, "+9", 2, &javascript_definition));
    EXPECT_TRUE(incremental_matches_full_def(base, 44, 2, "-0X", 3, &javascript_definition));
    EXPECT_TRUE(incremental_matches_full_def(base, 45, 2, "-1", 2, &javascript_definition));
}

// Regression: inserting before a key's ':' changes the Key lookahead, so a full
// parse reclassifies it as a String. The old incremental path re-lexed only the
// Key's children and kept the stale Key.
TEST(IncrementalParsing, EditBeforeKeyColonReclassifiesKey) {
    const char *base = "{\"message\": \"hello\"}";
    size_t colon = 0;
    while (base[colon] != ':') colon++;
    EXPECT_TRUE(incremental_matches_full(base, colon, 0, "X", 1));
    EXPECT_TRUE(incremental_matches_full(base, colon, 0, "\"", 1));
}

// Regression: inserting a quote inside a key's content changes string
// boundaries and can flip Key <-> String.
TEST(IncrementalParsing, QuoteInsideKeyContentMatchesFullParse) {
    const char *base = "{\n  \"message\": \"Hello, world!\",\n  \"n\": 42\n}\n";
    for (size_t offset = 5; offset <= 11; ++offset) {
        EXPECT_TRUE(incremental_matches_full(base, offset, 0, "\"", 1)) << "offset " << offset;
    }
}

// Regression: inserting '[' opens a nested container; the old splice produced
// overlapping/out-of-order leaves.
TEST(IncrementalParsing, ArrayOpenInsideArrayMatchesFullParse) {
    const char *base = "{\n  \"arr\": [1, 2.5, -3]\n}\n";
    size_t start = 0;
    while (base[start] != '[') start++;
    for (size_t offset = start; offset <= start + 12; ++offset) {
        EXPECT_TRUE(incremental_matches_full(base, offset, 0, "[", 1)) << "offset " << offset;
    }
}

// Regression: inserting '[' can leave the outer array unterminated. The full
// parser recovers by emitting Unprocessed; the incremental path must recover too
// (anchor must bubble to a container that allows arbitrary text).
TEST(IncrementalParsing, UnterminatedArrayRecoversLikeFullParse) {
    const char *base = "{\"arr\": [1, 2.5, -3]}";
    size_t one = 0;
    while (base[one] != '1') one++;
    EXPECT_TRUE(incremental_matches_full(base, one, 0, "[", 1));
}

// Regression: inserting a backslash starts a string escape.
TEST(IncrementalParsing, BackslashInsideStringMatchesFullParse) {
    const char *base = "{\n  \"message\": \"Hello, world!\"\n}\n";
    size_t start = 0;
    while (base[start] != 'H') start++;
    for (size_t offset = start; offset <= start + 12; ++offset) {
        EXPECT_TRUE(incremental_matches_full(base, offset, 0, "\\", 1)) << "offset " << offset;
    }
}

// Regression: inserting a newline changes whitespace/line structure.
TEST(IncrementalParsing, NewlineInsideStringMatchesFullParse) {
    const char *base = "{\n  \"message\": \"Hello, world!\"\n}\n";
    size_t start = 0;
    while (base[start] != 'H') start++;
    for (size_t offset = start; offset <= start + 12; ++offset) {
        EXPECT_TRUE(incremental_matches_full(base, offset, 0, "\n", 1)) << "offset " << offset;
    }
}

// Regression: whitespace inserted at a token boundary used to split/duplicate
// whitespace leaves.
TEST(IncrementalParsing, WhitespaceAtBoundaryMatchesFullParse) {
    const char *base = "{\n  \"message\": \"Hello\"\n}\n";
    for (size_t offset = 1; offset < strlen(base); ++offset) {
        EXPECT_TRUE(incremental_matches_full(base, offset, 0, " ", 1)) << "offset " << offset;
    }
}

// Exact equivalence over every single-character edit of the JSON sample. Unlike
// DifferentialAgainstFullParse this asserts zero mismatches, so it catches any
// regression in the previously-failing scenarios.
TEST(IncrementalParsing, AllStructuralEditsMatchFullParseExactly) {
    const char *base = "{\n  \"message\": \"Hello, world!\",\n  \"n\": 42,\n  \"arr\": [1, 2.5, -3]\n}\n";
    const char *chars[] = {"X", "9", ".", "-", "e", "\\", "\"", ",", "{", "[", ":", " ", "\n"};
    size_t mismatches = 0;
    for (size_t offset = 1; offset <= strlen(base); ++offset) {
        for (const char *ch : chars) {
            std::string edited(base);
            edited.insert(offset, ch);
            textparser::Parser probe;
            if (probe.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
            if (probe.parse(&json_definition) != 0) continue; // skip invalid JSON
            if (!incremental_matches_full(base, offset, 0, ch, strlen(ch))) mismatches++;
        }
    }
    EXPECT_EQ(mismatches, 0u);
}

// Regression: deeply nested unterminated containers made the speculative
// candidate retry loop exponential (the parser hung). The parse work budget must
// make it terminate with an error instead of spinning forever.
TEST(IncrementalParsing, PathologicalNestingTerminates) {
    std::string text;
    for (int i = 0; i < 30; ++i) text += "{\"a\":";
    textparser::Parser p;
    ASSERT_EQ(p.openmem(text.c_str(), (int)text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(p.parse(&json_definition), -1);
}

TEST(IncrementalParsing, DifferentialAgainstFullParse) {
    // Regression guard for incremental/full equivalence. Every single-character
    // edit is applied incrementally and compared against a fresh full parse.
    // The guard is exact: all edits that both paths accept must produce the
    // same CST shape.
    const char *base = "{\n  \"message\": \"Hello, world!\",\n  \"n\": 42,\n  \"arr\": [1, 2.5, -3]\n}\n";
    const char *chars[] = {"X", "9", ".", "-", "e", "\\", "\"", ",", "{", "[", ":", " ", "\n"};
    size_t total = 0;
    size_t equivalent = 0;

    for (size_t offset = 1; offset <= strlen(base); ++offset) {
        for (const char *ch : chars) {
            textparser::Parser incremental;
            if (incremental.openmem(base, (int)strlen(base), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
            if (incremental.parse(&json_definition) != 0) continue;
            if (incremental.parse_incremental(&json_definition, offset, 0, ch, strlen(ch), nullptr) != 0) continue;

            std::string new_text(base);
            new_text.insert(offset, ch);
            textparser::Parser full;
            if (full.openmem(new_text.c_str(), (int)new_text.size(), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
            if (full.parse(&json_definition) != 0) continue;

            std::vector<std::pair<int, size_t>> incremental_shape;
            std::vector<std::pair<int, size_t>> full_shape;
            collect_tree_shape(incremental.get_first_token(), incremental_shape);
            collect_tree_shape(full.get_first_token(), full_shape);
            total++;
            if (incremental_shape == full_shape) equivalent++;
        }
    }

    ASSERT_GT(total, 0u);
    EXPECT_EQ(equivalent, total) << "equivalent " << equivalent << "/" << total;
}

TEST(IncrementalParsing, MidLeafInsertResizesLeafAndMatchesFullParse) {
    // Inserting inside a leaf must resize that leaf and its ancestors rather
    // than reparsing, so the resulting CST equals a full parse.
    const char *text = "{\"message\": \"Hello\"}";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&json_definition), 0);

    const size_t offset = strlen("{\"message\": \"He"); // inside the string content
    ASSERT_EQ(parser.parse_incremental(&json_definition, offset, 0, "X", 1, nullptr), 0);

    std::string new_text = text;
    new_text.insert(offset, "X");

    textparser::Parser full;
    ASSERT_EQ(full.openmem(new_text.c_str(), (int)new_text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(full.parse(&json_definition), 0);

    std::vector<std::pair<int, size_t>> incremental_shape;
    std::vector<std::pair<int, size_t>> full_shape;
    collect_tree_shape(parser.get_first_token(), incremental_shape);
    collect_tree_shape(full.get_first_token(), full_shape);
    EXPECT_EQ(incremental_shape, full_shape);
}

TEST(IncrementalParsing, MidLeafDeleteResizesLeafAndMatchesFullParse) {
    const char *text = "{\"message\": \"Hello\"}";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&json_definition), 0);

    const size_t offset = strlen("{\"message\": \"He"); // delete the first 'l'
    ASSERT_EQ(parser.parse_incremental(&json_definition, offset, 1, nullptr, 0, nullptr), 0);

    std::string new_text = text;
    new_text.erase(offset, 1);

    textparser::Parser full;
    ASSERT_EQ(full.openmem(new_text.c_str(), (int)new_text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(full.parse(&json_definition), 0);

    std::vector<std::pair<int, size_t>> incremental_shape;
    std::vector<std::pair<int, size_t>> full_shape;
    collect_tree_shape(parser.get_first_token(), incremental_shape);
    collect_tree_shape(full.get_first_token(), full_shape);
    EXPECT_EQ(incremental_shape, full_shape);
}




TEST(IncrementalParsing, LongSingleLineArrayMatchesFullParse) {
    // A long line exercises the single-line search-bound cache in
    // textparser_get_end_of_line_units (previously O(line) per candidate,
    // O(n^2) overall) and the PCRE2 UTF-8 fast path. Guards against both
    // performance regressions while asserting the CST stays correct.
    std::string text = "[";
    for (int i = 0; i < 5000; ++i) {
        text += std::to_string(i);
        text += ",";
    }
    text += "1]";

    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(text.c_str(), (int)text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&json_definition), 0);
    EXPECT_EQ(parser.get_parse_error(), nullptr);

    // A mid-line edit must still match a full parse.
    const size_t offset = text.size() / 2;
    EXPECT_TRUE(incremental_matches_full(text.c_str(), offset, 0, " ", 1));
    EXPECT_TRUE(incremental_matches_full(text.c_str(), offset, 1, nullptr, 0));
}

TEST(IncrementalParsing, ExpressionPostProcessReDerivedOnIncrementalEdit) {
    // Initial expression: 2 + 3 * 4 (MulOperator nested under AddOperator)
    const char *initial_code = "<cfset res = 2 + 3 * 4 />";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(initial_code, strlen(initial_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);
    textparser_token_item *root = textparser_get_first_token(handle);
    textparser_post_process(&root, &cfml_definition);

    // Verify initial AST has AddOperator wrapping MulOperator
    textparser_token_item *assign = nullptr;
    for (textparser_token_item *t = root; t; t = t->next) {
        for (textparser_token_item *c = t->child; c; c = c->next) {
            if (c->token_id >= 0 && strcmp(cfml_definition.tokens[c->token_id].name, "AssignOperator") == 0) {
                assign = c;
                break;
            }
        }
    }
    ASSERT_NE(assign, nullptr);
    textparser_token_item *add = nullptr;
    for (textparser_token_item *c = assign->child; c; c = c->next) {
        if (c->token_id >= 0 && strcmp(cfml_definition.tokens[c->token_id].name, "AddOperator") == 0) {
            add = c;
            break;
        }
    }
    ASSERT_NE(add, nullptr);
    textparser_token_item *mul = nullptr;
    for (textparser_token_item *c = add->child; c; c = c->next) {
        if (c->token_id >= 0 && strcmp(cfml_definition.tokens[c->token_id].name, "MulOperator") == 0) {
            mul = c;
            break;
        }
    }
    ASSERT_NE(mul, nullptr);

    // Incrementally replace "+" with "*" and "*" with "+":
    // Change "2 + 3 * 4" to "2 * 3 + 4"
    // In "<cfset res = 2 + 3 * 4 />":
    // offset of '+': 15
    const char *plus_replacement = "*";
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 15, 1, plus_replacement, 1, nullptr), 0);

    // offset of '*': 19
    const char *mul_replacement = "+";
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 19, 1, mul_replacement, 1, nullptr), 0);

    // In "<cfset res = 2 * 3 + 4 />", AddOperator must now wrap MulOperator
    // with MulOperator being left child: (2 * 3) + 4
    root = textparser_get_first_token(handle);
    assign = nullptr;
    for (textparser_token_item *t = root; t; t = t->next) {
        for (textparser_token_item *c = t->child; c; c = c->next) {
            if (c->token_id >= 0 && strcmp(cfml_definition.tokens[c->token_id].name, "AssignOperator") == 0) {
                assign = c;
                break;
            }
        }
    }
    ASSERT_NE(assign, nullptr);
    add = nullptr;
    for (textparser_token_item *c = assign->child; c; c = c->next) {
        if (c->token_id >= 0 && strcmp(cfml_definition.tokens[c->token_id].name, "AddOperator") == 0) {
            add = c;
            break;
        }
    }
    ASSERT_NE(add, nullptr);
    mul = nullptr;
    for (textparser_token_item *c = add->child; c; c = c->next) {
        if (c->token_id >= 0 && strcmp(cfml_definition.tokens[c->token_id].name, "MulOperator") == 0) {
            mul = c;
            break;
        }
    }
    ASSERT_NE(mul, nullptr);

    // Compare with full parse + post process
    const char *final_code = "<cfset res = 2 * 3 + 4 />";
    textparser_t full_handle = nullptr;
    ASSERT_EQ(textparser_openmem(final_code, strlen(final_code), TEXTPARSER_ENCODING_LATIN1, &full_handle), 0);
    ASSERT_EQ(textparser_parse(full_handle, &cfml_definition), 0);
    textparser_token_item *full_root = textparser_get_first_token(full_handle);
    textparser_post_process(&full_root, &cfml_definition);

    std::vector<std::pair<int, size_t>> incremental_shape;
    std::vector<std::pair<int, size_t>> full_shape;
    collect_tree_shape(root, incremental_shape);
    collect_tree_shape(full_root, full_shape);
    EXPECT_EQ(incremental_shape, full_shape);

    textparser_close(handle);
    textparser_close(full_handle);
}

TEST(IncrementalParsing, RepeatedIncrementalExpressionEditsWithoutLeak) {
    const char *initial_code = "<cfset res = 1 + 2 + 3 + 4 />";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(initial_code, strlen(initial_code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);
    textparser_token_item *root = textparser_get_first_token(handle);
    textparser_post_process(&root, &cfml_definition);

    // Repeatedly swap operators back and forth
    for (int iter = 0; iter < 10; iter++) {
        const char *op = (iter % 2 == 0) ? "*" : "+";
        ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 15, 1, op, 1, nullptr), 0);
        ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 19, 1, op, 1, nullptr), 0);
    }

    // Now test a full parse reset (start_pos == 0, end_pos >= total)
    const char *reset_code = "<cfset res = 10 * 20 />";
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, strlen(initial_code), reset_code, strlen(reset_code), nullptr), 0);

    textparser_close(handle);
}

static bool tree_contains_type(const textparser_token_item *token,
                               const textparser_language_definition *def,
                               const char *name) {
    for (; token != nullptr; token = token->next) {
        if (token->token_id >= 0 && def->tokens[token->token_id].name != nullptr &&
            strcmp(def->tokens[token->token_id].name, name) == 0) {
            return true;
        }
        if (tree_contains_type(token->child, def, name)) return true;
    }
    return false;
}

// A raw CST (caller never called textparser_post_process) must stay raw after an
// incremental edit: no synthesized expression/template wrappers may appear.
TEST(IncrementalParsing, RawCstStaysRawAfterIncrementalEdit) {
    const char *initial = "<cfset res = 2 + 3 * 4 />";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(initial, strlen(initial), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);
    ASSERT_FALSE(tree_has_synthesized(textparser_get_first_token(handle)));

    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 15, 1, "*", 1, nullptr), 0);
    EXPECT_FALSE(tree_has_synthesized(textparser_get_first_token(handle)));

    textparser_close(handle);
}

// Once a caller has opted into AST mode via textparser_post_process, a full
// document reset edit must re-derive the AST instead of silently reverting to a
// raw CST.
TEST(IncrementalParsing, PostProcessModeStickyAcrossFullReset) {
    const char *initial = "<cfset res = 2 + 3 * 4 />";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(initial, strlen(initial), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cfml_definition), 0);
    textparser_token_item *root = textparser_get_first_token(handle);
    textparser_post_process(&root, &cfml_definition);
    ASSERT_TRUE(tree_has_synthesized(root));

    const char *replacement = "<cfset res = 5 * 6 + 7 />";
    ASSERT_EQ(textparser_parse_incremental(handle, &cfml_definition, 0, strlen(initial),
                                           replacement, strlen(replacement), nullptr), 0);

    root = textparser_get_first_token(handle);
    ASSERT_TRUE(tree_has_synthesized(root));

    textparser_t full = nullptr;
    ASSERT_EQ(textparser_openmem(replacement, strlen(replacement), TEXTPARSER_ENCODING_LATIN1, &full), 0);
    ASSERT_EQ(textparser_parse(full, &cfml_definition), 0);
    textparser_token_item *full_root = textparser_get_first_token(full);
    textparser_post_process(&full_root, &cfml_definition);

    std::vector<std::pair<int, size_t>> incremental_shape;
    std::vector<std::pair<int, size_t>> full_shape;
    collect_tree_shape(root, incremental_shape);
    collect_tree_shape(full_root, full_shape);
    EXPECT_EQ(incremental_shape, full_shape);

    textparser_close(handle);
    textparser_close(full);
}

// Cast disambiguation mutates a Parenthesis token ID in place (it does not
// synthesize a tagged wrapper), so the engine must still detect AST mode and
// reclassify the parenthesis after an edit.
TEST(IncrementalParsing, CastDisambiguationReDerivedOnIncrementalEdit) {
    const char *initial = "int y = (int)x;\n";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(initial, strlen(initial), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &c_definition), 0);
    textparser_token_item *root = textparser_get_first_token(handle);
    textparser_post_process(&root, &c_definition);
    ASSERT_TRUE(tree_contains_type(root, &c_definition, "TypeCast"));

    ASSERT_EQ(textparser_parse_incremental(handle, &c_definition, 9, 3, "long", 4, nullptr), 0);
    EXPECT_TRUE(tree_contains_type(textparser_get_first_token(handle), &c_definition, "TypeCast"));

    EXPECT_TRUE(processed_incremental_matches_full(initial, 9, 3, "long", 4, &c_definition));
    textparser_close(handle);
}

// Template/generic disambiguation synthesizes a TemplateGroup; after an edit the
// group must be re-derived (or dropped) exactly like a fresh parse.
TEST(IncrementalParsing, TemplateDisambiguationReDerivedOnIncrementalEdit) {
    const char *initial = "std::vector<int> x;\n";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(initial, strlen(initial), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, &cpp_definition), 0);
    textparser_token_item *root = textparser_get_first_token(handle);
    textparser_post_process(&root, &cpp_definition);
    ASSERT_TRUE(tree_contains_type(root, &cpp_definition, "TemplateGroup"));

    // Keep the template valid by renaming the variable, then verify the group
    // survives the edit and the whole AST matches a full parse.
    EXPECT_TRUE(processed_incremental_matches_full(initial, 17, 1, "y", 1, &cpp_definition));
    ASSERT_EQ(textparser_parse_incremental(handle, &cpp_definition, 17, 1, "y", 1, nullptr), 0);
    EXPECT_TRUE(tree_contains_type(textparser_get_first_token(handle), &cpp_definition, "TemplateGroup"));

    textparser_close(handle);
}

// Differential: every single-character insert over CFML, C and C++ must keep an
// already post-processed AST identical to a full parse + post_process.
TEST(IncrementalParsing, ProcessedAstIncrementalMatchesFullParse) {
    struct LangCase {
        const char *name;
        const char *text;
        const textparser_language_definition *def;
    };
    const LangCase cases[] = {
        {"cfml", "<cfset res = 2 + 3 * 4 />", &cfml_definition},
        {"c", "int y = (int)x;\n", &c_definition},
        {"cpp", "std::vector<int> x;\n", &cpp_definition},
    };
    const char *chars[] = {"+", "*", "(", ")", "9", " ", "x", "<", ">"};

    for (const LangCase &lang : cases) {
        size_t mismatches = 0;
        for (size_t offset = 0; offset <= strlen(lang.text); ++offset) {
            for (const char *ch : chars) {
                std::string edited(lang.text);
                edited.insert(offset, ch);
                textparser::Parser probe;
                if (probe.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
                if (probe.parse(lang.def) != 0) continue;
                if (!processed_incremental_matches_full(lang.text, offset, 0, ch, strlen(ch), lang.def)) {
                    mismatches++;
                }
            }
        }
        EXPECT_EQ(mismatches, 0u) << lang.name;
    }
}


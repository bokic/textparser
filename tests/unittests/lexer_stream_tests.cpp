#include <gtest/gtest.h>
#include <textparser.hpp>
#include <c_definition.json.h>
#include <json_definition.json.h>
#include <cfml_definition.json.h>
#include <cstring>
#include <string>
#include <vector>

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

namespace {

struct TokenView {
    int kind;
    size_t start;
    size_t end;
    size_t leading_trivia_start;
    size_t leading_trivia_count;
    int mode;
    int lexical_goal;
    uint32_t flags;
    std::string decoded_value;

    bool operator==(const TokenView &other) const {
        return kind == other.kind && start == other.start && end == other.end &&
               leading_trivia_start == other.leading_trivia_start &&
               leading_trivia_count == other.leading_trivia_count &&
               mode == other.mode && lexical_goal == other.lexical_goal &&
               flags == other.flags && decoded_value == other.decoded_value;
    }
};

struct TriviaView {
    int kind;
    size_t start;
    size_t end;
    uint32_t flags;

    bool operator==(const TriviaView &other) const {
        return kind == other.kind && start == other.start && end == other.end &&
               flags == other.flags;
    }
};

struct Snapshot {
    std::vector<TokenView> tokens;
    std::vector<TriviaView> trivia;

    bool operator==(const Snapshot &other) const {
        return tokens == other.tokens && trivia == other.trivia;
    }
};

Snapshot capture_snapshot(textparser::Parser &parser) {
    Snapshot snapshot;
    size_t token_count = 0;
    const textparser_lex_token *tokens = parser.lexer_tokens(&token_count);
    for (size_t i = 0; i < token_count; ++i) {
        snapshot.tokens.push_back(TokenView{
            tokens[i].kind, tokens[i].start, tokens[i].end,
            tokens[i].leading_trivia_start, tokens[i].leading_trivia_count,
            tokens[i].mode, tokens[i].lexical_goal, tokens[i].flags,
            tokens[i].decoded_value ? tokens[i].decoded_value : ""});
    }
    size_t trivia_count = 0;
    const textparser_lex_trivia *trivia = parser.lexer_trivia(&trivia_count);
    for (size_t i = 0; i < trivia_count; ++i) {
        snapshot.trivia.push_back(TriviaView{trivia[i].kind, trivia[i].start,
                                             trivia[i].end, trivia[i].flags});
    }
    return snapshot;
}

// Apply the edit incrementally, then assert the resulting lexer snapshot is
// byte-identical to a fresh full parse of the edited text.
void expect_snapshot_matches_full(const char *base, size_t offset, size_t old_len,
                                  const char *inserted, size_t new_len,
                                  const textparser_language_definition *def) {
    textparser::Parser incremental;
    ASSERT_EQ(incremental.openmem(base, (int)strlen(base), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(incremental.parse(def), 0);
    ASSERT_EQ(incremental.parse_incremental(def, offset, old_len, inserted, new_len, nullptr), 0);

    std::string edited(base);
    edited.replace(offset, old_len, inserted, new_len);
    textparser::Parser full;
    ASSERT_EQ(full.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(full.parse(def), 0);

    EXPECT_EQ(capture_snapshot(incremental), capture_snapshot(full));
}

} // namespace

// The in-leaf fast path patches the existing snapshot instead of rebuilding it.
// The patched snapshot must still equal a full parse.
TEST(lexer_streams, incremental_leaf_insert_matches_full_snapshot) {
    const char *base = "{\"message\": \"Hello\"}";
    expect_snapshot_matches_full(base, 13, 0, "X", 1, &json_definition);
    expect_snapshot_matches_full(base, 2, 0, "z", 1, &json_definition);
    expect_snapshot_matches_full(base, 12, 0, "q", 1, &json_definition);
}

TEST(lexer_streams, incremental_leaf_delete_matches_full_snapshot) {
    const char *base = "{\"message\": \"Hello\"}";
    expect_snapshot_matches_full(base, 13, 1, nullptr, 0, &json_definition);
    expect_snapshot_matches_full(base, 2, 1, nullptr, 0, &json_definition);
    expect_snapshot_matches_full(base, 12, 1, nullptr, 0, &json_definition);
}

TEST(lexer_streams, incremental_leaf_replace_matches_full_snapshot) {
    const char *base = "{\"message\": \"Hello\"}";
    expect_snapshot_matches_full(base, 13, 1, "Z", 1, &json_definition);
    expect_snapshot_matches_full(base, 3, 1, "xy", 2, &json_definition);
    expect_snapshot_matches_full(base, 4, 2, "q", 1, &json_definition);
}

// A structural edit (new delimiter) takes the general splice + rebuild path.
TEST(lexer_streams, incremental_structural_edit_matches_full_snapshot) {
    const char *base = "{\n  \"message\": \"Hello\",\n  \"n\": 42\n}\n";
    size_t off = strlen("{\n  \"message\": \"Hello\"");
    expect_snapshot_matches_full(base, off, 0, ",", 1, &json_definition);
    expect_snapshot_matches_full(base, 1, 0, "[", 1, &json_definition);
}

// Whitespace lives in the trivia stream; an edit there must keep the trivia
// offsets and the following token's aggregate flags correct.
TEST(lexer_streams, incremental_whitespace_edit_matches_full_snapshot) {
    const char *base = "int  a";
    expect_snapshot_matches_full(base, 4, 0, " ", 1, &c_definition);
    expect_snapshot_matches_full(base, 3, 0, "\n", 1, &c_definition);
    expect_snapshot_matches_full(base, 3, 1, nullptr, 0, &c_definition);
}

TEST(lexer_streams, incremental_cfml_edit_matches_full_snapshot) {
    const char *base = "<cfset a = 1><cfset b = 2>";
    expect_snapshot_matches_full(base, 14, 0, "9", 1, &cfml_definition);
    expect_snapshot_matches_full(base, 13, 13, "<cfset b = 200>", 15, &cfml_definition);
}

// Post-processed (AST) trees take the full-rebuild fallback; the snapshot must
// still match a full parse that was also post-processed.
TEST(lexer_streams, incremental_ast_mode_snapshot_matches_full) {
    const char *base = "<cfset res = 2 + 3 * 4 />";
    textparser::Parser incremental;
    ASSERT_EQ(incremental.openmem(base, (int)strlen(base), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(incremental.parse(&cfml_definition), 0);
    textparser_token_item *root = incremental.get_first_token();
    textparser_post_process(&root, &cfml_definition);
    ASSERT_EQ(incremental.parse_incremental(&cfml_definition, 15, 1, "*", 1, nullptr), 0);

    const char *edited = "<cfset res = 2 * 3 * 4 />";
    textparser::Parser full;
    ASSERT_EQ(full.openmem(edited, (int)strlen(edited), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(full.parse(&cfml_definition), 0);
    textparser_token_item *full_root = full.get_first_token();
    textparser_post_process(&full_root, &cfml_definition);

    EXPECT_EQ(capture_snapshot(incremental), capture_snapshot(full));
}

// Repeated edits must keep reusing the retained snapshot buffers while staying
// in sync with a full parse.
TEST(lexer_streams, repeated_leaf_edits_keep_snapshot_in_sync) {
    std::string text = "{\"message\": \"Hello\"}";
    textparser::Parser incremental;
    ASSERT_EQ(incremental.openmem(text.c_str(), (int)text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(incremental.parse(&json_definition), 0);

    for (int i = 0; i < 12; ++i) {
        size_t off = text.find("Hello") + 2;
        std::string ins(1, (i % 2 == 0) ? 'x' : 'y');
        ASSERT_EQ(incremental.parse_incremental(&json_definition, off, 0, ins.c_str(), 1, nullptr), 0);
        text.insert(off, ins);

        textparser::Parser full;
        ASSERT_EQ(full.openmem(text.c_str(), (int)text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(full.parse(&json_definition), 0);
        EXPECT_EQ(capture_snapshot(incremental), capture_snapshot(full)) << "iteration " << i;
    }
}

// Exact differential guard: every single-character insert and delete over a
// structural JSON sample must leave the snapshot equal to a full parse.
TEST(lexer_streams, all_single_char_edits_match_full_snapshot) {
    const char *base = "{\n  \"message\": \"Hello, world!\",\n  \"n\": 42,\n  \"arr\": [1, 2.5, -3]\n}\n";
    const char *chars[] = {"X", "9", ".", "-", "e", "\\", "\"", ",", "{", "[", ":", " ", "\n"};
    size_t mismatches = 0;
    size_t total = 0;

    for (size_t offset = 1; offset <= strlen(base); ++offset) {
        for (const char *ch : chars) {
            std::string edited(base);
            edited.insert(offset, ch);
            textparser::Parser probe;
            if (probe.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
            if (probe.parse(&json_definition) != 0) continue;

            textparser::Parser incremental;
            if (incremental.openmem(base, (int)strlen(base), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
            if (incremental.parse(&json_definition) != 0) continue;
            if (incremental.parse_incremental(&json_definition, offset, 0, ch, strlen(ch), nullptr) != 0) continue;

            textparser::Parser full;
            if (full.openmem(edited.c_str(), (int)edited.size(), TEXTPARSER_ENCODING_UTF_8) != 0) continue;
            if (full.parse(&json_definition) != 0) continue;

            ++total;
            if (!(capture_snapshot(incremental) == capture_snapshot(full))) ++mismatches;
        }
    }

    ASSERT_GT(total, 0u);
    EXPECT_EQ(mismatches, 0u);
}


// Phase 3: verify consecutive in-leaf edits correctly accumulate lazy position
// bias and materialize byte-for-byte identical to full re-parse when queried.
TEST(lexer_streams, consecutive_in_leaf_edits_accumulate_lazy_bias) {
    std::string text = "{\"first\": 12345, \"second\": 67890, \"third\": 11111}";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(text.c_str(), (int)text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&json_definition), 0);

    // Edit inside "12345" three times consecutively without intermediate snapshot query
    size_t off1 = text.find("12345") + 2;
    ASSERT_EQ(parser.parse_incremental(&json_definition, off1, 0, "99", 2, nullptr), 0);
    text.insert(off1, "99");

    size_t off2 = text.find("1299345") + 1;
    ASSERT_EQ(parser.parse_incremental(&json_definition, off2, 1, "8", 1, nullptr), 0);
    text.replace(off2, 1, "8");

    size_t off3 = text.find("1899345") + 3;
    ASSERT_EQ(parser.parse_incremental(&json_definition, off3, 2, nullptr, 0, nullptr), 0);
    text.erase(off3, 2);

    // Now query snapshot: it should lazily materialize and match full parse exactly
    textparser::Parser full;
    ASSERT_EQ(full.openmem(text.c_str(), (int)text.size(), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(full.parse(&json_definition), 0);

    EXPECT_EQ(capture_snapshot(parser), capture_snapshot(full));
}

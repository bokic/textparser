#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>
#include <bash_definition.json.h>

TEST(diagnostic_recovery, multi_diagnostic_reporting) {
    textparser_t handle = nullptr;
    const char *code = "line1\nline2 error_here\nline3";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_EQ(textparser_get_diagnostic_count(handle), 0u);

    // Report Error
    EXPECT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, "TS1005", "';' expected.", 12, 10), 0);
    // Report Warning
    EXPECT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_WARNING, "TS1100", "Unreachable code.", 23, 5), 0);

    EXPECT_EQ(textparser_get_diagnostic_count(handle), 2u);

    textparser_diagnostic diag0;
    EXPECT_EQ(textparser_get_diagnostic(handle, 0, &diag0), 0);
    EXPECT_EQ(diag0.severity, TEXTPARSER_SEVERITY_ERROR);
    EXPECT_STREQ(diag0.code, "TS1005");
    EXPECT_STREQ(diag0.message, "';' expected.");
    EXPECT_EQ(diag0.start_pos, 12u);
    EXPECT_EQ(diag0.length, 10u);
    EXPECT_EQ(diag0.line, 1u); // 0-based line 1 is 2nd line

    textparser_diagnostic diag1;
    EXPECT_EQ(textparser_get_diagnostic(handle, 1, &diag1), 0);
    EXPECT_EQ(diag1.severity, TEXTPARSER_SEVERITY_WARNING);
    EXPECT_STREQ(diag1.code, "TS1100");
    EXPECT_STREQ(diag1.message, "Unreachable code.");

    // Clear diagnostics
    textparser_clear_diagnostics(handle);
    EXPECT_EQ(textparser_get_diagnostic_count(handle), 0u);

    textparser_close(handle);
}

TEST(diagnostic_recovery, sync_token_recovery) {
    textparser_t handle = nullptr;
    const char *code = "foo bar; baz";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    // Set definition
    textparser_parse(handle, &json_definition);

    // Try recovery with empty sync array
    int sync_tokens[] = { -1 };
    size_t new_offset = 0;
    EXPECT_NE(textparser_recover_until_token(handle, sync_tokens, 0, &new_offset), 0);
    EXPECT_EQ(new_offset, strlen(code));

    textparser_close(handle);
}

TEST(diagnostic_recovery, bash_sync_token_recovery_semicolon) {
    textparser_t handle = nullptr;
    const char *code = "broken @@ syntax ; echo hello";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    textparser_parse(handle, &bash_definition);

    const int sync_tokens[] = { TextParser_bash_Semicolon, TextParser_END };
    size_t new_offset = 0;
    int res = textparser_recover_until_token(handle, sync_tokens, 0, &new_offset);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(new_offset, 17u);
    EXPECT_EQ(code[new_offset], ';');

    // Report recovered diagnostic for skipped span
    EXPECT_EQ(textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, "BASH_ERR",
                                           "Syntax error encountered; recovered at ';'", 0, new_offset), 0);
    EXPECT_EQ(textparser_get_diagnostic_count(handle), 1u);

    textparser_diagnostic diag;
    EXPECT_EQ(textparser_get_diagnostic(handle, 0, &diag), 0);
    EXPECT_EQ(diag.severity, TEXTPARSER_SEVERITY_ERROR);
    EXPECT_STREQ(diag.code, "BASH_ERR");
    EXPECT_EQ(diag.start_pos, 0u);
    EXPECT_EQ(diag.length, 17u);

    textparser_close(handle);
}

TEST(diagnostic_recovery, bash_sync_token_recovery_newline) {
    textparser_t handle = nullptr;
    const char *code = "broken_cmd arg1 ???\necho next_line";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    textparser_parse(handle, &bash_definition);

    const int sync_tokens[] = { TextParser_bash_Newline, TextParser_END };
    size_t new_offset = 0;
    int res = textparser_recover_until_token(handle, sync_tokens, 0, &new_offset);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(new_offset, 19u);
    EXPECT_EQ(code[new_offset], '\n');

    textparser_close(handle);
}

TEST(diagnostic_recovery, bash_multi_barrier_pipeline_recovery) {
    textparser_t handle = nullptr;
    const char *code = "bad1 | bad2 ; bad3 \n echo finished";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    textparser_parse(handle, &bash_definition);

    const int sync_tokens[] = {
        TextParser_bash_Pipe,
        TextParser_bash_Semicolon,
        TextParser_bash_Newline,
        TextParser_END
    };

    size_t offset = 0;
    // 1st recovery stops at '|' (offset 5)
    int res = textparser_recover_until_token(handle, sync_tokens, offset, &offset);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(offset, 5u);
    EXPECT_EQ(code[offset], '|');

    // Advance past '|'
    offset++;
    // 2nd recovery stops at ';' (offset 12)
    res = textparser_recover_until_token(handle, sync_tokens, offset, &offset);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(offset, 12u);
    EXPECT_EQ(code[offset], ';');

    // Advance past ';'
    offset++;
    // 3rd recovery stops at '\n' (offset 19)
    res = textparser_recover_until_token(handle, sync_tokens, offset, &offset);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(offset, 19u);
    EXPECT_EQ(code[offset], '\n');

    // Advance past '\n'
    offset++;
    // 4th recovery finds no more barrier tokens and reaches EOF
    res = textparser_recover_until_token(handle, sync_tokens, offset, &offset);
    EXPECT_NE(res, 0);
    EXPECT_EQ(offset, strlen(code));

    textparser_close(handle);
}

TEST(diagnostic_recovery, bash_sync_recovery_edge_conditions) {
    textparser_t handle = nullptr;
    const char *code = "; echo test";
    int err = textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    textparser_parse(handle, &bash_definition);

    const int sync_tokens[] = { TextParser_bash_Semicolon, TextParser_END };
    size_t offset = 0;

    // Edge condition 1: starting directly at barrier token
    int res = textparser_recover_until_token(handle, sync_tokens, 0, &offset);
    EXPECT_EQ(res, 0);
    EXPECT_EQ(offset, 0u);

    // Edge condition 2: starting at EOF
    res = textparser_recover_until_token(handle, sync_tokens, strlen(code), &offset);
    EXPECT_NE(res, 0);
    EXPECT_EQ(offset, strlen(code));

    // Edge condition 3: no barrier token found in input
    const char *nobarrier = "echo 1 2 3";
    textparser_t handle2 = nullptr;
    ASSERT_EQ(textparser_openmem(nobarrier, strlen(nobarrier), TEXTPARSER_ENCODING_LATIN1, &handle2), 0);
    textparser_parse(handle2, &bash_definition);
    res = textparser_recover_until_token(handle2, sync_tokens, 0, &offset);
    EXPECT_NE(res, 0);
    EXPECT_EQ(offset, strlen(nobarrier));
    textparser_close(handle2);

    // Edge condition 4: null argument validation
    EXPECT_EQ(textparser_recover_until_token(nullptr, sync_tokens, 0, &offset), -1);
    EXPECT_EQ(textparser_recover_until_token(handle, nullptr, 0, &offset), -1);
    EXPECT_EQ(textparser_recover_until_token(handle, sync_tokens, 0, nullptr), -1);

    textparser_close(handle);
}

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <json_definition.json.h>

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

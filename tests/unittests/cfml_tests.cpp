#include "tokenparser.hpp"
#include <textparser.h>

#include <gtest/gtest.h>

#include <cfml_definition.json.h>


TEST(parse_CFML, crash_cfset) {
    auto tokens = TextParser(R"(<!--- <!--- --->)", &cfml_definition);
}

TEST(parse_CFML, null_definition) {
    textparser_t handle = nullptr;
    int err = textparser_openmem("test", 4, TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    err = textparser_parse(handle, nullptr);
    EXPECT_EQ(err, -1);

    textparser_close(handle);
}

TEST(parse_CFML, basic_cfset) {
    auto tokens = TextParser(R"(<cfset a = 1234 />)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "StartTag");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   18);
    EXPECT_EQ   (tokens[0].children, 1);

    EXPECT_STREQ(tokens[0][0].type, "Expression");
    EXPECT_EQ   (tokens[0][0].position, 7);
    EXPECT_EQ   (tokens[0][0].length,   9);
    EXPECT_EQ   (tokens[0][0].children, 3);

    EXPECT_STREQ(tokens[0][0][0].type, "Variable");
    EXPECT_EQ   (tokens[0][0][0].position, 7);
    EXPECT_EQ   (tokens[0][0][0].length,   1);
    EXPECT_EQ   (tokens[0][0][0].children, 0);

    EXPECT_STREQ(tokens[0][0][1].type, "Operator");
    EXPECT_EQ   (tokens[0][0][1].position, 9);
    EXPECT_EQ   (tokens[0][0][1].length,   1);
    EXPECT_EQ   (tokens[0][0][1].children, 0);

    EXPECT_STREQ(tokens[0][0][2].type, "Number");
    EXPECT_EQ   (tokens[0][0][2].position, 11);
    EXPECT_EQ   (tokens[0][0][2].length,   4);
    EXPECT_EQ   (tokens[0][0][2].children, 0);
}

TEST(parse_CFML, line_mapping_subsystem) {
    const char *text = "line0\nline1\n\nline3\n";
    // offsets of newlines: 5, 11, 12, 18
    textparser_t handle = nullptr;
    int err = textparser_openmem(text, strlen(text), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    // Initial state: no line map
    EXPECT_EQ(textparser_get_line_count(handle), 1);
    EXPECT_EQ(textparser_get_line_start_position(handle, 0), 0);
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 10), 0);

    // Build the line map
    err = textparser_build_line_map(handle);
    ASSERT_EQ(err, 0);

    // Verify line count (4 newlines => 5 lines)
    EXPECT_EQ(textparser_get_line_count(handle), 5);

    // Verify line starting positions
    EXPECT_EQ(textparser_get_line_start_position(handle, 0), 0);  // line 0 start
    EXPECT_EQ(textparser_get_line_start_position(handle, 1), 6);  // line 1 start (after \n at 5)
    EXPECT_EQ(textparser_get_line_start_position(handle, 2), 12); // line 2 start (after \n at 11)
    EXPECT_EQ(textparser_get_line_start_position(handle, 3), 13); // line 3 start (after \n at 12)
    EXPECT_EQ(textparser_get_line_start_position(handle, 4), 19); // line 4 start (after \n at 18)

    // Verify line numbers at specific positions
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 0), 0);  // "l" in line0
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 5), 0);  // newline at 5 (terminates line 0)
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 6), 1);  // "l" in line1
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 11), 1); // newline at 11 (terminates line 1)
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 12), 2); // newline at 12 (terminates line 2)
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 13), 3); // "l" in line3
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 18), 3); // newline at 18 (terminates line 3)
    EXPECT_EQ(textparser_get_line_number_at_position(handle, 19), 4); // empty line 4 at the end

    textparser_close(handle);
}

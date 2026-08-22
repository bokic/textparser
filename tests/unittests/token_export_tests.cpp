#include <gtest/gtest.h>
#include <textparser.hpp>
#include <vector>
#include <string>
#include <cstring>

#include <c_definition.json.h>
#include <cfml_definition.json.h>
#include <json_definition.json.h>

TEST(token_export_tests, full_document_export_c) {
    const char *code = "int main() {\n    return 42;\n}\n";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t count = 0;
    // Step 1: Query token count
    EXPECT_EQ(parser.export_tokens(nullptr, 0, &count), 0);
    EXPECT_GT(count, 0u);

    std::vector<textparser_token_range> tokens(count);
    EXPECT_EQ(parser.export_tokens(tokens.data(), count, &count), 0);

    // Verify coordinates and ordering
    size_t last_pos = 0;
    for (size_t i = 0; i < count; i++) {
        EXPECT_GE(tokens[i].start_pos, last_pos);
        EXPECT_GT(tokens[i].length, 0u);
        EXPECT_LE(tokens[i].start_line, tokens[i].end_line);
        last_pos = tokens[i].start_pos;
    }

    // Verify first token is "int" keyword at Line 0, Col 0
    EXPECT_EQ(tokens[0].start_line, 0u);
    EXPECT_EQ(tokens[0].start_col, 0u);
    EXPECT_EQ(tokens[0].length, 3u);
}

TEST(token_export_tests, byte_range_export) {
    const char *code = "<cfset a = 10 /><cfset b = 20 /><cfset c = 30 />";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&cfml_definition), 0);

    size_t total_count = 0;
    parser.export_tokens(nullptr, 0, &total_count);
    EXPECT_GT(total_count, 1u);

    // Query range for the middle tag "<cfset b = 20 />" (starts at offset 16)
    size_t range_count = 0;
    EXPECT_EQ(parser.export_tokens_range(16, 32, nullptr, 0, &range_count), 0);
    EXPECT_GT(range_count, 0u);
    EXPECT_LT(range_count, total_count);

    std::vector<textparser_token_range> range_tokens(range_count);
    EXPECT_EQ(parser.export_tokens_range(16, 32, range_tokens.data(), range_count, &range_count), 0);

    for (const auto &tok : range_tokens) {
        EXPECT_GE(tok.start_pos + tok.length, 16u);
        EXPECT_LE(tok.start_pos, 32u);
    }
}

TEST(token_export_tests, line_range_export) {
    const char *c_code = "int a = 1;\nint b = 2;\nint c = 3;\nint d = 4;\n";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(c_code, (int)strlen(c_code), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t total_count = 0;
    parser.export_tokens(nullptr, 0, &total_count);
    EXPECT_GT(total_count, 0u);

    size_t count_l1_l2 = 0;
    // Export lines 1 to 2 ("int b = 2;\nint c = 3;\n")
    EXPECT_EQ(parser.export_tokens_lines(1, 2, nullptr, 0, &count_l1_l2), 0);
    EXPECT_GT(count_l1_l2, 0u);
    EXPECT_LT(count_l1_l2, total_count);

    std::vector<textparser_token_range> tokens(count_l1_l2);
    EXPECT_EQ(parser.export_tokens_lines(1, 2, tokens.data(), count_l1_l2, &count_l1_l2), 0);

    for (const auto &tok : tokens) {
        EXPECT_GE(tok.start_line, 1u);
        EXPECT_LE(tok.start_line, 2u);
    }
}

TEST(token_export_tests, buffer_overflow_handling) {
    const char *code = "int a = 1; int b = 2; int c = 3;";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t count = 0;
    parser.export_tokens(nullptr, 0, &count);
    ASSERT_GT(count, 2u);

    // Provide buffer of size 2 (smaller than total tokens)
    textparser_token_range small_buf[2];
    size_t req_count = 0;
    int res = parser.export_tokens(small_buf, 2, &req_count);
    EXPECT_EQ(res, -2); // Buffer too small error code
    EXPECT_EQ(req_count, count); // Still returns required count
}

TEST(token_export_tests, empty_file_handling) {
    const char *code = "";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, 0, TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t count = 0;
    EXPECT_EQ(parser.export_tokens(nullptr, 0, &count), 0);
    EXPECT_EQ(count, 0u);
}

TEST(token_export_tests, string_token_color_inheritance) {
    const char *code = "const char *str = \"Hello world\";";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t count = 0;
    ASSERT_EQ(parser.export_tokens(nullptr, 0, &count), 0);
    ASSERT_GT(count, 0u);

    std::vector<textparser_token_range> tokens(count);
    ASSERT_EQ(parser.export_tokens(tokens.data(), count, &count), 0);

    // Verify DataType (char) color is 0x4ec9b0 and Keyword (const) color is 0xc586c0
    // Verify DoubleString (containing "Hello world") color is 0xce9178
    bool found_char_type = false;
    bool found_const_kw = false;
    bool found_string = false;

    for (const auto &tok : tokens) {
        if (tok.start_pos == 0 && tok.length == 5) { // "const"
            EXPECT_EQ(tok.text_color, 0xc586c0);
            found_const_kw = true;
        }
        if (tok.start_pos == 6 && tok.length == 4) { // "char"
            EXPECT_EQ(tok.text_color, 0x4ec9b0);
            found_char_type = true;
        }
        if (tok.start_pos >= 18 && tok.start_pos + tok.length <= 31) { // inside "Hello world"
            EXPECT_EQ(tok.text_color, 0xce9178);
            found_string = true;
        }
    }

    EXPECT_TRUE(found_const_kw);
    EXPECT_TRUE(found_char_type);
    EXPECT_TRUE(found_string);
}

TEST(token_export_tests, tagged_type_and_type_suffix_coloring) {
    const char *code = "enum textparser_encoding encoding, textparser_t handle;";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    size_t count = 0;
    ASSERT_EQ(parser.export_tokens(nullptr, 0, &count), 0);
    ASSERT_GT(count, 0u);

    std::vector<textparser_token_range> tokens(count);
    ASSERT_EQ(parser.export_tokens(tokens.data(), count, &count), 0);

    // "enum" -> TagSpecifier (0xc586c0)
    // "textparser_encoding" -> TypeName inside TaggedType (0x4ec9b0)
    // "encoding" -> Variable (0x9cdcfe)
    // "textparser_t" -> DataType due to _t suffix (0x4ec9b0)
    // "handle" -> Variable (0x9cdcfe)
    bool found_enum_kw = false;
    bool found_tag_type = false;
    bool found_encoding_var = false;
    bool found_custom_t_type = false;
    bool found_handle_var = false;

    for (const auto &tok : tokens) {
        if (tok.start_pos == 0 && tok.length == 4) { // "enum"
            EXPECT_EQ(tok.text_color, 0xc586c0);
            found_enum_kw = true;
        }
        if (tok.start_pos == 5 && tok.length == 19) { // "textparser_encoding"
            EXPECT_EQ(tok.text_color, 0x4ec9b0);
            found_tag_type = true;
        }
        if (tok.start_pos == 25 && tok.length == 8) { // "encoding"
            EXPECT_EQ(tok.text_color, 0x9cdcfe);
            found_encoding_var = true;
        }
        if (tok.start_pos == 35 && tok.length == 12) { // "textparser_t"
            EXPECT_EQ(tok.text_color, 0x4ec9b0);
            found_custom_t_type = true;
        }
        if (tok.start_pos == 48 && tok.length == 6) { // "handle"
            EXPECT_EQ(tok.text_color, 0x9cdcfe);
            found_handle_var = true;
        }
    }

    EXPECT_TRUE(found_enum_kw);
    EXPECT_TRUE(found_tag_type);
    EXPECT_TRUE(found_encoding_var);
    EXPECT_TRUE(found_custom_t_type);
    EXPECT_TRUE(found_handle_var);
}

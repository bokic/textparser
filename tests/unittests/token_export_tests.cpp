#include <gtest/gtest.h>
#include <textparser.hpp>
#include <vector>
#include <string>
#include <cstring>

#include <c_definition.json.h>
#include <cfml_definition.json.h>
#include <json_definition.json.h>
#include <textparser-json.h>

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
        if (tok.start_pos == 18 && tok.length == 1) { // start quote `"`
            EXPECT_EQ(tok.text_color, 0x9e6a57);
        }
        if (tok.start_pos == 19 && tok.length == 5) { // "Hello" body
            EXPECT_EQ(tok.text_color, 0xce9178);
            found_string = true;
        }
        if (tok.start_pos == 25 && tok.length == 5) { // "world" body
            EXPECT_EQ(tok.text_color, 0xce9178);
        }
        if (tok.start_pos == 30 && tok.length == 1) { // end quote `"`
            EXPECT_EQ(tok.text_color, 0x9e6a57);
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

TEST(token_export_tests, delimiter_coloring_string_literal) {
    const char *json_def_str = R"({
        "name": "delim_lang",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": ["StringLiteral"],
        "otherTextInside": true,
        "tokens": {
            "StringLiteral": {
                "type": "StartStop",
                "startRegex": "\"",
                "endRegex": "\"",
                "otherTextInside": true,
                "textColor": "0xce9178",
                "textBackground": "0x222222",
                "delimiterTextColor": "0x808080",
                "delimiterTextBackground": "0x112233",
                "delimiterTextFlags": "1"
            }
        }
    })";

    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(json_def_str, &definition), 0);
    ASSERT_NE(definition, nullptr);

    const char *code = "\"hello\"";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, definition), 0);

    const textparser_token_item *root = textparser_get_first_token(handle);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(textparser_get_token_text_color(root), 0xce9178);

    // Direct AST delimiter checking
    ASSERT_NE(root->child, nullptr);
    EXPECT_EQ(root->child->token_id, TEXTPARSER_TOKEN_ID_START_DELIMITER);
    EXPECT_EQ(textparser_get_token_text_color(root->child), 0x808080);
    EXPECT_EQ(textparser_get_token_text_background(root->child), 0x112233);
    EXPECT_EQ(textparser_get_token_text_flags(root->child), 1u);

    // Token range export checking
    size_t count = 0;
    ASSERT_EQ(textparser_export_tokens(handle, nullptr, 0, &count), 0);
    ASSERT_EQ(count, 3u);

    std::vector<textparser_token_range> ranges(count);
    ASSERT_EQ(textparser_export_tokens(handle, ranges.data(), count, &count), 0);

    // Start delimiter `"`
    EXPECT_EQ(ranges[0].start_pos, 0u);
    EXPECT_EQ(ranges[0].length, 1u);
    EXPECT_EQ(ranges[0].token_id, TEXTPARSER_TOKEN_ID_START_DELIMITER);
    EXPECT_EQ(ranges[0].text_color, 0x808080);
    EXPECT_EQ(ranges[0].text_background, 0x112233);
    EXPECT_EQ(ranges[0].text_flags, 1u);

    // String content `hello`
    EXPECT_EQ(ranges[1].start_pos, 1u);
    EXPECT_EQ(ranges[1].length, 5u);
    EXPECT_EQ(ranges[1].token_id, TEXTPARSER_TOKEN_ID_UNPROCESSED);
    EXPECT_EQ(ranges[1].text_color, 0xce9178);
    EXPECT_EQ(ranges[1].text_background, 0x222222);

    // End delimiter `"`
    EXPECT_EQ(ranges[2].start_pos, 6u);
    EXPECT_EQ(ranges[2].length, 1u);
    EXPECT_EQ(ranges[2].token_id, TEXTPARSER_TOKEN_ID_END_DELIMITER);
    EXPECT_EQ(ranges[2].text_color, 0x808080);
    EXPECT_EQ(ranges[2].text_background, 0x112233);
    EXPECT_EQ(ranges[2].text_flags, 1u);

    textparser_close(handle);
    textparser_free_language_definition(definition);
}

TEST(token_export_tests, delimiter_coloring_empty_string_edge_case) {
    const char *json_def_str = R"({
        "name": "delim_lang2",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["txt"],
        "defaultTextEncoding": "utf-8",
        "startTokens": ["StringLiteral"],
        "otherTextInside": true,
        "tokens": {
            "StringLiteral": {
                "type": "StartStop",
                "startRegex": "\"",
                "endRegex": "\"",
                "otherTextInside": true,
                "textColor": "0xce9178",
                "delimiterTextColor": "0x808080"
            }
        }
    })";

    textparser_language_definition *definition = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_string(json_def_str, &definition), 0);
    ASSERT_NE(definition, nullptr);

    const char *code = "\"\"";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
    ASSERT_EQ(textparser_parse(handle, definition), 0);

    size_t count = 0;
    ASSERT_EQ(textparser_export_tokens(handle, nullptr, 0, &count), 0);
    ASSERT_EQ(count, 2u);

    std::vector<textparser_token_range> ranges(count);
    ASSERT_EQ(textparser_export_tokens(handle, ranges.data(), count, &count), 0);

    // Start delimiter `"`
    EXPECT_EQ(ranges[0].start_pos, 0u);
    EXPECT_EQ(ranges[0].length, 1u);
    EXPECT_EQ(ranges[0].text_color, 0x808080);

    // End delimiter `"`
    EXPECT_EQ(ranges[1].start_pos, 1u);
    EXPECT_EQ(ranges[1].length, 1u);
    EXPECT_EQ(ranges[1].text_color, 0x808080);

    textparser_close(handle);
    textparser_free_language_definition(definition);
}


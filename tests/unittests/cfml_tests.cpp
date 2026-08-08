#include "tokenparser.hpp"
#include <textparser.h>
#include <textparser-json.h>

#include <gtest/gtest.h>

#include <cfml_definition.json.h>
#include <cfml.h>


TEST(parse_CFML, crash_cfset) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<!--- <!--- --->)", &cfml_definition);
    textparser_suppress_errors() = false;
}

TEST(parse_CFML, error_resynchronization_and_merging) {
    textparser_t handle = nullptr;
    const char *text = "<cfset a = 1234 /> @@@@ <cfset b = 5678 />";
    int err = textparser_openmem(text, strlen(text), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, nullptr);

    err = textparser_parse(handle, &cfml_definition);
    EXPECT_EQ(err, 0);

    const textparser_token_item *item = textparser_get_first_token(handle);
    ASSERT_NE(item, nullptr);
    EXPECT_NE(item->token_id, TEXTPARSER_TOKEN_ID_ERROR); // First valid tag

    item = textparser_get_token_next(item);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->token_id, TEXTPARSER_TOKEN_ID_ERROR); // Merged error node for "@@@@"
    EXPECT_EQ(item->len, 4);

    item = textparser_get_token_next(item);
    ASSERT_NE(item, nullptr);
    EXPECT_NE(item->token_id, TEXTPARSER_TOKEN_ID_ERROR); // Second valid tag after error resynchronization

    textparser_close(handle);
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

// Helper function to scan tokens and check if a specific token type exists
static void scan_tokens_for_type(const TokenParserItem &item, const std::string &target_type, bool &found) {
    if (found) return;
    if (item.type && target_type == item.type) {
        found = true;
        return;
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens_for_type(item[i], target_type, found);
    }
}

static bool item_has_token_type(const TokenParserItem &item, const std::string &target_type) {
    bool found = false;
    scan_tokens_for_type(item, target_type, found);
    return found;
}

static bool has_token_type(const TextParser &tokens, const std::string &target_type) {
    bool found = false;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens_for_type(tokens[i], target_type, found);
    }
    return found;
}

// Dump helper for visual debugging and validation
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
static std::string dump_tokens(const TokenParserItem &item, int indent = 0) {
    if (!item.type) return "";
    std::string spaces(indent * 2, ' ');
    std::string result = spaces + item.type + " [" + std::to_string(item.position) + ", " + std::to_string(item.length) + "]";
    if (!item.value.empty()) {
        result += " '" + item.value + "'";
    }
    result += "\n";
    for (size_t i = 0; i < item.children; ++i) {
        result += dump_tokens(item[i], indent + 1);
    }
    return result;
}
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// -------------------------------------------------------------
// 1. COMMENT CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, comments_nested) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<!--- outer <!--- inner ---> outer --->)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    // We expect 1 top-level Comment token
    EXPECT_EQ(tokens.count, 1);
    if (tokens.count > 0) {
        EXPECT_STREQ(tokens[0].type, "Comment");
    }
}

TEST(parse_CFML, comments_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<!--- unclosed comment)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, comments_script) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>
        /* block comment */
        // line comment
        x = 1;
    </cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "ScriptBlockComment"));
    EXPECT_TRUE(has_token_type(tokens, "ScriptLineComment"));
}

// -------------------------------------------------------------
// 2. STRING & ESCAPED QUOTE CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, string_double_escaped) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "hello ""world""!" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "DoubleChar")); // "" should be recognized
}

TEST(parse_CFML, string_single_escaped) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = 'hello ''world''!' />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "SingleChar")); // '' should be recognized
}

TEST(parse_CFML, string_mixed_quotes) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cfset x = "hello 'world'!" />)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cfset x = 'hello "world"!' />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    
    EXPECT_TRUE(has_token_type(tokens1, "DoubleString"));
    EXPECT_TRUE(has_token_type(tokens2, "SingleString"));
}

TEST(parse_CFML, string_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "unclosed string />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

// -------------------------------------------------------------
// 3. SHARP EXPRESSION CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, sharp_in_double_string) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "hello #name#" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, sharp_in_single_string) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = 'hello #name#' />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, sharp_escaped) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "hello ##name##" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "SharpChar")); // Should be recognized as ##
}

TEST(parse_CFML, sharp_outside_string) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = #myVar# />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, sharp_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "hello #unclosed" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, sharp_nested_expression) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "#func(arg1, "#nested#")#" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

// -------------------------------------------------------------
// 4. CFSCRIPT CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, cfscript_basic) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>
        x = 1;
        y = 2;
    </cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_EQ(tokens.count, 1);
    EXPECT_TRUE(has_token_type(tokens, "ScriptStartTag"));
    EXPECT_TRUE(has_token_type(tokens, "ScriptEndTag"));
}

TEST(parse_CFML, cfscript_complex) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>
        if (x) {
            myStruct = { a = 1, b = 2 };
        }
    </cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, cfscript_operators) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>
        x += 1;
        y -= 1;
        z = a && b;
    </cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

static void find_token_value(const TokenParserItem &item, const std::string &target_type, const std::string &target_value, bool &found) {
    if (item.type && target_type == item.type && target_value == item.value) {
        found = true;
    }
    for (size_t i = 0; i < item.children; ++i) {
        find_token_value(item[i], target_type, target_value, found);
    }
}

static bool has_token_value(const TextParser &tokens, const std::string &target_type, const std::string &target_value) {
    bool found = false;
    for (size_t i = 0; i < tokens.count; ++i) {
        find_token_value(tokens[i], target_type, target_value, found);
    }
    return found;
}

TEST(parse_CFML, symbolic_and_operator) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>writeOutput(1 && 3);</cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;

    ASSERT_EQ(tokens.count, 1);
    ASSERT_STREQ(tokens[0].type, "ScriptTagPair");
    ASSERT_EQ(tokens[0].children, 3);
    ASSERT_STREQ(tokens[0][1].type, "ScriptExpression");

    // ScriptExpression: [0] Function, [1] Parenthesis, [2] ExpressionEnd
    ASSERT_STREQ(tokens[0][1][0].type, "Function");
    ASSERT_STREQ(tokens[0][1][1].type, "Parenthesis");
    ASSERT_STREQ(tokens[0][1][2].type, "ExpressionEnd");

    // Parenthesis: [0] ScriptExpression: Number Operator Number
    ASSERT_STREQ(tokens[0][1][1][0].type, "ScriptExpression");
    ASSERT_EQ(tokens[0][1][1][0].children, 3);
    ASSERT_STREQ(tokens[0][1][1][0][0].type, "Number");
    ASSERT_STREQ(tokens[0][1][1][0][1].type, "Operator");
    EXPECT_EQ(tokens[0][1][1][0][1].length, 2);
    EXPECT_STREQ(tokens[0][1][1][0][1].value.c_str(), "&&");
    ASSERT_STREQ(tokens[0][1][1][0][2].type, "Number");

    EXPECT_TRUE(has_token_value(tokens, "Operator", "&&"));
    EXPECT_FALSE(has_token_value(tokens, "Operator", "&"));
}

TEST(parse_CFML, symbolic_or_operator) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>writeOutput(0 || 3);</cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;

    ASSERT_EQ(tokens.count, 1);
    ASSERT_STREQ(tokens[0].type, "ScriptTagPair");
    ASSERT_EQ(tokens[0].children, 3);
    ASSERT_STREQ(tokens[0][1].type, "ScriptExpression");

    // Parenthesis: [0] ScriptExpression: Number Operator Number
    ASSERT_STREQ(tokens[0][1][1].type, "Parenthesis");
    ASSERT_STREQ(tokens[0][1][1][0].type, "ScriptExpression");
    ASSERT_EQ(tokens[0][1][1][0].children, 3);
    ASSERT_STREQ(tokens[0][1][1][0][0].type, "Number");
    ASSERT_STREQ(tokens[0][1][1][0][1].type, "Operator");
    EXPECT_EQ(tokens[0][1][1][0][1].length, 2);
    EXPECT_STREQ(tokens[0][1][1][0][1].value.c_str(), "||");
    ASSERT_STREQ(tokens[0][1][1][0][2].type, "Number");

    EXPECT_TRUE(has_token_value(tokens, "Operator", "||"));
}

TEST(parse_CFML, symbolic_logical_sharp_expression) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfoutput>#0 || 3#</cfoutput>)", &cfml_definition);
    textparser_suppress_errors() = false;

    ASSERT_EQ(tokens.count, 1);
    ASSERT_STREQ(tokens[0].type, "OutputTagPair");
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
    EXPECT_TRUE(has_token_value(tokens, "Operator", "||"));
}

TEST(parse_CFML, sharp_leading_string_with_interpolation) {
    // A leading '#' that cannot close as a #...# expression is treated as
    // literal text; the string's #...# interpolation still tokenizes.
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfoutput>#"x #a#"</cfoutput>)", &cfml_definition);
    textparser_suppress_errors() = false;

    ASSERT_EQ(tokens.count, 1);
    ASSERT_STREQ(tokens[0].type, "OutputTagPair");
    EXPECT_TRUE(has_token_type(tokens, "OutputStartTag"));
    EXPECT_TRUE(has_token_type(tokens, "OutputEndTag"));
    EXPECT_TRUE(has_token_value(tokens, "SharpExpression", "#a#"));
}

TEST(parse_CFML, sharp_wrapped_string_with_interpolation) {
    // A closed #"..."# expression wrapping a string still parses as a
    // SharpExpression containing a DoubleString with a nested interpolation.
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfoutput>#"x #a#"#</cfoutput>)", &cfml_definition);
    textparser_suppress_errors() = false;

    ASSERT_EQ(tokens.count, 1);
    ASSERT_STREQ(tokens[0].type, "OutputTagPair");
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
    EXPECT_TRUE(has_token_type(tokens, "DoubleString"));
    EXPECT_TRUE(has_token_value(tokens, "SharpExpression", "#a#"));
}

// -------------------------------------------------------------
// 5. CFML TAG CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, tag_nested_cfoutput) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfoutput>outer <cfoutput>inner #var#</cfoutput> end</cfoutput>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, tag_nested_cfloop) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfloop>outer <cfloop>inner</cfloop> end</cfloop>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, tag_nested_cfquery) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfquery name="q">SELECT * FROM t WHERE id = #id#</cfquery>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    EXPECT_TRUE(has_token_type(tokens, "QueryStartTag"));
    EXPECT_TRUE(has_token_type(tokens, "QueryEndTag"));
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, tag_custom_tags) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cf_mycustomtag attr="val">body</cf_mycustomtag>)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cf_mycustomtag attr="val" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    
    EXPECT_TRUE(has_token_type(tokens1, "StartTag"));
    EXPECT_TRUE(has_token_type(tokens1, "EndTag"));
    EXPECT_TRUE(has_token_type(tokens2, "StartTag"));
}

TEST(parse_CFML, tag_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cf_mycustomtag attr="val">)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

// -------------------------------------------------------------
// 6. COMPLEX EXPRESSION CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, expr_word_operators) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cfset x = a is not b />)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cfset x = a contains b />)", &cfml_definition);
    auto tokens3 = TextParser(R"(<cfset x = a greater than or equal to b />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, expr_arithmetic_precedence) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = a + b * c - d / e />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, expr_function_calls) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = func1(func2(a, b), c) />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, expr_array_struct_access) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cfset x = array[index] />)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cfset x = struct.member />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, expr_numbers) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cfset x = +1.23 />)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cfset x = -.45 />)", &cfml_definition);
    auto tokens3 = TextParser(R"(<cfset x = 123. />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
}

TEST(parse_CFML, validation_closing_tags) {
    // 1. Valid closed component
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfcomponent></cfcomponent>", 28, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }
    
    // 2. Missing closing tag for cfcomponent
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfcomponent>", 13, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "CFML tag [cfcomponent] requires a closing tag </cfcomponent>");
        
        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 3. Forbidden end tag cfabort
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfabort></cfabort>", 19, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Ending tag </cfabort> is forbidden");
        
        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 4. End tag without start tag
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("</cfcomponent>", 14, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Ending tag </cfcomponent> has no matching start tag");
        
        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 5. Self-closing start tag
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfcomponent />", 15, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }
}

TEST(parse_CFML, validation_functions) {
    // 1. Valid CFML function call
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfset x = acos(0.5) />", 23, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }
    
    // 2. Unknown CFML function call
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfset x = nonExistingFunc() />", 31, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Unknown CFML function: [nonExistingFunc]");
        
        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 3. CFML function call with insufficient arguments
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfset x = acos() />", 20, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Function [acos] requires at least 1 arguments, but 0 were provided");
        
        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 4. CFML function call with excessive arguments
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfset x = acos(0.5, 0.6) />", 28, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "Function [acos] takes at most 1 arguments, but 2 were provided");
        
        textparser_validation_clear(validation);
        textparser_close(handle);
    }

    // 5. Valid nested CFML function call
    {
        textparser_t handle = nullptr;
        int res = textparser_openmem("<cfset x = acos(sin(0.5)) />", 28, TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);
        
        textparser_validation *validation = textparser_validate_cfml(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }
}

TEST(parse_CFML, validation_cfprocessingdirective_position) {
    // 1. Within first 4096 bytes (valid)
    {
        std::string content = std::string(3000, ' ') + "<cfprocessingdirective pageEncoding=\"utf-8\" />";
        textparser_t handle = nullptr;
        int res = textparser_openmem(content.c_str(), content.length(), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_cfml(handle);
        EXPECT_EQ(validation, nullptr);
        textparser_close(handle);
    }

    // 2. Starts/ends > 4096 bytes (invalid)
    {
        std::string content = std::string(4090, ' ') + "<cfprocessingdirective pageEncoding=\"utf-8\" />";
        textparser_t handle = nullptr;
        int res = textparser_openmem(content.c_str(), content.length(), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, 0);

        textparser_validation *validation = textparser_validate_cfml(handle);
        ASSERT_NE(validation, nullptr);
        EXPECT_EQ(validation->len, 1);
        EXPECT_EQ(validation->items[0]->type, TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR);
        EXPECT_STREQ(validation->items[0]->text, "cfprocessingdirective should be located within first 4096 bytes of the file");

        textparser_validation_clear(validation);
        textparser_close(handle);
    }
}

TEST(parse_CFML, get_encoding_cfprocessingdirective) {
    // 1. utf-8 pageEncoding
    {
        const char *content = "  <cfprocessingdirective pageEncoding=\"utf-8\" />";
        textparser_t handle = nullptr;
        int res = textparser_openmem(content, strlen(content), TEXTPARSER_ENCODING_UTF_8, &handle);
        ASSERT_EQ(res, 0);
        enum textparser_encoding enc = textparser_get_encoding_cfml(handle);
        EXPECT_EQ(enc, TEXTPARSER_ENCODING_UTF_8);
        textparser_close(handle);
    }

    // 2. utf-16 pageEncoding
    {
        const char *content = "<cfprocessingdirective pageEncoding='utf-16' />";
        textparser_t handle = nullptr;
        int res = textparser_openmem(content, strlen(content), TEXTPARSER_ENCODING_UTF_8, &handle);
        ASSERT_EQ(res, 0);
        enum textparser_encoding enc = textparser_get_encoding_cfml(handle);
        EXPECT_EQ(enc, TEXTPARSER_ENCODING_UTF_16);
        textparser_close(handle);
    }

    // 3. iso-8859-1 pageEncoding
    {
        const char *content = "<cfprocessingdirective pageencoding=\"iso-8859-1\">";
        textparser_t handle = nullptr;
        int res = textparser_openmem(content, strlen(content), TEXTPARSER_ENCODING_UTF_8, &handle);
        ASSERT_EQ(res, 0);
        enum textparser_encoding enc = textparser_get_encoding_cfml(handle);
        EXPECT_EQ(enc, TEXTPARSER_ENCODING_LATIN1);
        textparser_close(handle);
    }

    // 4. Default fallback when tag not present or past 4096 bytes
    {
        std::string content = std::string(4100, ' ') + "<cfprocessingdirective pageEncoding=\"utf-16\" />";
        textparser_t handle = nullptr;
        int res = textparser_openmem(content.c_str(), content.length(), TEXTPARSER_ENCODING_UTF_8, &handle);
        ASSERT_EQ(res, 0);
        enum textparser_encoding enc = textparser_get_encoding_cfml(handle);
        EXPECT_EQ(enc, TEXTPARSER_ENCODING_UTF_8);
        textparser_close(handle);
    }
}

TEST(parse_CFML, validation_tag_at_eof) {
    // 1. Start tag at EOF
    {
        const char *input = "<cfoutput";
        textparser_t handle = nullptr;
        int res = textparser_openmem(input, strlen(input), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, -1);
        ASSERT_NE(textparser_get_first_token(handle), nullptr);

        textparser_validation *validation = textparser_validate_cfml(handle);
        if (validation) {
            textparser_validation_clear(validation);
        }
        textparser_close(handle);
    }

    // 2. End tag at EOF
    {
        const char *input = "</cfcomponent";
        textparser_t handle = nullptr;
        int res = textparser_openmem(input, strlen(input), TEXTPARSER_ENCODING_LATIN1, &handle);
        ASSERT_EQ(res, 0);
        res = textparser_parse(handle, &cfml_definition);
        ASSERT_EQ(res, -1);
        ASSERT_NE(textparser_get_first_token(handle), nullptr);

        textparser_validation *validation = textparser_validate_cfml(handle);
        if (validation) {
            textparser_validation_clear(validation);
        }
        textparser_close(handle);
    }
}

TEST(parse_CFML, openmem_invalid_params) {
    textparser_t handle = nullptr;
    // 1. Negative length
    int res = textparser_openmem("test", -1, TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(res, -1);
    ASSERT_EQ(handle, nullptr);

    // 2. nullptr handle
    res = textparser_openmem("test", 4, TEXTPARSER_ENCODING_LATIN1, nullptr);
    ASSERT_EQ(res, -1);

    // 3. nullptr text
    res = textparser_openmem(nullptr, 4, TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(res, -1);
    ASSERT_EQ(handle, nullptr);
}

TEST(parse_CFML, context_nested_tokens_cfloop_top_level) {
    // At top level, cfloop body hash should NOT parse as SharpExpression
    auto tokens = TextParser(R"(<cfloop from="1" to="2" index="i">#i#</cfloop>)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "LoopTagPair");
    EXPECT_FALSE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, context_nested_tokens_cfloop_under_cfoutput) {
    // Under cfoutput, cfloop body hash SHOULD parse as SharpExpression
    auto tokens = TextParser(R"(<cfoutput><cfloop from="1" to="2" index="i">#i#</cfloop></cfoutput>)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "OutputTagPair");
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, context_nested_tokens_cfloop_under_cfquery) {
    // Under cfquery, cfloop body hash SHOULD parse as SharpExpression
    auto tokens = TextParser(R"(<cfquery name="q"><cfloop from="1" to="2" index="i">#i#</cfloop></cfquery>)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "QueryTagPair");
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

// -------------------------------------------------------------
// CORNER CASES & EDGE CONDITIONS FOR contextNestedTokens
// -------------------------------------------------------------

TEST(parse_CFML, context_nested_tokens_deeply_nested_loops) {
    // Deeply nested cfloop inside cfoutput: SharpExpression must propagate through multi-level ancestors
    auto tokens = TextParser(R"(<cfoutput><cfloop index="a" from="1" to="2"><cfloop index="b" from="1" to="2">#a#_#b#</cfloop></cfloop></cfoutput>)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);
    EXPECT_STREQ(tokens[0].type, "OutputTagPair");
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, context_nested_tokens_escaped_hash_in_cfloop) {
    // Escaped hash ## inside cfloop under cfoutput vs top-level
    auto tokens_top = TextParser(R"(<cfloop index="i" from="1" to="2">##i##</cfloop>)", &cfml_definition);
    EXPECT_FALSE(has_token_type(tokens_top, "SharpChar"));

    auto tokens_out = TextParser(R"(<cfoutput><cfloop index="i" from="1" to="2">##i##</cfloop></cfoutput>)", &cfml_definition);
    EXPECT_TRUE(has_token_type(tokens_out, "SharpChar"));
}

TEST(parse_CFML, context_nested_tokens_attribute_hashes_always_active) {
    // Hashes in tag attributes (e.g. to="#max#") are parsed as SharpExpression inside Expression even at top-level
    auto tokens = TextParser(R"(<cfloop from="1" to="#max#" index="i">literal_body_#i#</cfloop>)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);

    // LoopStartTag -> Expression -> SharpExpression for to="#max#"
    EXPECT_TRUE(item_has_token_type(tokens[0][0], "SharpExpression"));

    // LoopExpression (body) should NOT contain SharpExpression at top-level
    EXPECT_FALSE(item_has_token_type(tokens[0][1], "SharpExpression"));
}

TEST(parse_CFML, context_nested_tokens_runtime_json_load) {
    // Test that contextNestedTokens parsed from runtime JSON string behaves identically
    const char *custom_json = R"json({
        "name": "custom_cf",
        "caseSensitivity": false,
        "otherTextInside": true,
        "defaultFileExtensions": ["cfm"],
        "startTokens": ["OutputTagPair", "LoopTagPair"],
        "tokens": {
            "OutputTagPair": {
                "type": "GroupAllChildrenInSameOrder",
                "otherTextInside": true,
                "multiLine": true,
                "nestedTokens": ["OutputStartTag", "OutputExpr", "OutputEndTag"]
            },
            "OutputStartTag": { "type": "StartStop", "startRegex": "<cfoutput(?=[>\\s])", "endRegex": "/?>", "multiLine": true },
            "OutputEndTag": { "type": "SimpleToken", "startRegex": "</cfoutput>" },
            "OutputExpr": {
                "type": "Group",
                "otherTextInside": true,
                "multiLine": true,
                "nestedTokens": ["LoopTagPair", "SharpExpr"]
            },
            "LoopTagPair": {
                "type": "GroupAllChildrenInSameOrder",
                "otherTextInside": true,
                "multiLine": true,
                "nestedTokens": ["LoopStartTag", "LoopExpr", "LoopEndTag"]
            },
            "LoopStartTag": { "type": "StartStop", "startRegex": "<cfloop(?=[>\\s])", "endRegex": "/?>", "multiLine": true },
            "LoopEndTag": { "type": "SimpleToken", "startRegex": "</cfloop>" },
            "LoopExpr": {
                "type": "Group",
                "otherTextInside": true,
                "multiLine": true,
                "nestedTokens": ["CommentToken"],
                "contextNestedTokens": [
                    {
                        "whenParentIn": ["OutputExpr", "OutputTagPair"],
                        "nestedTokens": ["CommentToken", "SharpExpr"]
                    }
                ]
            },
            "CommentToken": { "type": "SimpleToken", "startRegex": "<!--.*?-->" },
            "SharpExpr": {
                "type": "StartStop",
                "startRegex": "#",
                "endRegex": "#",
                "multiLine": true
            }
        }
    })json";

    textparser_language_definition *runtime_def = nullptr;
    int err = textparser_json_load_language_definition_from_string(custom_json, &runtime_def);
    ASSERT_EQ(err, 0);
    ASSERT_NE(runtime_def, nullptr);

    // Test 1: Top-level cfloop body hash with runtime def -> no SharpExpr
    {
        textparser_t h = nullptr;
        const char *code = "<cfloop>#x#</cfloop>";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &h), 0);
        int parse_err = textparser_parse(h, runtime_def);
        if (parse_err != 0) {
            std::cout << "Parse Error: " << textparser_parse_error(h) << " at " << textparser_parse_error_position(h) << std::endl;
        }
        ASSERT_EQ(parse_err, 0);
        TextParser tp_wrap(code, runtime_def);
        EXPECT_FALSE(has_token_type(tp_wrap, "SharpExpr"));
        textparser_close(h);
    }

    // Test 2: Enclosed in cfoutput with runtime def -> SharpExpr parsed
    {
        textparser_t h = nullptr;
        const char *code = "<cfoutput><cfloop>#x#</cfloop></cfoutput>";
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &h), 0);
        ASSERT_EQ(textparser_parse(h, runtime_def), 0);
        TextParser tp_wrap(code, runtime_def);
        EXPECT_TRUE(has_token_type(tp_wrap, "SharpExpr"));
        textparser_close(h);
    }

    textparser_free_language_definition(runtime_def);
}
TEST(parse_CFML, parser_error_leading_sharp_quote) {
    // 1. Basic malformed leading #" inside cfoutput
    {
        const char *code = "<cfoutput>#\"x #a#\"</cfoutput>";
        textparser_t handle = nullptr;
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
        textparser_validation *val = textparser_validate_cfml(handle);
        ASSERT_NE(val, nullptr);
        bool found_error = false;
        for (int i = 0; i < val->len; i++) {
            if (val->items[i]->type == TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR) {
                found_error = true;
                break;
            }
        }
        EXPECT_TRUE(found_error);
        textparser_validation_clear(val);
        textparser_close(handle);
    }

    // 2. Multiple sharp quotes inside cfoutput
    {
        const char *code = "<cfoutput>text #\"a# more #\"b#</cfoutput>";
        textparser_t handle = nullptr;
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
        textparser_validation *val = textparser_validate_cfml(handle);
        ASSERT_NE(val, nullptr);
        bool found_error = false;
        for (int i = 0; i < val->len; i++) {
            if (val->items[i]->type == TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR) {
                found_error = true;
                break;
            }
        }
        EXPECT_TRUE(found_error);
        textparser_validation_clear(val);
        textparser_close(handle);
    }

    // 3. Valid sharp expression inside cfoutput should pass without validation errors
    {
        const char *code = "<cfoutput>#a#</cfoutput>";
        textparser_t handle = nullptr;
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
        textparser_validation *val = textparser_validate_cfml(handle);
        bool found_error = false;
        if (val != nullptr) {
            for (int i = 0; i < val->len; i++) {
                if (val->items[i]->type == TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR) {
                    found_error = true;
                    break;
                }
            }
            textparser_validation_clear(val);
        }
        EXPECT_FALSE(found_error);
        textparser_close(handle);
    }

    // 4. Valid sharp expression containing string quotes should pass
    {
        const char *code = "<cfoutput># UCase(\"hello\") #</cfoutput>";
        textparser_t handle = nullptr;
        ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
        EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
        textparser_validation *val = textparser_validate_cfml(handle);
        bool found_error = false;
        if (val != nullptr) {
            for (int i = 0; i < val->len; i++) {
                if (val->items[i]->type == TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR) {
                    found_error = true;
                    break;
                }
            }
            textparser_validation_clear(val);
        }
        EXPECT_FALSE(found_error);
        textparser_close(handle);
    }
}

TEST(parse_CFML, script_component_basic) {
    const char *code = "component {\n    function sayHello() {\n        return \"hello\";\n    }\n}";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    textparser_set_filename(handle, "MyComponent.cfc");

    EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);

    const char *token_str = textparser_get_token_type_str(&cfml_definition, first);
    EXPECT_STREQ(token_str, "ScriptExpression");
    textparser_close(handle);
}

TEST(parse_CFML, script_component_leading_comments_and_imports) {
    const char *code = "// Header comment\n/* Block comment */\nimport com.example.Service;\ncomponent extends=\"Base\" {\n}";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    // Case-insensitivity test (.CFC)
    textparser_set_filename(handle, "Service.CFC");

    EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);

    const char *token_str = textparser_get_token_type_str(&cfml_definition, first);
    EXPECT_STREQ(token_str, "ScriptExpression");
    textparser_close(handle);
}

TEST(parse_CFML, script_component_cfml_comment_quirk) {
    const char *code = "<!--- CFML comment --->\ncomponent {\n}";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    textparser_set_filename(handle, "QuirkComponent.cfc");

    EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
    textparser_token_item *first = textparser_get_first_token(handle);
    ASSERT_NE(first, nullptr);

    // Initial <!--- fails script detection; first token is standard Comment tag token
    const char *token_str = textparser_get_token_type_str(&cfml_definition, first);
    EXPECT_STREQ(token_str, "Comment");
    textparser_close(handle);
}

TEST(parse_CFML, cfm_file_unaffected) {
    const char *code = "component { }";
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(code, strlen(code), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    textparser_set_filename(handle, "index.cfm");

    EXPECT_EQ(textparser_parse(handle, &cfml_definition), 0);
    textparser_token_item *first = textparser_get_first_token(handle);
    // On .cfm file, component { } without <cfscript> is not matched as ScriptExpression
    if (first != nullptr) {
        const char *token_str = textparser_get_token_type_str(&cfml_definition, first);
        EXPECT_STRNE(token_str, "ScriptExpression");
    }
    textparser_close(handle);
}




#include "tokenparser.hpp"
#include <textparser.h>

#include <gtest/gtest.h>

#include <cfml_definition.json.h>
#include <cfml.h>


TEST(parse_CFML, crash_cfset) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<!--- <!--- --->)", &cfml_definition);
    textparser_suppress_errors() = false;
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
    if (item.type && target_type == item.type) {
        found = true;
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens_for_type(item[i], target_type, found);
    }
}

static bool has_token_type(const TextParser &tokens, const std::string &target_type) {
    bool found = false;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens_for_type(tokens[i], target_type, found);
    }
    return found;
}

// Dump helper for visual debugging and validation
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

static std::string dump_all_tokens(const TextParser &tokens) {
    std::string result;
    for (size_t i = 0; i < tokens.count; ++i) {
        result += dump_tokens(tokens[i]);
    }
    return result;
}

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
        printf("DEBUG nested comments tree:\n%s", dump_all_tokens(tokens).c_str());
    }
}

TEST(parse_CFML, comments_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<!--- unclosed comment)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG unclosed HTML comment tree:\n%s", dump_all_tokens(tokens).c_str());
}

TEST(parse_CFML, comments_script) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>
        /* block comment */
        // line comment
        x = 1;
    </cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG script comments tree:\n%s", dump_all_tokens(tokens).c_str());
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
    
    printf("DEBUG double escaped string tree:\n%s", dump_all_tokens(tokens).c_str());
    EXPECT_TRUE(has_token_type(tokens, "DoubleChar")); // "" should be recognized
}

TEST(parse_CFML, string_single_escaped) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = 'hello ''world''!' />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG single escaped string tree:\n%s", dump_all_tokens(tokens).c_str());
    EXPECT_TRUE(has_token_type(tokens, "SingleChar")); // '' should be recognized
}

TEST(parse_CFML, string_mixed_quotes) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cfset x = "hello 'world'!" />)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cfset x = 'hello "world"!' />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG mixed quotes 1 tree:\n%s", dump_all_tokens(tokens1).c_str());
    printf("DEBUG mixed quotes 2 tree:\n%s", dump_all_tokens(tokens2).c_str());
    
    EXPECT_TRUE(has_token_type(tokens1, "DoubleString"));
    EXPECT_TRUE(has_token_type(tokens2, "SingleString"));
}

TEST(parse_CFML, string_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "unclosed string />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG unclosed string tree:\n%s", dump_all_tokens(tokens).c_str());
}

// -------------------------------------------------------------
// 3. SHARP EXPRESSION CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, sharp_in_double_string) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "hello #name#" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG sharp in double string tree:\n%s", dump_all_tokens(tokens).c_str());
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, sharp_in_single_string) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = 'hello #name#' />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG sharp in single string tree:\n%s", dump_all_tokens(tokens).c_str());
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, sharp_escaped) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "hello ##name##" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG escaped sharp tree:\n%s", dump_all_tokens(tokens).c_str());
    EXPECT_TRUE(has_token_type(tokens, "SharpChar")); // Should be recognized as ##
}

TEST(parse_CFML, sharp_outside_string) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = #myVar# />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG sharp outside string tree:\n%s", dump_all_tokens(tokens).c_str());
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, sharp_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "hello #unclosed" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG unclosed sharp tree:\n%s", dump_all_tokens(tokens).c_str());
}

TEST(parse_CFML, sharp_nested_expression) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = "#func(arg1, "#nested#")#" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG nested sharp tree:\n%s", dump_all_tokens(tokens).c_str());
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
    
    printf("DEBUG basic cfscript tree:\n%s", dump_all_tokens(tokens).c_str());
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
    
    printf("DEBUG complex cfscript tree:\n%s", dump_all_tokens(tokens).c_str());
}

TEST(parse_CFML, cfscript_operators) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfscript>
        x += 1;
        y -= 1;
        z = a && b;
    </cfscript>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG cfscript operators tree:\n%s", dump_all_tokens(tokens).c_str());
}

// -------------------------------------------------------------
// 5. CFML TAG CORNER CASES
// -------------------------------------------------------------

TEST(parse_CFML, tag_nested_cfoutput) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfoutput>outer <cfoutput>inner #var#</cfoutput> end</cfoutput>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG nested cfoutput tree:\n%s", dump_all_tokens(tokens).c_str());
}

TEST(parse_CFML, tag_nested_cfloop) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfloop>outer <cfloop>inner</cfloop> end</cfloop>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG nested cfloop tree:\n%s", dump_all_tokens(tokens).c_str());
}

TEST(parse_CFML, tag_nested_cfquery) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfquery name="q">SELECT * FROM t WHERE id = #id#</cfquery>)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG nested cfquery tree:\n%s", dump_all_tokens(tokens).c_str());
    EXPECT_TRUE(has_token_type(tokens, "QueryStartTag"));
    EXPECT_TRUE(has_token_type(tokens, "QueryEndTag"));
    EXPECT_TRUE(has_token_type(tokens, "SharpExpression"));
}

TEST(parse_CFML, tag_custom_tags) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cf_mycustomtag attr="val">body</cf_mycustomtag>)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cf_mycustomtag attr="val" />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG custom tag tree:\n%s", dump_all_tokens(tokens1).c_str());
    printf("DEBUG self-closing custom tag tree:\n%s", dump_all_tokens(tokens2).c_str());
    
    EXPECT_TRUE(has_token_type(tokens1, "StartTag"));
    EXPECT_TRUE(has_token_type(tokens1, "EndTag"));
    EXPECT_TRUE(has_token_type(tokens2, "StartTag"));
}

TEST(parse_CFML, tag_unclosed) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cf_mycustomtag attr="val">)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG unclosed tag tree:\n%s", dump_all_tokens(tokens).c_str());
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
    
    printf("DEBUG word operator 'is not' tree:\n%s", dump_all_tokens(tokens1).c_str());
    printf("DEBUG word operator 'contains' tree:\n%s", dump_all_tokens(tokens2).c_str());
    printf("DEBUG word operator 'greater than or equal to' tree:\n%s", dump_all_tokens(tokens3).c_str());
}

TEST(parse_CFML, expr_arithmetic_precedence) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = a + b * c - d / e />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG arithmetic precedence tree:\n%s", dump_all_tokens(tokens).c_str());
}

TEST(parse_CFML, expr_function_calls) {
    textparser_suppress_errors() = true;
    auto tokens = TextParser(R"(<cfset x = func1(func2(a, b), c) />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG function calls tree:\n%s", dump_all_tokens(tokens).c_str());
}

TEST(parse_CFML, expr_array_struct_access) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cfset x = array[index] />)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cfset x = struct.member />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG array access tree:\n%s", dump_all_tokens(tokens1).c_str());
    printf("DEBUG struct member access tree:\n%s", dump_all_tokens(tokens2).c_str());
}

TEST(parse_CFML, expr_numbers) {
    textparser_suppress_errors() = true;
    auto tokens1 = TextParser(R"(<cfset x = +1.23 />)", &cfml_definition);
    auto tokens2 = TextParser(R"(<cfset x = -.45 />)", &cfml_definition);
    auto tokens3 = TextParser(R"(<cfset x = 123. />)", &cfml_definition);
    textparser_suppress_errors() = false;
    
    printf("DEBUG numbers +1.23 tree:\n%s", dump_all_tokens(tokens1).c_str());
    printf("DEBUG numbers -.45 tree:\n%s", dump_all_tokens(tokens2).c_str());
    printf("DEBUG numbers 123. tree:\n%s", dump_all_tokens(tokens3).c_str());
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



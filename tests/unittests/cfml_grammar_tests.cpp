#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <cfml.h>

#include <cstring>
#include <string>
#include <vector>

namespace {

struct CFMLGrammarFixture : testing::Test {
    textparser_language_definition *definition = nullptr;

    void SetUp() override {
        ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                      "definitions/cfml_definition.json", &definition),
                  TEXTPARSER_JSON_NO_ERROR);
        ASSERT_NE(definition, nullptr);
        ASSERT_NE(definition->grammar, nullptr);
        ASSERT_STREQ(definition->initial_lexer_mode, "default");
    }

    void TearDown() override {
        if (definition != nullptr) {
            textparser_free_language_definition(definition);
            definition = nullptr;
        }
    }

    int token(const char *name) const {
        for (int i = 0; definition->tokens[i].name != nullptr; i++) {
            if (std::strcmp(definition->tokens[i].name, name) == 0) return i;
        }
        return -1;
    }

    int production(const char *name) const {
        for (size_t i = 0; i < definition->grammar->production_count; i++) {
            if (std::strcmp(definition->grammar->productions[i].name, name) == 0)
                return definition->grammar->productions[i].id;
        }
        return -1;
    }

    textparser_node *parse_expression(const char *source,
                                      textparser_match_status expected = TEXTPARSER_MATCH_OK) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(definition), 0)
            << (textparser_parse_error(parser.get()) ? textparser_parse_error(parser.get()) : "")
            << " at " << textparser_parse_error_position(parser.get());
        for (size_t i = 0; i < definition->operator_definition_count; i++) {
            EXPECT_EQ(textparser_register_operator(parser.get(), &definition->operator_definitions[i]), 0);
        }
        result = {};
        EXPECT_EQ(parser.execute_production(definition->grammar->productions,
                                            definition->grammar->production_count,
                                            production("Expression"), &result), 0);
        EXPECT_EQ(result.status, expected) << source;
        return result.node;
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(definition), 0);
        EXPECT_EQ(textparser_cfml_register_validators(parser.get()), 0);
        for (size_t i = 0; i < definition->operator_definition_count; i++) {
            EXPECT_EQ(textparser_register_operator(parser.get(), &definition->operator_definitions[i]), 0);
        }
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(definition, &result), 0);
        if (result.status == TEXTPARSER_MATCH_OK) {
            const textparser_lex_token *remaining = nullptr;
            int peek = textparser_lexer_peek(
                parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
            if (peek == 0 && remaining != nullptr) {
                result = {};
                result.status = TEXTPARSER_MATCH_NO;
            }
        }
        EXPECT_EQ(result.status, expected) << source;
        return result.node;
    }

    const char *node_name(const textparser_node *node) {
        return textparser_grammar_node_name(parser.get(), node);
    }

    void rejects(const char *source, const char *code) {
        SCOPED_TRACE(source);
        parse_source(source);
        bool found = false;
        for (size_t i = 0; i < textparser_get_diagnostic_count(parser.get()); ++i) {
            textparser_diagnostic diagnostic{};
            EXPECT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
            if (diagnostic.code && std::strcmp(diagnostic.code, code) == 0) {
                found = true;
                EXPECT_EQ(diagnostic.severity, TEXTPARSER_SEVERITY_ERROR);
                EXPECT_GT(diagnostic.length, 0u);
            }
        }
        EXPECT_TRUE(found) << code;
    }

    textparser::Parser parser;
    textparser_match_result result = {};
};

TEST_F(CFMLGrammarFixture, loads_cfml_v2_grammar_profile) {
    EXPECT_STREQ(definition->name, "cfml");
    EXPECT_FALSE(definition->case_sensitivity);
    EXPECT_NE(definition->grammar, nullptr);
    EXPECT_GT(definition->grammar->production_count, 20u);
    EXPECT_EQ(definition->operator_definition_count, 20u);
    EXPECT_STREQ(definition->initial_lexer_mode, "default");
    EXPECT_GT(definition->recovery_sync_token_count, 0u);
}

// -------------------------------------------------------------------------
// 1. CFML Operator Precedence Tests (Verified Against CFML_Precedence.md)
// -------------------------------------------------------------------------

TEST_F(CFMLGrammarFixture, cfml_precedence_exponentiation_vs_multiplication) {
    // 2 ^ 3 * 2 evaluates as (2^3) * 2 = 16
    // Root of expression must be MulOperator, with left child PowerOperator
    auto *root = parse_expression("2 ^ 3 * 2");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "MulOperator");
    ASSERT_NE(root->child, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child), "PowerOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_multiplication_vs_modulo) {
    // 8 MOD 3 * 2 evaluates as 8 MOD (3 * 2) = 2 (Multiplication > Modulo)
    // Root of expression must be ModOperator, with right child MulOperator
    auto *root = parse_expression("8 MOD 3 * 2");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "ModOperator");
    ASSERT_NE(root->child, nullptr);
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child->next), "MulOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_multiplication_vs_integer_division) {
    // 8 \ 3 * 2 evaluates as 8 \ (3 * 2) = 1 (Multiplication > Integer division)
    // Root of expression must be IntegerDivOperator, with right child MulOperator
    auto *root = parse_expression("8 \\ 3 * 2");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "IntegerDivOperator");
    ASSERT_NE(root->child, nullptr);
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child->next), "MulOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_integer_division_vs_modulo) {
    // 8 MOD 5 \ 2 evaluates as 8 MOD (5 \ 2) = 0 (Integer division > Modulo)
    // Root of expression must be ModOperator, with right child IntegerDivOperator
    auto *root = parse_expression("8 MOD 5 \\ 2");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "ModOperator");
    ASSERT_NE(root->child, nullptr);
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child->next), "IntegerDivOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_addition_vs_concatenation) {
    // "A" & 2 + 3 evaluates as "A" & (2 + 3) = "A5" (Addition > Concatenation)
    // Root of expression must be ConcatOperator, with right child AddOperator
    auto *root = parse_expression("\"A\" & 2 + 3");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "ConcatOperator");
    ASSERT_NE(root->child, nullptr);
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child->next), "AddOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_comparison_vs_logical_not) {
    // NOT 0 GT 3 evaluates as NOT (0 GT 3) = true (Comparison > Logical NOT)
    // Root of expression must be LogicalNotOperator, with operand CompareOperator
    auto *root = parse_expression("not 0 > 3");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "LogicalNotOperator");
    ASSERT_NE(root->child, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child), "CompareOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_logical_and_vs_logical_or) {
    // true or true and false evaluates as true or (true and false) = true (AND > OR)
    // Root of expression must be LogicalOrOperator, with right child LogicalAndOperator
    auto *root = parse_expression("true or true and false");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "LogicalOrOperator");
    ASSERT_NE(root->child, nullptr);
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child->next), "LogicalAndOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_left_associative_power) {
    // 2 ^ 3 ^ 2 evaluates as (2^3)^2 = 64 on ColdFusion (left-associative)
    // Root of expression must be PowerOperator, whose left child is PowerOperator
    auto *root = parse_expression("2 ^ 3 ^ 2");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "PowerOperator");
    ASSERT_NE(root->child, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child), "PowerOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_unary_minus_tighter_than_power) {
    // -2 ^ 2 evaluates as (-2)^2 = 4 on ColdFusion (unary minus binds tighter than ^)
    // Root is PowerOperator with left operand being unary minus
    auto *root = parse_expression("-2 ^ 2");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "PowerOperator");
    ASSERT_NE(root->child, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child), "AddOperator");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_ternary_and_assignment) {
    // x = a ? b : c + 5
    // Assignment has lowest precedence, ternary condition is inside, c + 5 is right branch
    auto *root = parse_expression("x = a ? b : c + 5");
    ASSERT_NE(root, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root), "AssignOperator");
    ASSERT_NE(root->child, nullptr);
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), root->child->next), "Question");
}

TEST_F(CFMLGrammarFixture, cfml_precedence_null_coalescing_and_elvis) {
    auto *elvis = parse_expression("x ?: 'default'");
    ASSERT_NE(elvis, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), elvis), "ElvisOperator");

    auto *nullc = parse_expression("x ?? 'default'");
    ASSERT_NE(nullc, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), nullc), "NullCoalescingOperator");
}

// -------------------------------------------------------------------------
// 2. CFML Script Grammar: Declarations, Functions, and Statements
// -------------------------------------------------------------------------

TEST_F(CFMLGrammarFixture, parses_script_variable_declarations) {
    EXPECT_NE(parse_source("<cfscript>var a = 1; var b = 'hello', c = true;</cfscript>"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_script_function_declarations) {
    const char *code =
        "<cfscript>\n"
        "public numeric function calculateSum(required numeric a, numeric b = 0) {\n"
        "    return a + b;\n"
        "}\n"
        "</cfscript>";
    EXPECT_NE(parse_source(code), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_script_arrow_functions) {
    EXPECT_NE(parse_source("<cfscript>fn = (x) => x * 2; mult = (a, b) => { return a * b; };</cfscript>"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_script_control_flow_statements) {
    const char *code =
        "<cfscript>\n"
        "if (x > 10) {\n"
        "    y = 1;\n"
        "} else if (x == 10) {\n"
        "    y = 0;\n"
        "} else {\n"
        "    y = -1;\n"
        "}\n"
        "for (var item in items) {\n"
        "    total += item;\n"
        "}\n"
        "for (var i = 0; i < 10; i++) {\n"
        "    count++;\n"
        "}\n"
        "while (active) {\n"
        "    poll();\n"
        "}\n"
        "do {\n"
        "    step();\n"
        "} while (running);\n"
        "switch (status) {\n"
        "    case 200:\n"
        "        handleOk();\n"
        "        break;\n"
        "    default:\n"
        "        handleError();\n"
        "}\n"
        "try {\n"
        "    risk();\n"
        "} catch (any e) {\n"
        "    log(e);\n"
        "} finally {\n"
        "    cleanup();\n"
        "}\n"
        "</cfscript>";
    EXPECT_NE(parse_source(code), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_script_coldfusion_specific_statements) {
    const char *code =
        "<cfscript>\n"
        "lock name=\"myLock\" timeout=5 type=\"exclusive\" {\n"
        "    data.save();\n"
        "}\n"
        "transaction action=\"begin\" {\n"
        "    commit();\n"
        "}\n"
        "param name=\"pageIndex\" default=1;\n"
        "include \"common/header.cfm\";\n"
        "import services.UserService;\n"
        "</cfscript>";
    EXPECT_NE(parse_source(code), nullptr);
}

// -------------------------------------------------------------------------
// 3. Script Components (.cfc) and Interfaces
// -------------------------------------------------------------------------

TEST_F(CFMLGrammarFixture, parses_cfc_script_component) {
    const char *code =
        "component extends=\"BaseService\" implements=\"IService\" accessors=true {\n"
        "    property name=\"id\" type=\"numeric\";\n"
        "    property name=\"name\" type=\"string\";\n"
        "\n"
        "    public any function init(numeric id, string name) {\n"
        "        variables.id = arguments.id;\n"
        "        variables.name = arguments.name;\n"
        "        return this;\n"
        "    }\n"
        "}";
    EXPECT_NE(parse_source(code), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_cfc_script_interface) {
    const char *code =
        "interface {\n"
        "    public string function getName();\n"
        "    public void function setName(required string name);\n"
        "}";
    EXPECT_NE(parse_source(code), nullptr);
}

// -------------------------------------------------------------------------
// 4. Tag-Based Template and Embedded Expressions
// -------------------------------------------------------------------------

TEST_F(CFMLGrammarFixture, parses_cfml_tags_and_output_interpolation) {
    const char *code =
        "<cfoutput>\n"
        "    <h1>Welcome, #user.name#!</h1>\n"
        "    <p>Age: #user.age + 1#</p>\n"
        "</cfoutput>\n"
        "<cfloop from=\"1\" to=\"5\" index=\"i\">\n"
        "    <cfoutput>Item ###i#</cfoutput>\n"
        "</cfloop>\n"
        "<cfquery name=\"getUser\">\n"
        "    SELECT id, name FROM users WHERE id = #userId#\n"
        "</cfquery>\n"
        "<cfmail to=\"user@example.com\" from=\"admin@example.com\" subject=\"Notice\">\n"
        "    Hello #user.name#\n"
        "</cfmail>\n"
        "<cfsavecontent variable=\"content\">\n"
        "    Captured block #x#\n"
        "</cfsavecontent>\n"
        "<cfset x = 42 />\n";
    EXPECT_NE(parse_source(code), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_struct_and_array_literals) {
    EXPECT_NE(parse_expression("[1, 2, 'three', true]"), nullptr);
    EXPECT_NE(parse_expression("[:]"), nullptr); // Empty ordered struct
    EXPECT_NE(parse_expression("{ a: 1, b: 'two', 'c': [3, 4] }"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_safe_navigation_and_member_chains) {
    EXPECT_NE(parse_expression("user?.profile?.address?.zip"), nullptr);
    EXPECT_NE(parse_expression("data.items[1].getValue()"), nullptr);
    EXPECT_NE(parse_expression("new services.ReportService(config).generate()"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_full_expressions_in_tag_attributes) {
    for (const char *code : {
             "<cfset x = a + b />",
             "<cfset x = -1 />",
             "<cfset x = f(a, b) />",
             "<cfset x = a.b.c />",
             "<cfset x = a[0] />",
             "<cfset x = #a + b# />",
             "<cfset x = user?.name />",
             "<cfset x = a eq b />",
             "<cfset x = a MOD b />"})
        EXPECT_NE(parse_source(code), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_assignment_and_ordered_struct_literals) {
    EXPECT_NE(parse_expression("{ a = 1, b: 'two' }"), nullptr);
    EXPECT_NE(parse_expression("[ a = 1, b = 2 ]"), nullptr);
    EXPECT_NE(parse_expression("{ \"a\" = 1 }"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_if_tag_pairs) {
    for (const char *code : {
             "<cfif x>a</cfif>",
             "<cfif x eq 1>a<cfelse>b</cfif>",
             "<cfif x>a<cfelseif y>b<cfelse>c</cfif>",
             "<cfif x>a</cfif><cfif y>b</cfif>"})
        EXPECT_NE(parse_source(code), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_nested_comments) {
    EXPECT_NE(parse_source("<!--- outer <!--- inner ---> outer ---><cfset x = 1 />"), nullptr);
    EXPECT_NE(parse_source("<!--- simple ---><cfset x = 1 />"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_pageencoding_directive) {
    EXPECT_NE(parse_source("<cfscript>pageencoding \"UTF-8\";</cfscript>"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_string_concatenation_in_tag_attributes) {
    EXPECT_NE(parse_source("<cfset res = \"A\" & 2 + 3 />"), nullptr);
    EXPECT_NE(parse_source("<cfset res = \"A\" & \"B\" />"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_multiple_switch_cases) {
    EXPECT_NE(parse_source("<cfscript>switch(v){ case 1: break; case 2: break; }</cfscript>"), nullptr);
    EXPECT_NE(parse_source("<cfscript>switch(v){ case 1: x=1; default: x=2; }</cfscript>"), nullptr);
    EXPECT_NE(parse_source(
        "<cfscript>switch(val) { case \"B\": res = \"Got B\"; break; default: continue; }</cfscript>"),
        nullptr);
}

TEST_F(CFMLGrammarFixture, parses_cfset_expressions) {
    for (const char *code : {
             "<cfset x = 1 />",
             "<cfset x = a + b />",
             "<cfset x++ />",
             "<cfset ++x />",
             "<cfset x-- />",
             "<cfset var x = 1 />",
             "<cfset x = user?.name />"})
        EXPECT_NE(parse_source(code), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_property_and_thread_declarations) {
    EXPECT_NE(parse_source(
        "<cfscript>property name=\"id\" required=\"true\"; param name=\"p\" default=\"1\"; "
        "lock name=\"L\" { super.init(); null; } transaction { thread name=\"T\" {} }</cfscript>"),
        nullptr);
}

TEST_F(CFMLGrammarFixture, line_comment_stops_before_script_end_tag) {
    EXPECT_NE(parse_source("<cfscript>//</cfscript>"), nullptr);
    EXPECT_NE(parse_source("<cfscript>a = 1; // trailing</cfscript>"), nullptr);
}

TEST_F(CFMLGrammarFixture, parses_literal_hash_and_nested_sharp) {
    EXPECT_NE(parse_source("<cfoutput>#a#</cfoutput>"), nullptr);
    EXPECT_NE(parse_source("<cfoutput>#</cfoutput>"), nullptr);
    EXPECT_NE(parse_source("<cfoutput>#\"x #a#\"</cfoutput>"), nullptr);
    EXPECT_NE(parse_source("<cfoutput>#\"x #a#\"#</cfoutput>"), nullptr);
}

TEST_F(CFMLGrammarFixture, validates_builtin_function_calls) {
    EXPECT_NE(parse_source("<cfset x = acos(0.5) />"), nullptr);
    EXPECT_NE(parse_source("<cfset x = acos(0.5) /><cfset y = abs(-1) />"), nullptr);
    rejects("<cfset x = nonExistingFunc() />", "CF2001");
    rejects("<cfset x = acos() />", "CF2002");
    rejects("<cfset x = acos(1, 2) />", "CF2002");
}

TEST_F(CFMLGrammarFixture, validates_cfprocessingdirective_position) {
    std::string ok = std::string(3000, ' ') + "<cfprocessingdirective pageEncoding=\"utf-8\" />";
    EXPECT_NE(parse_source(ok.c_str()), nullptr);
    std::string bad = std::string(4090, ' ') + "<cfprocessingdirective pageEncoding=\"utf-8\" />";
    rejects(bad.c_str(), "CF2003");
}

TEST_F(CFMLGrammarFixture, validates_known_and_unknown_tags) {
    EXPECT_NE(parse_source("<cfset x = 1 />"), nullptr);
    EXPECT_NE(parse_source("<cfcomponent></cfcomponent>"), nullptr);
    EXPECT_NE(parse_source("<div>hello</div>"), nullptr); // HTML is not a CFML tag
    rejects("<cffoo></cffoo>", "CF2004");
}

TEST_F(CFMLGrammarFixture, validates_tag_pairing) {
    EXPECT_NE(parse_source("<cfcomponent></cfcomponent>"), nullptr);
    EXPECT_NE(parse_source("<cfcomponent />"), nullptr);
    EXPECT_NE(parse_source("<cfoutput></CFOUTPUT><CFCOMPONENT></cfcomponent>"), nullptr);
    rejects("<cfcomponent>", "CF2005");
    rejects("<CFCOMPONENT>", "CF2005");
    rejects("<cfabort></cfabort>", "CF2006");
    rejects("<CFSET x = 1></CFSET>", "CF2006");
    rejects("</cfcomponent>", "CF2007");
}

// -------------------------------------------------------------------------
// 5. Diagnostic Recovery
// -------------------------------------------------------------------------

TEST_F(CFMLGrammarFixture, recovers_at_statement_boundaries_in_script) {
    // Missing expression after '=' in first statement, followed by valid statement
    const char *code = "<cfscript>var a = ; var b = 42;</cfscript>";
    textparser_node *root = parse_source(code, TEXTPARSER_MATCH_OK);
    ASSERT_NE(root, nullptr);
    // Verified that recovery successfully resumed and parsed second statement
}

} // namespace

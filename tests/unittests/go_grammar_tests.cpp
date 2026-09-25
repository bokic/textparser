#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <go_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct GoGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/go_definition.json", &json_definition),
                      TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json_definition, nullptr);
            ASSERT_NE(json_definition->grammar, nullptr);
        }
    }

    void TearDown() override {
        if (json_definition != nullptr) {
            textparser_free_language_definition(json_definition);
            json_definition = nullptr;
        }
    }

    const textparser_language_definition *get_definition() const {
        return GetParam() ? json_definition : &go_definition;
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(get_definition()), 0);
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(get_definition(), &result), 0);
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

    const textparser_node *find(const textparser_node *node, const char *kind) {
        if (!node) return nullptr;
        const char *name = textparser_grammar_node_name(parser.get(), node);
        if (name && std::strcmp(name, kind) == 0) return node;
        for (auto *child = node->child; child; child = child->next) {
            if (auto *found = find(child, kind)) return found;
        }
        return nullptr;
    }

    textparser::Parser parser;
    textparser_match_result result = {};
};

INSTANTIATE_TEST_SUITE_P(DefinitionSources, GoGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(GoGrammarFixture, parses_package_and_imports) {
    const char *source =
        "package main\n"
        "import \"fmt\"\n"
        "import (\n"
        "    \"os\"\n"
        "    myio \"io\"\n"
        "    . \"strings\"\n"
        "    _ \"image/png\"\n"
        ")\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PackageClause"), nullptr);
    EXPECT_NE(find(node, "ImportDeclaration"), nullptr);
    EXPECT_NE(find(node, "ImportSpec"), nullptr);
}

TEST_P(GoGrammarFixture, parses_constants_and_variables) {
    const char *source =
        "package config\n"
        "const Pi = 3.14159\n"
        "const (\n"
        "    A = 1\n"
        "    B\n"
        "    C float64 = 2.0\n"
        ")\n"
        "var x int\n"
        "var y, z = 1, \"two\"\n"
        "var (\n"
        "    a int = 10\n"
        "    b string\n"
        ")\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ConstDeclaration"), nullptr);
    EXPECT_NE(find(node, "VarDeclaration"), nullptr);
    EXPECT_NE(find(node, "ConstSpec"), nullptr);
    EXPECT_NE(find(node, "VarSpec"), nullptr);
}

TEST_P(GoGrammarFixture, parses_types_structs_and_interfaces) {
    const char *source =
        "package model\n"
        "type MyInt int\n"
        "type Alias = string\n"
        "type Stringer interface {\n"
        "    String() string\n"
        "    ~int | ~string\n"
        "}\n"
        "type Point struct {\n"
        "    X, Y int `json:\"coords\"`\n"
        "    tag string\n"
        "    *pkg.Base\n"
        "}\n"
        "type Vector[T any] []T\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeDeclaration"), nullptr);
    EXPECT_NE(find(node, "StructType"), nullptr);
    EXPECT_NE(find(node, "InterfaceType"), nullptr);
    EXPECT_NE(find(node, "TypeParameters"), nullptr);
}

TEST_P(GoGrammarFixture, parses_functions_methods_and_generics) {
    const char *source =
        "package service\n"
        "func Add(a, b int) int {\n"
        "    return a + b\n"
        "}\n"
        "func (p *Point) Move(dx, dy int) {\n"
        "    p.X += dx\n"
        "    p.Y += dy\n"
        "}\n"
        "func Map[T, U any](s []T, f func(T) U) []U {\n"
        "    res := make([]U, len(s))\n"
        "    for i, v := range s {\n"
        "        res[i] = f(v)\n"
        "    }\n"
        "    return res\n"
        "}\n"
        "func sum(nums ...int) (total int, count int) {\n"
        "    for _, n := range nums {\n"
        "        total += n\n"
        "        count++\n"
        "    }\n"
        "    return total, count\n"
        "}\n"
        "func asmStub()\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "MethodDeclaration"), nullptr);
    EXPECT_NE(find(node, "Receiver"), nullptr);
    EXPECT_NE(find(node, "Signature"), nullptr);
}

TEST_P(GoGrammarFixture, parses_control_flow_statements) {
    const char *source =
        "package main\n"
        "func logic(val int, ch chan int) {\n"
        "    if x := val; x > 0 {\n"
        "        val = x * 2\n"
        "    } else if x < 0 {\n"
        "        val = -x\n"
        "    } else {\n"
        "        val = 0\n"
        "    }\n"
        "    for i := 0; i < 10; i++ {\n"
        "        if i == 5 {\n"
        "            continue\n"
        "        }\n"
        "    }\n"
        "    for val > 0 {\n"
        "        val--\n"
        "    }\n"
        "    switch val {\n"
        "    case 1, 2:\n"
        "        val = 10\n"
        "    default:\n"
        "        val = 20\n"
        "    }\n"
        "    select {\n"
        "    case ch <- val:\n"
        "        val = 0\n"
        "    case msg := <-ch:\n"
        "        val = msg\n"
        "    default:\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
    EXPECT_NE(find(node, "SwitchStatement"), nullptr);
    EXPECT_NE(find(node, "SelectStatement"), nullptr);
}

TEST_P(GoGrammarFixture, parses_type_switch_and_type_assertions) {
    const char *source =
        "package main\n"
        "func testType(x any) string {\n"
        "    switch v := x.(type) {\n"
        "    case int:\n"
        "        return \"int\"\n"
        "    case string:\n"
        "        return v\n"
        "    default:\n"
        "        s := x.(string)\n"
        "        return s\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SwitchStatement"), nullptr);
    EXPECT_NE(find(node, "TypeAssertionSuffix"), nullptr);
}

TEST_P(GoGrammarFixture, parses_goroutines_defers_and_closures) {
    const char *source =
        "package main\n"
        "func asyncOps() {\n"
        "    go func(msg string) {\n"
        "        println(msg)\n"
        "    }(\"done\")\n"
        "    defer cleanup()\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "GoStatement"), nullptr);
    EXPECT_NE(find(node, "DeferStatement"), nullptr);
    EXPECT_NE(find(node, "FunctionLiteral"), nullptr);
}

TEST_P(GoGrammarFixture, parses_composite_literals_slices_and_operators) {
    const char *source =
        "package main\n"
        "func literals() {\n"
        "    p := Point{X: 1, Y: 2}\n"
        "    m := map[string]int{\"a\": 1, \"b\": 2}\n"
        "    s := []int{1, 2, 3, 4, 5}\n"
        "    slice1 := s[1:3]\n"
        "    slice2 := s[:2]\n"
        "    slice3 := s[2:]\n"
        "    slice4 := s[1:2:3]\n"
        "    raw := `multiline\n"
        "string`\n"
        "    runeVal := 'z'\n"
        "    hexVal := 0xDEAD_BEEF\n"
        "    binVal := 0b1010_0101\n"
        "    octVal := 0o755\n"
        "    floatVal := 1.234e-5\n"
        "    calc := 1 + 2 * 3 - 4 / 2 &^ 7\n"
        "    _ = calc\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CompositeLiteral"), nullptr);
    EXPECT_NE(find(node, "BracketSuffix"), nullptr);
}

TEST_P(GoGrammarFixture, parses_pratt_operator_precedence_tree) {
    const char *source =
        "package main\n"
        "func calc() bool {\n"
        "    return a || b && c == d + e * f\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
}

TEST_P(GoGrammarFixture, recovers_from_missing_or_malformed_tokens) {
    const char *source =
        "package main\n"
        "func broken() {\n"
        "    x := 10 + \n"
        "    y := 20\n"
        "}\n";
    parser.reset();
    EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(parser.parse(get_definition()), 0);
    result = {};
    int exec_rc = parser.execute_language_grammar(get_definition(), &result);
    EXPECT_EQ(exec_rc, 0);
}

} // namespace

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <c_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct CGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/c_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &c_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, CGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(CGrammarFixture, parses_translation_unit_and_preprocessor) {
    const char *source =
        "#include <stdio.h>\n"
        "#include \"custom.h\"\n"
        "#define PI 3.14159\n"
        "#define MAX(a, b) ((a) > (b) ? (a) : (b))\n"
        ";\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_functions_and_declarations) {
    const char *source =
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "void noop(void) {}\n"
        "static inline const char *get_name(const void *ptr) {\n"
        "    return \"anonymous\";\n"
        "}\n"
        "int main(int argc, char **argv) {\n"
        "    int sum = add(1, 2);\n"
        "    return sum;\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_structs_unions_and_enums) {
    const char *source =
        "struct Point {\n"
        "    int x;\n"
        "    int y;\n"
        "};\n"
        "union Data {\n"
        "    int i;\n"
        "    float f;\n"
        "    char str[20];\n"
        "};\n"
        "enum Color {\n"
        "    RED = 0,\n"
        "    GREEN = 1,\n"
        "    BLUE = 2,\n"
        "};\n"
        "typedef struct Node {\n"
        "    int value;\n"
        "    struct Node *next;\n"
        "} Node_t;\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_bitfields_and_anonymous_structures) {
    const char *source =
        "struct BitFlags {\n"
        "    unsigned int is_valid : 1;\n"
        "    unsigned int count : 7;\n"
        "    unsigned int : 24;\n"
        "};\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_designated_initializers_and_compound_literals) {
    const char *source =
        "void test() {\n"
        "    struct Point pt = {.x = 10, .y = 20};\n"
        "    int arr[5] = {[0] = 1, [4] = 42};\n"
        "    struct Point *p = &(struct Point){.x = 1, .y = 2};\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_control_flow_statements) {
    const char *source =
        "void loop_test(int n) {\n"
        "    if (n > 0) {\n"
        "        n--;\n"
        "    } else if (n < 0) {\n"
        "        n++;\n"
        "    } else {\n"
        "        return;\n"
        "    }\n"
        "    while (n < 10) {\n"
        "        n++;\n"
        "    }\n"
        "    do {\n"
        "        n--;\n"
        "    } while (n > 0);\n"
        "    for (int i = 0; i < 10; i++) {\n"
        "        if (i == 5) continue;\n"
        "        if (i == 8) break;\n"
        "    }\n"
        "    for (;;) { break; }\n"
        "    switch (n) {\n"
        "        case 0:\n"
        "            break;\n"
        "        case 1:\n"
        "        case 2:\n"
        "            n = 42;\n"
        "            break;\n"
        "        default:\n"
        "            goto end;\n"
        "    }\n"
        "end:\n"
        "    return;\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_pratt_expression_precedence) {
    const char *source =
        "void expr_test() {\n"
        "    int a = 1 + 2 * 3;\n"
        "    int b = (1 + 2) * 3;\n"
        "    int c = a << 2 + 1;\n"
        "    int d = a == b && c != d || a < b;\n"
        "    int e = a ? b : c ? d : 0;\n"
        "    int f = a = b = 42;\n"
        "    int g = (a, b, c);\n"
        "    int h = (int)3.14 + (int)a;\n"
        "    int *p = &a;\n"
        "    int val = *p;\n"
        "    int inc = (*p)++;\n"
        "    size_t sz = sizeof(int) + sizeof a;\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_c23_attributes_and_static_assert) {
    const char *source =
        "[[nodiscard]] int calculate(int x);\n"
        "[[maybe_unused]] static int unused_var = 0;\n"
        "_Static_assert(sizeof(int) >= 2, \"int is too small\");\n"
        "static_assert(1);\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_c23_typeof_and_generic_selection) {
    const char *source =
        "void generic_test() {\n"
        "    int x = 42;\n"
        "    typeof(x) y = x;\n"
        "    typeof_unqual(const int) z = 10;\n"
        "    const char *s = _Generic(x, int: \"integer\", default: \"unknown\");\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, parses_complex_declarators_and_function_pointers) {
    const char *source =
        "int (*fn_ptr)(int, char*);\n"
        "int (*signal(int sig, void (*func)(int)))(int);\n"
        "char *(*(*arr[3])())[5];\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CGrammarFixture, recovers_from_statement_syntax_errors) {
    const char *source =
        "int main() {\n"
        "    int a = ;\n"
        "    int b = 42;\n"
        "    return b;\n"
        "}\n";
    auto *node = parse_source(source);
    EXPECT_NE(node, nullptr);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
}

} // namespace

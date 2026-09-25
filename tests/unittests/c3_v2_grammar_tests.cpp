#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <c3_definition.json.h>

#include <cstring>
#include <functional>
#include <string>

namespace {

struct C3GrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/c3_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &c3_definition;
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK,
                                  bool allow_diagnostics = false) {
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
        if (expected == TEXTPARSER_MATCH_NO &&
            (result.status == TEXTPARSER_MATCH_ERROR ||
             (result.status == TEXTPARSER_MATCH_OK && textparser_get_diagnostic_count(parser.get()) != 0))) {
            EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
            result.status = TEXTPARSER_MATCH_NO;
        }
        if (result.status != expected) {
            std::cout << "Parse failed for source:\n" << source << "\nStatus: " << result.status << std::endl;
            const textparser_lex_token *rem = nullptr;
            textparser_lexer_peek(parser.get(), 0, textparser_get_lexical_goal(parser.get()), &rem);
            if (rem) {
                std::string token_str = (rem->end <= std::strlen(source)) ? std::string(source + rem->start, rem->end - rem->start) : "";
                std::cout << "Remaining token: kind=" << rem->kind << ", text='" << token_str << "', span=[" << rem->start << ", " << rem->end << "]" << std::endl;
            }
            size_t diag_count = textparser_get_diagnostic_count(parser.get());
            std::cout << "Diagnostic count: " << diag_count << std::endl;
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] " << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
                }
            }
        }
        EXPECT_EQ(result.status, expected) << source;
        if (expected == TEXTPARSER_MATCH_OK && !allow_diagnostics) {
            EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u) << source;
        }
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, C3GrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Module and Import ---

TEST_P(C3GrammarFixture, parses_module_declaration) {
    const char *source =
        "module myapp;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ModuleDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_qualified_module_path) {
    const char *source =
        "module math::utils;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ModuleDeclaration"), nullptr);
    EXPECT_NE(find(node, "ModulePath"), nullptr);
}

TEST_P(C3GrammarFixture, parses_import_declarations) {
    const char *source =
        "module app;\n"
        "import std::io;\n"
        "import std::math, std::string;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ImportDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_source_without_module) {
    const char *source =
        "import std::io;\n"
        "fn int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

// --- Function Declarations ---

TEST_P(C3GrammarFixture, parses_simple_function) {
    const char *source =
        "fn void hello() {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_function_with_parameters) {
    const char *source =
        "fn int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "ParameterList"), nullptr);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
}

TEST_P(C3GrammarFixture, parses_function_with_default_param) {
    const char *source =
        "fn void greet(char* name = \"world\") {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_variadic_function) {
    const char *source =
        "fn void log(char* fmt, ...) {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_function_with_annotation) {
    const char *source =
        "@inline\n"
        "fn int square(int x) {\n"
        "    return x * x;\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, parses_extern_function_prototype) {
    const char *source =
        "extern fn int printf(char* fmt, ...);\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_method_function) {
    const char *source =
        "fn void Vec2.normalize(Vec2* self) {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "FunctionName"), nullptr);
}

// --- Struct Declarations ---

TEST_P(C3GrammarFixture, parses_simple_struct) {
    const char *source =
        "struct Point {\n"
        "    int x;\n"
        "    int y;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StructDeclaration"), nullptr);
    EXPECT_NE(find(node, "StructBody"), nullptr);
}

TEST_P(C3GrammarFixture, parses_struct_with_inline_union) {
    const char *source =
        "struct Variant {\n"
        "    int tag;\n"
        "    union {\n"
        "        int i;\n"
        "        double d;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StructDeclaration"), nullptr);
    EXPECT_NE(find(node, "StructInlineDecl"), nullptr);
}

// --- Union Declarations ---

TEST_P(C3GrammarFixture, parses_union_declaration) {
    const char *source =
        "union Value {\n"
        "    int i;\n"
        "    float f;\n"
        "    bool b;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "UnionDeclaration"), nullptr);
}

// --- Enum Declarations ---

TEST_P(C3GrammarFixture, parses_simple_enum) {
    const char *source =
        "enum Color {\n"
        "    RED,\n"
        "    GREEN,\n"
        "    BLUE\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "EnumDeclaration"), nullptr);
    EXPECT_NE(find(node, "EnumBody"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_enum_with_backing_type) {
    const char *source =
        "enum Status : int {\n"
        "    OK = 0,\n"
        "    ERR = 1\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

// --- Fault Declarations ---

TEST_P(C3GrammarFixture, rejects_fault_declaration) {
    const char *source =
        "fault IoError {\n"
        "    FILE_NOT_FOUND,\n"
        "    PERMISSION_DENIED\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

// --- Interface Declarations ---

TEST_P(C3GrammarFixture, parses_interface_declaration) {
    const char *source =
        "interface Printable {\n"
        "    fn void print(Printable* self);\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InterfaceDeclaration"), nullptr);
    EXPECT_NE(find(node, "InterfaceMethod"), nullptr);
}

// --- Typedef / ConstDef ---

TEST_P(C3GrammarFixture, parses_typedef_declaration) {
    const char *source =
        "typedef MyInt = int;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypedefDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_const_declaration) {
    const char *source =
        "const int MAX_SIZE = 1024;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ConstDeclaration"), nullptr);
}

// --- Global Variables ---

TEST_P(C3GrammarFixture, parses_global_variable) {
    const char *source =
        "int g_counter = 0;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "GlobalVarDeclaration"), nullptr);
}

// --- Statements ---

TEST_P(C3GrammarFixture, parses_if_else) {
    const char *source =
        "fn void check(int x) {\n"
        "    if (x > 0) {\n"
        "        return;\n"
        "    } else {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
}

TEST_P(C3GrammarFixture, parses_while_loop) {
    const char *source =
        "fn void loop() {\n"
        "    int i = 0;\n"
        "    while (i < 10) {\n"
        "        i = i + 1;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
}

TEST_P(C3GrammarFixture, parses_for_loop) {
    const char *source =
        "fn void count() {\n"
        "    for (int i = 0; i < 10; i = i + 1) {\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
}

TEST_P(C3GrammarFixture, parses_foreach_loop) {
    const char *source =
        "fn void iterate(int[] arr) {\n"
        "    foreach (int x : arr) {\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForeachStatement"), nullptr);
}

TEST_P(C3GrammarFixture, parses_switch_statement) {
    const char *source =
        "fn void classify(int x) {\n"
        "    switch (x) {\n"
        "        case 0:\n"
        "            break;\n"
        "        case 1:\n"
        "            break;\n"
        "        default:\n"
        "            break;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SwitchStatement"), nullptr);
    EXPECT_NE(find(node, "CaseClause"), nullptr);
    EXPECT_NE(find(node, "DefaultClause"), nullptr);
    EXPECT_NE(find(node, "BreakStatement"), nullptr);
}

TEST_P(C3GrammarFixture, parses_defer_statement) {
    const char *source =
        "fn void cleanup() {\n"
        "    defer {\n"
        "        int x = 0;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DeferStatement"), nullptr);
}

TEST_P(C3GrammarFixture, parses_do_while) {
    const char *source =
        "fn void dowork() {\n"
        "    do {\n"
        "        int x = 0;\n"
        "    } while (true);\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DoWhileStatement"), nullptr);
}

// --- Compile-time features ---

TEST_P(C3GrammarFixture, parses_compile_time_directive) {
    const char *source =
        "fn void run() {\n"
        "    $assert(true);\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CompileTimeStatement"), nullptr);
}

// --- Expressions ---

TEST_P(C3GrammarFixture, parses_arithmetic_expressions) {
    const char *source =
        "fn int math() {\n"
        "    int a = 1 + 2 * 3 - 4 / 2;\n"
        "    return a;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Plus"), nullptr);
    EXPECT_NE(find(node, "Star"), nullptr);
}

TEST_P(C3GrammarFixture, parses_bitwise_expressions) {
    const char *source =
        "fn int bits(int x) {\n"
        "    return x & 0xFF | 0x0F ^ 0xAA;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Ampersand"), nullptr);
}

TEST_P(C3GrammarFixture, parses_comparison_expressions) {
    const char *source =
        "fn bool compare(int a, int b) {\n"
        "    return a == b || a != b && a <= b;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Equal"), nullptr);
    EXPECT_NE(find(node, "LogicalOr"), nullptr);
    EXPECT_NE(find(node, "LogicalAnd"), nullptr);
}

TEST_P(C3GrammarFixture, parses_assignment_operators) {
    const char *source =
        "fn void ops() {\n"
        "    int x = 5;\n"
        "    x += 1;\n"
        "    x -= 1;\n"
        "    x *= 2;\n"
        "    x /= 2;\n"
        "    x %= 3;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PlusAssign"), nullptr);
}

TEST_P(C3GrammarFixture, parses_ternary_expression) {
    const char *source =
        "fn int max(int a, int b) {\n"
        "    return a > b ? a : b;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Question"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_function_calls) {
    const char *source =
        "fn void call() {\n"
        "    foo();\n"
        "    bar(1, 2, 3);\n"
        "    baz(.x = 1, .y = 2);\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, parses_array_subscript) {
    const char *source =
        "fn int get(int[] arr) {\n"
        "    return arr[0];\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PostfixSuffix"), nullptr);
}

TEST_P(C3GrammarFixture, parses_member_access) {
    const char *source =
        "fn int getx(Point p) {\n"
        "    return p.x;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PostfixSuffix"), nullptr);
}

TEST_P(C3GrammarFixture, parses_number_literals) {
    const char *source =
        "fn void nums() {\n"
        "    int a = 42;\n"
        "    int b = 0xFF;\n"
        "    int c = 0b1010;\n"
        "    int d = 0o77;\n"
        "    double e = 3.14;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    // LocalDeclaration nodes should be present for each variable
    EXPECT_NE(find(node, "LocalDeclaration"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_boolean_and_null_literals) {
    const char *source =
        "fn void bools() {\n"
        "    bool t = true;\n"
        "    bool f = false;\n"
        "    void* p = null;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
}

TEST_P(C3GrammarFixture, parses_string_literal) {
    const char *source =
        "fn void strings() {\n"
        "    char* s = \"hello world\";\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
}

TEST_P(C3GrammarFixture, parses_char_literal) {
    const char *source =
        "fn void chars() {\n"
        "    char c = 'a';\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
}

TEST_P(C3GrammarFixture, rejects_struct_literal) {
    const char *source =
        "fn Point make() {\n"
        "    return Point { .x = 1, .y = 2 };\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, rejects_array_literal) {
    const char *source =
        "fn void arr() {\n"
        "    int[3] a = [1, 2, 3];\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

// --- Module-qualified access ---

TEST_P(C3GrammarFixture, parses_module_qualified_identifier) {
    const char *source =
        "fn void io_call() {\n"
        "    std::io::println(\"hi\");\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "QualifiedIdentifier"), nullptr);
}

// --- Type system ---

TEST_P(C3GrammarFixture, parses_pointer_type) {
    const char *source =
        "fn void ptr(int* p) {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeRef"), nullptr);
}

TEST_P(C3GrammarFixture, parses_array_type) {
    const char *source =
        "fn void arr(int[10] a) {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeRef"), nullptr);
}

TEST_P(C3GrammarFixture, parses_builtin_types) {
    const char *source =
        "fn void types(void* p, bool b, char c, int i, long l, double d, float f) {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    // FunctionDeclaration and ParameterList should be present
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "ParameterList"), nullptr);
}

// --- Full programs ---

TEST_P(C3GrammarFixture, parses_complete_program) {
    const char *source =
        "module myapp;\n"
        "\n"
        "import std::io;\n"
        "\n"
        "struct Point {\n"
        "    double x;\n"
        "    double y;\n"
        "}\n"
        "\n"
        "fn double distance(Point a, Point b) {\n"
        "    double dx = a.x - b.x;\n"
        "    double dy = a.y - b.y;\n"
        "    return dx * dx + dy * dy;\n"
        "}\n"
        "\n"
        "fn int main() {\n"
        "    Point p1 = { .x = 0.0, .y = 0.0 };\n"
        "    Point p2 = { .x = 3.0, .y = 4.0 };\n"
        "    double d = distance(p1, p2);\n"
        "    return 0;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ModuleDeclaration"), nullptr);
    EXPECT_NE(find(node, "ImportDeclaration"), nullptr);
    EXPECT_NE(find(node, "StructDeclaration"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(C3GrammarFixture, parses_empty_file) {
    auto *node = parse_source("");
    // An empty file should parse successfully (all parts are optional)
    (void)node;
}

// --- Edge cases ---

TEST_P(C3GrammarFixture, parses_nested_blocks) {
    const char *source =
        "fn void nested() {\n"
        "    {\n"
        "        {\n"
        "            int x = 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Block"), nullptr);
}

TEST_P(C3GrammarFixture, parses_chained_method_calls) {
    const char *source =
        "fn void chain() {\n"
        "    obj.foo().bar(1).baz();\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PostfixExpression"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_range_expression) {
    const char *source =
        "fn void ranges() {\n"
        "    foreach (int i : 0..10) {\n"
        "    }\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, parses_shift_operators) {
    const char *source =
        "fn int shifts(int x) {\n"
        "    return (x << 2) >> 1;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "LShift"), nullptr);
}

TEST_P(C3GrammarFixture, parses_unary_operations) {
    const char *source =
        "fn void unary(int x, bool b) {\n"
        "    int neg = -x;\n"
        "    bool inv = !b;\n"
        "    int flip = ~x;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "UnaryExpression"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_function_type_parameter) {
    const char *source =
        "fn void apply(fn int(int) callback, int val) {\n"
        "    callback(val);\n"
        "}\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, ternary_disambiguation_edge_cases) {
    const char *sources[] = {
        "fn int f() { return a?b:c; }",
        "fn int f() { return a ? b ? c : d : e; }",
        "fn int f() { return a ? b : c ? d : e; }",
        "fn int f() { return a /* condition */ ? /* yes */ b : c; }",
        "fn int f() { return a ? -b : +c; }",
        "fn int f() { return a ? foo(b, c) : arr[0]; }",
        "fn int f() { return a ? b! : c!; }",
        "fn int f() { return a ? (b ? c : d) : e; }",
    };
    for (const char *source : sources) {
        SCOPED_TRACE(source);
        auto *node = parse_source(source);
        ASSERT_NE(node, nullptr);
        EXPECT_NE(find(node, "Question"), nullptr);
        EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);
    }
}

TEST_P(C3GrammarFixture, rejects_obsolete_postfix_question) {
    const char *sources[] = {
        "fn int f() { return value?; }",
        "fn int f() { return (value?); }",
        "fn int f() { return foo(value?, other?); }",
        "fn int f() { return arr[index?]; }",
    };
    for (const char *source : sources) {
        SCOPED_TRACE(source);
        parse_source(source, TEXTPARSER_MATCH_NO);
    }
}

TEST_P(C3GrammarFixture, rejects_incomplete_ternaries) {
    const char *sources[] = {
        "fn int f() { return a ? b; }",
        "fn int f() { return a ? b :; }",
        "fn int f() { return a ? b : c ? d :; }",
        "fn int f() { return a ? b ? c : d; }",
    };
    for (const char *source : sources) {
        SCOPED_TRACE(source);
        parse_source(source, TEXTPARSER_MATCH_NO);
    }
}

// Syntax acceptance verified with C3 0.8.4: c3c compile-only -P.
TEST_P(C3GrammarFixture, accepts_valid_neighbors_of_removed_forms) {
    const char *sources[] = {
        "fn Point f() { return Point {1, 2}; }",
        "fn Point f() { return Point {}; }",
        "fn int square(int x) @inline { return x * x; }",
        "enum Status : int { OK, ERR }",
        "fn void call() { foo(); bar(1, 2, 3); }",
        "fn Point make() { return { .x = 1, .y = 2 }; }",
        "fn void arr() { int[3] a = {1, 2, 3}; }",
        "fn void iterate(int[] arr) { foreach (int i : arr) {} }",
        "fn void apply(Callback callback, int val) { callback(val); }",
        "fn int get(int[3] arr) { return arr[1]; }",
    };
    for (const char *source : sources) {
        SCOPED_TRACE(source);
        ASSERT_NE(parse_source(source), nullptr);
        EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);
    }
}

TEST_P(C3GrammarFixture, rejects_removed_forms_at_boundaries) {
    const char *sources[] = {
        "@inline fn void f();",
        "enum Status : int { OK = 0 }",
        "fault Empty {}",
        "fn void f() { foo(.x = 1); }",
        "fn void f() { foo(.x = 1, .y = 2); }",
        "fn Point f() { return Point { .x = 1 }; }",
        "fn int[] f() { return []; }",
        "fn int[] f() { return [1]; }",
        "fn void f() { foreach (int i : 0..10) {} }",
        "fn void f() { foreach (int i : 0..) {} }",
        "fn void f(fn void() callback) {}",
    };
    for (const char *source : sources) {
        SCOPED_TRACE(source);
        parse_source(source, TEXTPARSER_MATCH_NO);
    }
}

TEST_P(C3GrammarFixture, c3_084_alias) {
    ASSERT_NE(parse_source("alias Callback = fn int(int); alias Index = int; fn void f(Callback cb) { cb(1); }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_faults) {
    ASSERT_NE(parse_source("faultdef NOT_FOUND, BAD_INPUT;"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_bitstruct) {
    ASSERT_NE(parse_source("bitstruct Flags : uint { uint low : 0..3; bool enabled : 4; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_enum_data) {
    ASSERT_NE(parse_source("enum Color : int (int code) { RED {1}, BLUE {2} }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_generics) {
    ASSERT_NE(parse_source("module box <Type>; struct Box { Type value; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_generic_type) {
    ASSERT_NE(parse_source("module box; fn void f(Box{int}* b) {}"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_macro) {
    ASSERT_NE(parse_source("macro int square(int x) { return x * x; } macro twice(x) => x + x;"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_macro_at) {
    ASSERT_NE(parse_source("macro @trace(#expr) { #expr; } fn void f() { @trace(foo()); }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_if) {
    ASSERT_NE(parse_source("fn void f() { $if true: int x = 1; $else int y = 2; $endif }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_for) {
    ASSERT_NE(parse_source("fn void f() { $for var $i = 0; $i < 3; $i++: foo($i); $endfor }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_foreach) {
    ASSERT_NE(parse_source("fn void f() { $foreach $x : {1, 2}: foo($x); $endforeach }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_switch) {
    ASSERT_NE(parse_source("fn void f() { $switch 1: $case 1: foo(); $default: bar(); $endswitch }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_assert) {
    ASSERT_NE(parse_source("$assert true; fn void f() { $assert 1 == 1; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_updates) {
    ASSERT_NE(parse_source("fn void f() { int i = 0; ++i; i++; --i; i--; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_optionals) {
    ASSERT_NE(parse_source("fn int? f() { return read()!; } fn int g() { return read()!!; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_orelse) {
    ASSERT_NE(parse_source("fn int f() { return read() ?? 0; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_elvis) {
    ASSERT_NE(parse_source("fn int f() { return x ?: 0; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_slices) {
    ASSERT_NE(parse_source("fn void f(int[] a) { foo(a[1..3]); foo(a[..]); foo(a[^1]); }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_labeled) {
    ASSERT_NE(parse_source("fn void f() { for OUTER: (;;) { break OUTER; } }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_if_try) {
    ASSERT_NE(parse_source("fn void f() { if (try x = read()) { foo(x); } }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_if_catch) {
    ASSERT_NE(parse_source("fn void f() { if (catch err = read()) { foo(err); } }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_function_short) {
    ASSERT_NE(parse_source("fn int square(int x) => x * x;"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_switch_range) {
    ASSERT_NE(parse_source("fn void f(int x) { switch (x) { case 1..3: break; default: break; } }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_attributes) {
    ASSERT_NE(parse_source("fn void f() @inline {} struct Foo @packed { int x; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_attrdef) {
    ASSERT_NE(parse_source("attrdef @Fast = @inline;"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_vectors) {
    ASSERT_NE(parse_source("fn void f() { int[<4>] a = {1,2,3,4}; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_inferred_array) {
    ASSERT_NE(parse_source("fn void f() { int[*] a = {1,2,3}; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_builtin) {
    ASSERT_NE(parse_source("fn void f() { int n = @sizeOf(int); }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_function_type) {
    ASSERT_NE(parse_source("alias Callback = fn int(int);"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_string_escape) {
    ASSERT_NE(parse_source("fn void f() { char* s = \"a\\\"b\"; char c = '\\n'; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_multi_module) {
    ASSERT_NE(parse_source("module first; fn void a() {} module second; fn void b() {}"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_empty_statement) {
    ASSERT_NE(parse_source("fn void f() { ; for (;;) ; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_asm_string) {
    ASSERT_NE(parse_source("fn void f() { asm(\"nop\"); }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_const_local) {
    ASSERT_NE(parse_source("fn int f() { const int VALUE = 1; return VALUE; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_qualified) {
    ASSERT_NE(parse_source("fn void f(foo::bar::Thing x) {}"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_multi_vars) {
    ASSERT_NE(parse_source("int a, b; fn void f() { int x, y; }"), nullptr);
}


TEST_P(C3GrammarFixture, recovers_bad_statement_and_retains_following_code) {
    auto *node = parse_source("fn int f() { int broken = ; return 7; } fn void next() {}",
                              TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
    textparser_diagnostic d = {};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &d), 0);
    EXPECT_STREQ(d.code, "C31003");
    EXPECT_GT(d.length, 0u);
    EXPECT_LT(d.start_pos, 27u);
}

TEST_P(C3GrammarFixture, rejects_incomplete_modern_constructs) {
    const char *sources[] = {
        "alias Callback = fn int(;",
        "faultdef ;",
        "bitstruct Flags : uint { bool enabled : ; }",
        "fn void f() { $if true: foo(); }",
        "fn void f() { $for var $i = 0; $i < 2; $i++: foo(); }",
        "fn void f() { $foreach $x : {1,2}: foo(); }",
        "fn void f() { $switch 1: $case 1: foo(); }",
        "fn int f() => ;",
        "fn int f() { return value ?? ; }",
        "fn void f() { foo(a[1..); }",
    };
    for (const char *source : sources) parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, comments_only_source) {
    parse_source("// line\n/* block */\n");
}

TEST_P(C3GrammarFixture, c3_precedence_and_assignment_associativity) {
    auto *node = parse_source("fn int f() { return a + b << c; }");
    auto *add = find(node, "Plus");
    ASSERT_NE(add, nullptr);
    ASSERT_NE(add->child, nullptr);
    ASSERT_NE(add->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), add->child->next), "LShift");

    node = parse_source("fn void f() { a = b = c; }");
    auto *assign = find(node, "Assign");
    ASSERT_NE(assign, nullptr);
    ASSERT_NE(assign->child, nullptr);
    ASSERT_NE(assign->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), assign->child->next), "Assign");
}

TEST_P(C3GrammarFixture, c3_084_asm_block) {
    ASSERT_NE(parse_source("fn void f() { asm { nop; } }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_asm_operands) {
    ASSERT_NE(parse_source("fn void f() { asm { mov $eax, 1; } }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_lambda) {
    ASSERT_NE(parse_source("fn void f() { var callback = fn int(int x) => x + 1; }"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_condition_init) {
    parse_source("fn void f() { if (int x = get(); x > 0) { foo(x); } }", TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, c3_084_properties) {
    ASSERT_NE(parse_source("fn void f() { int x = int::max; var k = Foo::kind; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_hexfloat) {
    ASSERT_NE(parse_source("fn double f() { return 0x1.2p3; }"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_numeric_suffix) {
    parse_source("fn void f() { int x = 10i32; float y = 1.5f; }", TEXTPARSER_MATCH_NO);
}

TEST_P(C3GrammarFixture, c3_084_nested_comments) {
    ASSERT_NE(parse_source("/* outer /* inner */ outer */ fn void f() {}"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_macro_body) {
    ASSERT_NE(parse_source("macro run(; @body()) { @body(); }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_named_arg) {
    ASSERT_NE(parse_source("fn void f() { foo(x: 1); }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_indexed_init) {
    ASSERT_NE(parse_source("fn void f() { int[3] a = { [0] = 1, [2] = 3 }; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_nested) {
    ASSERT_NE(parse_source("fn void f() { $if true: $if false: foo(); $else bar(); $endif $endif }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_switch_nocond) {
    ASSERT_NE(parse_source("fn void f() { switch { case true: foo(); default: bar(); } }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_logic) {
    ASSERT_NE(parse_source("fn bool f() { return a &&& b ||| c; }"), nullptr);
}

TEST_P(C3GrammarFixture, c3_084_ct_ternary) {
    ASSERT_NE(parse_source("fn int f() { return a ??? b : c; }"), nullptr);
}

TEST(C3DefinitionParity, identical_tree_and_recovery_diagnostics) {
    textparser_language_definition *json = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                  "definitions/c3_definition.json", &json), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(json, nullptr);
    const char *source =
        "module corpus; alias Callback = fn int(int); faultdef FAILED; "
        "bitstruct Flags : uint { bool enabled : 0; } "
        "fn int f() { int broken = ; return a + b << c; } "
        "macro twice(x) => x + x;";
    std::string trees[2], diagnostics[2];
    const textparser_language_definition *definitions[] = { &c3_definition, json };
    for (int i = 0; i < 2; ++i) {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem(source, std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(definitions[i]), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(definitions[i], &result), 0);
        ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
        ASSERT_NE(result.node, nullptr);
        EXPECT_EQ(textparser_node_get_category(result.node), TEXTPARSER_CST_SOURCE_FILE);
        std::function<void(const textparser_node *)> visit = [&](const textparser_node *n) {
            for (; n; n = n->next) {
                const char *name = textparser_grammar_node_name(parser.get(), n);
                trees[i] += "(" + std::string(name ? name : "") + ":" +
                    std::to_string(n->source_start) + ":" + std::to_string(n->source_end) +
                    ":" + std::to_string(textparser_node_get_category(n));
                visit(n->child);
                trees[i] += ")";
            }
        };
        visit(result.node);
        ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
        for (size_t j = 0; j < textparser_get_diagnostic_count(parser.get()); ++j) {
            textparser_diagnostic d = {};
            ASSERT_EQ(textparser_get_diagnostic(parser.get(), j, &d), 0);
            diagnostics[i] += std::string(d.code) + ":" + d.message + ":" +
                std::to_string(d.start_pos) + ":" + std::to_string(d.length);
        }
    }
    EXPECT_EQ(trees[0], trees[1]);
    EXPECT_EQ(diagnostics[0], diagnostics[1]);
    textparser_free_language_definition(json);
}

TEST_P(C3GrammarFixture, parses_documented_declaration) {
    ASSERT_NE(parse_source("<* documentation *> fn void documented() {}"), nullptr);
}

TEST_P(C3GrammarFixture, rejects_orphan_documentation) {
    parse_source("<* documentation *>", TEXTPARSER_MATCH_NO);
}

} // namespace

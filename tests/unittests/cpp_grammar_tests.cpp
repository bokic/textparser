#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <cpp_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct CppGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/cpp_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &cpp_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, CppGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(CppGrammarFixture, parses_translation_unit_and_preprocessor) {
    const char *source =
        "#include <iostream>\n"
        "#include <vector>\n"
        "#define BUFFER_SIZE 1024\n"
        "using namespace std;\n"
        ";\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_namespaces_and_using_declarations) {
    const char *source =
        "namespace Outer {\n"
        "    namespace Inner {\n"
        "        void fn() {}\n"
        "    }\n"
        "}\n"
        "namespace A::B::C {\n"
        "    int value = 42;\n"
        "}\n"
        "namespace {\n"
        "    void internal_helper() {}\n"
        "}\n"
        "using namespace Outer::Inner;\n"
        "using StringList = std::vector<std::string>;\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_classes_inheritance_and_members) {
    const char *source =
        "class Base {\n"
        "public:\n"
        "    Base() = default;\n"
        "    virtual ~Base() = default;\n"
        "    virtual void print() const = 0;\n"
        "protected:\n"
        "    int id = 0;\n"
        "};\n"
        "class Derived : public Base {\n"
        "public:\n"
        "    explicit Derived(int initial_id)\n"
        "        : Base(), id_val(initial_id) {}\n"
        "    ~Derived() override {}\n"
        "    void print() const override {\n"
        "        std::cout << id_val << std::endl;\n"
        "    }\n"
        "private:\n"
        "    int id_val;\n"
        "};\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_operator_overloading) {
    const char *source =
        "struct Vector2D {\n"
        "    double x, y;\n"
        "    Vector2D operator+(const Vector2D &other) const {\n"
        "        return {x + other.x, y + other.y};\n"
        "    }\n"
        "    bool operator==(const Vector2D &other) const {\n"
        "        return x == other.x && y == other.y;\n"
        "    }\n"
        "    double operator[](int index) const {\n"
        "        return (index == 0) ? x : y;\n"
        "    }\n"
        "    void operator()() const {}\n"
        "};\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_concepts) {
    const char *source =
        "template <typename T>\n"
        "concept Addable = requires(T a, T b) {\n"
        "    a + b;\n"
        "};\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_template_class) {
    const char *source =
        "template <typename T, int N = 10>\n"
        "class FixedArray {\n"
        "    T elements[N];\n"
        "public:\n"
        "    constexpr size_t size() const { return N; }\n"
        "};\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_template_function) {
    const char *source =
        "template <typename... Args>\n"
        "void log_all(Args... args) {}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_lambdas_and_trailing_return_types) {
    const char *source =
        "void lambda_test() {\n"
        "    int factor = 2;\n"
        "    auto square = [](int x) -> int { return x * x; };\n"
        "    auto multiply = [factor](int x) { return x * factor; };\n"
        "    auto capture_all = [&](int x) mutable noexcept { return x + factor; };\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_range_for_and_structured_bindings) {
    const char *source =
        "void modern_statements() {\n"
        "    std::vector<int> nums = {1, 2, 3};\n"
        "    for (const auto &item : nums) {\n"
        "        if (item == 2) break;\n"
        "    }\n"
        "    auto [first, second] = std::make_pair(1, 2);\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_try_catch_and_exceptions) {
    const char *source =
        "void exception_test() {\n"
        "    try {\n"
        "        throw std::runtime_error(\"error\");\n"
        "    } catch (const std::exception &e) {\n"
        "        return;\n"
        "    } catch (...) {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_pratt_expression_precedence_and_casts) {
    const char *source =
        "void expr_test() {\n"
        "    int a = 10, b = 20;\n"
        "    auto cmp = a <=> b;\n"
        "    bool flag = a < b && b > 0 || a == b;\n"
        "    double d = static_cast<double>(a) / b;\n"
        "    const int *cp = const_cast<const int*>(&a);\n"
        "    void *vp = reinterpret_cast<void*>(cp);\n"
        "    int *p = new int(42);\n"
        "    delete p;\n"
        "    int *arr = new int[10];\n"
        "    delete[] arr;\n"
        "}\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, parses_coroutines_and_modern_specifiers) {
    const char *source =
        "Task coroutine_demo() {\n"
        "    co_await std::suspend_always{};\n"
        "    co_yield 42;\n"
        "    co_return;\n"
        "}\n"
        "constexpr int compile_time_val = 100;\n"
        "consteval int consteval_calc(int x) { return x * 2; }\n"
        "decltype(compile_time_val) copy_val = compile_time_val;\n";
    EXPECT_NE(parse_source(source), nullptr);
}

TEST_P(CppGrammarFixture, recovers_from_syntax_errors) {
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

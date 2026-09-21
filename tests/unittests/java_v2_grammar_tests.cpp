#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <java_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct JavaGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/java_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &java_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionPaths, JavaGrammarFixture, testing::Values(false, true),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(JavaGrammarFixture, parses_basic_class_methods_and_fields) {
    const char *source =
        "package com.example.service;\n"
        "import java.util.List;\n"
        "import static java.lang.Math.PI;\n"
        "public class Calculator {\n"
        "    private int total = 0;\n"
        "    public Calculator(int initial) {\n"
        "        this.total = initial;\n"
        "    }\n"
        "    public int add(int a, int b) {\n"
        "        return a + b;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PackageDeclaration"), nullptr);
    EXPECT_NE(find(node, "ImportDeclaration"), nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
    EXPECT_NE(find(node, "ConstructorDeclaration"), nullptr);
    EXPECT_NE(find(node, "MethodDeclaration"), nullptr);
    EXPECT_NE(find(node, "FieldDeclaration"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_records_and_compact_constructor) {
    const char *source =
        "public record Point(int x, int y) implements java.io.Serializable {\n"
        "    public Point {\n"
        "        if (x < 0 || y < 0) {\n"
        "            throw new IllegalArgumentException(\"Negative\");\n"
        "        }\n"
        "    }\n"
        "    public int sum() {\n"
        "        return x + y;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "RecordDeclaration"), nullptr);
    EXPECT_NE(find(node, "CompactConstructorDeclaration"), nullptr);
    EXPECT_NE(find(node, "MethodDeclaration"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_sealed_classes_interfaces_and_permits) {
    const char *source =
        "public sealed interface Shape permits Circle, Rectangle {\n"
        "    double area();\n"
        "}\n"
        "final class Circle implements Shape {\n"
        "    private final double radius;\n"
        "    public Circle(double r) { this.radius = r; }\n"
        "    public double area() { return 3.14 * radius * radius; }\n"
        "}\n"
        "non-sealed class Rectangle implements Shape {\n"
        "    private final double w, h;\n"
        "    public Rectangle(double w, double h) { this.w = w; this.h = h; }\n"
        "    public double area() { return w * h; }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InterfaceDeclaration"), nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_enums_with_constructors_and_constants) {
    const char *source =
        "public enum Operation {\n"
        "    PLUS(\"+\") {\n"
        "        public double apply(double x, double y) { return x + y; }\n"
        "    },\n"
        "    MINUS(\"-\") {\n"
        "        public double apply(double x, double y) { return x - y; }\n"
        "    };\n"
        "    private final String symbol;\n"
        "    Operation(String symbol) { this.symbol = symbol; }\n"
        "    public abstract double apply(double x, double y);\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "EnumDeclaration"), nullptr);
    EXPECT_NE(find(node, "EnumConstant"), nullptr);
    EXPECT_NE(find(node, "ConstructorDeclaration"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_modern_switch_rules_and_pattern_matching) {
    const char *source =
        "public class SwitchDemo {\n"
        "    public int process(Object obj) {\n"
        "        return switch (obj) {\n"
        "            case Integer i -> i * 2;\n"
        "            case String s when s.length() > 5 -> s.length();\n"
        "            case null, default -> 0;\n"
        "        };\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SwitchExpression"), nullptr);
    EXPECT_NE(find(node, "SwitchRule"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_try_with_resources_and_multicatch) {
    const char *source =
        "public class ResourceTest {\n"
        "    public void run() {\n"
        "        try (AutoCloseable a = open(); AutoCloseable b = open()) {\n"
        "            work();\n"
        "        } catch (IOException | SQLException ex) {\n"
        "            ex.printStackTrace();\n"
        "        } finally {\n"
        "            cleanup();\n"
        "        }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TryStatement"), nullptr);
    EXPECT_NE(find(node, "CatchClause"), nullptr);
    EXPECT_NE(find(node, "FinallyClause"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_generics_bounds_and_wildcards) {
    const char *source =
        "@SuppressWarnings(\"unchecked\")\n"
        "public class GenericBox<T extends Comparable<? super T> > {\n"
        "    private List<? extends Number> numbers;\n"
        "    public <U extends List<T> > void process(U list) {\n"
        "        Collections.sort(list);\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeParameterList"), nullptr);
    EXPECT_NE(find(node, "WildcardType"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_lambdas_method_references_and_operators) {
    const char *source =
        "public class FunctionalTest {\n"
        "    public void test() {\n"
        "        Runnable r = () -> {};\n"
        "        Function<String, Integer> f = String::length;\n"
        "        Supplier<List<String> > s = ArrayList::new;\n"
        "        int shift = 0x80000000 >>> 2;\n"
        "        int ternary = (shift > 0) ? shift : -1;\n"
        "        int[] arr = new int[]{ 1, 2, 3 };\n"
        "        int item = arr[0];\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "LambdaExpression"), nullptr);
    EXPECT_NE(find(node, "MethodReferenceSuffix"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_literals_text_blocks_and_numbers) {
    const char *source =
        "public class LiteralsTest {\n"
        "    String text = \"\"\"\n"
        "        SELECT *\n"
        "        FROM users\n"
        "        WHERE id = 1\n"
        "        \"\"\";\n"
        "    char c = '\\n';\n"
        "    double d = 1.23e-4;\n"
        "    long hex = 0xDEAD_BEEFL;\n"
        "    int bin = 0b1010_0101;\n"
        "    boolean b = true;\n"
        "    Object o = null;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_module_declaration) {
    const char *source =
        "module com.example.myapp {\n"
        "    requires java.base;\n"
        "    requires transitive java.logging;\n"
        "    exports com.example.api;\n"
        "    opens com.example.internal to com.google.gson;\n"
        "    uses com.example.spi.Service;\n"
        "    provides com.example.spi.Service with com.example.internal.ServiceImpl;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ModuleDeclaration"), nullptr);
}

TEST_P(JavaGrammarFixture, parses_annotations_and_annotation_types) {
    const char *source =
        "public @interface Validated {\n"
        "    String value() default \"all\";\n"
        "    int level() default 1;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AnnotationTypeDeclaration"), nullptr);
    EXPECT_NE(find(node, "AnnotationTypeElement"), nullptr);
}

TEST_P(JavaGrammarFixture, recovers_from_missing_semicolons_in_declarations) {
    const char *source =
        "public class Sample {\n"
        "    int a = 10\n"
        "    int b = 20;\n"
        "}\n";
    parser.reset();
    EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(parser.parse(get_definition()), 0);
    result = {};
    int exec_rc = parser.execute_language_grammar(get_definition(), &result);
    EXPECT_EQ(exec_rc, 0);
}

} // namespace

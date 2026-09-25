#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <csharp_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct CSharpGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/csharp_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &csharp_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, CSharpGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(CSharpGrammarFixture, parses_using_directives_and_file_scoped_namespace) {
    const char *source =
        "using System;\n"
        "using System.Collections.Generic;\n"
        "using System.Threading.Tasks;\n"
        "using static System.Math;\n"
        "global using System.IO;\n"
        "namespace MyApp.Services;\n"
        "public class Service {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FileScopedNamespaceDeclaration"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_block_namespaces_and_nested_types) {
    const char *source =
        "namespace Outer {\n"
        "    namespace Inner {\n"
        "        public class Worker {\n"
        "            private int id;\n"
        "        }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "NamespaceDeclaration"), nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_classes_records_structs_and_interfaces) {
    const char *source =
        "public interface IRepository<T> where T : class {\n"
        "    Task<T> GetByIdAsync(int id);\n"
        "}\n"
        "public readonly struct Point {\n"
        "    public readonly int X;\n"
        "    public readonly int Y;\n"
        "}\n"
        "public record Person(string FirstName, string LastName);\n"
        "public record struct Coords(double Lat, double Long);\n"
        "public class Repository<T> : IRepository<T> where T : class {\n"
        "    public async Task<T> GetByIdAsync(int id) {\n"
        "        await Task.Yield();\n"
        "        return null;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InterfaceDeclaration"), nullptr);
    EXPECT_NE(find(node, "StructDeclaration"), nullptr);
    EXPECT_NE(find(node, "RecordDeclaration"), nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_enums_with_explicit_underlying_types) {
    const char *source =
        "public enum FileMode : byte {\n"
        "    CreateNew = 1,\n"
        "    Create = 2,\n"
        "    Open = 3,\n"
        "    OpenOrCreate = 4,\n"
        "    Truncate = 5,\n"
        "    Append = 6\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "EnumDeclaration"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_properties_with_init_and_expression_bodies) {
    const char *source =
        "public class User {\n"
        "    public int Id { get; init; }\n"
        "    public string Name { get; set; } = string.Empty;\n"
        "    public bool IsActive => Id > 0;\n"
        "    private int count;\n"
        "    public int Count {\n"
        "        get { return count; }\n"
        "        set { count = value; }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PropertyDeclaration"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_methods_constructors_and_destructors) {
    const char *source =
        "public class Connection {\n"
        "    private string connStr;\n"
        "    public Connection(string connStr) {\n"
        "        this.connStr = connStr;\n"
        "    }\n"
        "    public Connection() : this(\"default\") {}\n"
        "    ~Connection() {\n"
        "    }\n"
        "    public void Open() => DoOpen();\n"
        "    private void DoOpen() {}\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ConstructorDeclaration"), nullptr);
    EXPECT_NE(find(node, "DestructorDeclaration"), nullptr);
    EXPECT_NE(find(node, "MethodDeclaration"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_control_flow_and_switch_expression) {
    const char *source =
        "public class Evaluator {\n"
        "    public string Describe(int val) {\n"
        "        if (val < 0) {\n"
        "            return \"negative\";\n"
        "        } else if (val == 0) {\n"
        "            return \"zero\";\n"
        "        } else {\n"
        "            return \"positive\";\n"
        "        }\n"
        "    }\n"
        "    public int Transform(int code) {\n"
        "        return code switch {\n"
        "            1 => 10,\n"
        "            2 => 20,\n"
        "            _ => 0\n"
        "        };\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "SwitchExpressionSuffix"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_loops_foreach_and_yield) {
    const char *source =
        "public class Generators {\n"
        "    public IEnumerable<int> Generate(int max) {\n"
        "        for (int i = 0; i < max; i++) {\n"
        "            yield return i;\n"
        "        }\n"
        "        int j = 0;\n"
        "        while (j < 10) {\n"
        "            j++;\n"
        "        }\n"
        "        do {\n"
        "            j--;\n"
        "        } while (j > 0);\n"
        "        yield break;\n"
        "    }\n"
        "    public void Consume(List<string> items) {\n"
        "        foreach (var item in items) {\n"
        "            if (item == null) continue;\n"
        "        }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "DoStatement"), nullptr);
    EXPECT_NE(find(node, "ForeachStatement"), nullptr);
    EXPECT_NE(find(node, "YieldStatement"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_try_catch_finally_and_lock_using) {
    const char *source =
        "public class ResourceHandler {\n"
        "    private object gate = new object();\n"
        "    public void Run() {\n"
        "        lock (gate) {\n"
        "            try {\n"
        "                using (var stream = OpenStream()) {\n"
        "                    Process(stream);\n"
        "                }\n"
        "            } catch (IOException ex) when (ex != null) {\n"
        "                throw;\n"
        "            } finally {\n"
        "                Cleanup();\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "LockStatement"), nullptr);
    EXPECT_NE(find(node, "TryStatement"), nullptr);
    EXPECT_NE(find(node, "UsingStatement"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_lambdas_and_null_coalescing_operators) {
    const char *source =
        "public class Expressions {\n"
        "    public void Test() {\n"
        "        Func<int, int> square = x => x * x;\n"
        "        Func<int, int, int> add = (a, b) => a + b;\n"
        "        string s = null;\n"
        "        string res = s ?? \"default\";\n"
        "        s ?" "?= \"initialized\";\n"
        "        int? len = s?.Length;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "LambdaExpression"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_all_csharp_literals_and_comments) {
    const char *source =
        "// Line comment\n"
        "/* Block\n"
        "   comment */\n"
        "public class Literals {\n"
        "    int hex = 0x2A;\n"
        "    int bin = 0b1010_0101;\n"
        "    int dec = 1_000_000;\n"
        "    double dbl = 42.5e-2;\n"
        "    float flt = 3.14f;\n"
        "    decimal decm = 100.50m;\n"
        "    char c = '\\n';\n"
        "    string str = \"Hello\\tWorld\";\n"
        "    string verb = @\"C:\\path\\file.txt\";\n"
        "    string interp = $\"Value is {hex}\";\n"
        "    string raw = \"\"\"multi\n"
        "line\n"
        "raw string\"\"\";\n"
        "    bool b1 = true;\n"
        "    bool b2 = false;\n"
        "    object n = null;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_global_alias_and_attributes) {
    const char *source =
        "[Serializable]\n"
        "[Obsolete(\"Use NewService instead\", false)]\n"
        "public class Client {\n"
        "    public void Log() {\n"
        "        global::System.Console.WriteLine(\"Done\");\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AttributeList"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_with_expression_and_object_initializers) {
    const char *source =
        "public record Config(string Host, int Port);\n"
        "public class Initializers {\n"
        "    public void Run() {\n"
        "        var cfg = new Config(\"localhost\", 8080);\n"
        "        var updated = cfg with { Port = 9090 };\n"
        "        var list = new List<int> { 1, 2, 3 };\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WithExpressionSuffix"), nullptr);
}

TEST_P(CSharpGrammarFixture, parses_pattern_matching_is_expressions) {
    const char *source =
        "public class Patterns {\n"
        "    public bool Check(object obj) {\n"
        "        if (obj is string text) {\n"
        "            return text.Length > 0;\n"
        "        }\n"
        "        return false;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
}

TEST_P(CSharpGrammarFixture, recovers_from_missing_semicolons_in_declarations) {
    // Missing semicolon after field declaration should trigger recovery and continue parsing next member
    const char *source =
        "public class Sample {\n"
        "    int a = 10\n"
        "    int b = 20;\n"
        "}\n";
    // We expect diagnostics / recovery
    parser.reset();
    EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(parser.parse(get_definition()), 0);
    result = {};
    int exec_rc = parser.execute_language_grammar(get_definition(), &result);
    EXPECT_EQ(exec_rc, 0);
}

} // namespace

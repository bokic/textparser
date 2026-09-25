#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <swift_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct SwiftGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/swift_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &swift_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, SwiftGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(SwiftGrammarFixture, parses_imports_and_attributes) {
    const char *source =
        "@main\n"
        "import Foundation\n"
        "import class UIKit.UIViewController\n"
        "import func Darwin.sqrt\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ImportDeclaration"), nullptr);
    EXPECT_NE(find(node, "Attribute"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_constants_variables_and_accessors) {
    const char *source =
        "let pi: Double = 3.14159\n"
        "var count = 42\n"
        "var (x, y) = (10, 20)\n"
        "var name: String {\n"
        "    get {\n"
        "        return _name\n"
        "    }\n"
        "    set {\n"
        "        _name = newValue\n"
        "    }\n"
        "}\n"
        "var score: Int = 0 {\n"
        "    willSet {\n"
        "        print(newValue)\n"
        "    }\n"
        "    didSet {\n"
        "        print(oldValue)\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
    EXPECT_NE(find(node, "AccessorBlock"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_functions_methods_and_operator_overloads) {
    const char *source =
        "func greet(to name: String = \"World\") -> String {\n"
        "    return \"Hello, \" + name\n"
        "}\n"
        "func fetchRemoteData() async throws -> [String] {\n"
        "    return []\n"
        "}\n"
        "func sum(_ numbers: Int...) -> Int {\n"
        "    return 0\n"
        "}\n"
        "static func + (lhs: Vector, rhs: Vector) -> Vector {\n"
        "    return Vector()\n"
        "}\n"
        "func find<T: Equatable, U>(in items: [T]) -> U? where U: Hashable {\n"
        "    return nil\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "ParameterClause"), nullptr);
    EXPECT_NE(find(node, "Parameter"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_classes_structs_actors_protocols_and_extensions) {
    const char *source =
        "public class Animal: LivingCreature {\n"
        "    private let name: String\n"
        "    init?(name: String) {\n"
        "        self.name = name\n"
        "    }\n"
        "    deinit {\n"
        "        // cleanup\n"
        "    }\n"
        "}\n"
        "struct Point<T: Numeric>: Equatable {\n"
        "    var x: T\n"
        "    var y: T\n"
        "}\n"
        "actor BankAccount {\n"
        "    var balance: Double = 0.0\n"
        "}\n"
        "protocol Identifiable {\n"
        "    var id: String { get }\n"
        "    func identify() -> String\n"
        "}\n"
        "extension Point where T == Int {\n"
        "    mutating func reset() {\n"
        "        self.x = 0\n"
        "        self.y = 0\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
    EXPECT_NE(find(node, "InitializerDeclaration"), nullptr);
    EXPECT_NE(find(node, "DeinitializerDeclaration"), nullptr);
    EXPECT_NE(find(node, "StructDeclaration"), nullptr);
    EXPECT_NE(find(node, "ActorDeclaration"), nullptr);
    EXPECT_NE(find(node, "ProtocolDeclaration"), nullptr);
    EXPECT_NE(find(node, "ExtensionDeclaration"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_enums_with_associated_and_raw_values) {
    const char *source =
        "enum Direction: String {\n"
        "    case north = \"N\"\n"
        "    case south = \"S\"\n"
        "    case east, west\n"
        "}\n"
        "enum Result<T, E> {\n"
        "    case success(T)\n"
        "    case failure(E)\n"
        "}\n"
        "indirect enum LinkedList<Element> {\n"
        "    case empty\n"
        "    case node(Element, LinkedList<Element>)\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "EnumDeclaration"), nullptr);
    EXPECT_NE(find(node, "EnumCaseClause"), nullptr);
    EXPECT_NE(find(node, "EnumCase"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_control_flow_and_pattern_matching) {
    const char *source =
        "func flowTest(opt: Int?, items: [Int]) {\n"
        "    if let val = opt, val > 0 {\n"
        "        print(val)\n"
        "    } else if val < 0 {\n"
        "        print(-val)\n"
        "    } else {\n"
        "        print(0)\n"
        "    }\n"
        "    guard let safe = opt else {\n"
        "        return\n"
        "    }\n"
        "    for item in items where item % 2 == 0 {\n"
        "        if item == 10 {\n"
        "            continue\n"
        "        }\n"
        "    }\n"
        "    while count > 0 {\n"
        "        count -= 1\n"
        "    }\n"
        "    repeat {\n"
        "        count += 1\n"
        "    } while count < 10\n"
        "    switch state {\n"
        "    case 1, 2 where count > 0:\n"
        "        fallthrough\n"
        "    case 3:\n"
        "        break\n"
        "    default:\n"
        "        return\n"
        "    }\n"
        "    do {\n"
        "        try riskyOperation()\n"
        "    } catch SpecificError.notFound where count == 0 {\n"
        "        print(\"not found\")\n"
        "    } catch {\n"
        "        print(\"general error\")\n"
        "    }\n"
        "    defer {\n"
        "        cleanup()\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "GuardStatement"), nullptr);
    EXPECT_NE(find(node, "ForInStatement"), nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "RepeatWhileStatement"), nullptr);
    EXPECT_NE(find(node, "SwitchStatement"), nullptr);
    EXPECT_NE(find(node, "DoCatchStatement"), nullptr);
    EXPECT_NE(find(node, "DeferStatement"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_closures_and_trailing_closures) {
    const char *source =
        "func closureTests() {\n"
        "    let simple = { x, y in x + y }\n"
        "    let complex = { [weak self, unowned delegate] (val: Int) async throws -> String in\n"
        "        return String(val)\n"
        "    }\n"
        "    let mapped = items.map { $0 * 2 }.filter { $0 > 10 }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClosureExpression"), nullptr);
    EXPECT_NE(find(node, "TrailingClosureSuffix"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_collections_literals_and_keypaths) {
    const char *source =
        "func literals() {\n"
        "    let arr = [1, 2, 3, 4]\n"
        "    let dict = [\"key\": \"value\", \"foo\": \"bar\"]\n"
        "    let emptyDict = [:]\n"
        "    let kp = \\Person.name\n"
        "    let sub = arr[0]\n"
        "    let multiline = \"\"\"\n"
        "        line 1\n"
        "        line 2\n"
        "        \"\"\"\n"
        "    let raw = #\"Hello \"world\"\"#\n"
        "    let regex = #/[a-zA-Z]+/#\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ArrayOrDictionaryLiteral"), nullptr);
    EXPECT_NE(find(node, "KeyPathExpression"), nullptr);
    EXPECT_NE(find(node, "SubscriptSuffix"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_compiler_directives) {
    const char *source =
        "#if os(macOS)\n"
        "let platform = \"macOS\"\n"
        "#elseif os(iOS)\n"
        "let platform = \"iOS\"\n"
        "#else\n"
        "let platform = \"other\"\n"
        "#endif\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ConditionalCompilationBlock"), nullptr);
}

TEST_P(SwiftGrammarFixture, parses_pratt_operator_precedence_tree) {
    const char *source =
        "func calc() -> Bool {\n"
        "    return a ?? b || c && d == e + f * g\n"
        "}\n"
        "func rangeAndTernary() {\n"
        "    let r1 = 0...10\n"
        "    let r2 = 0..<count\n"
        "    let result = condition ? val1 : val2\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
}

TEST_P(SwiftGrammarFixture, recovers_from_syntax_errors) {
    const char *source =
        "func broken() {\n"
        "    let x = 10 + \n"
        "    let y = 20\n"
        "}\n";
    parser.reset();
    EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(parser.parse(get_definition()), 0);
    result = {};
    int exec_rc = parser.execute_language_grammar(get_definition(), &result);
    EXPECT_EQ(exec_rc, 0);
}

} // namespace

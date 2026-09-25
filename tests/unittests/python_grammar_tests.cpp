#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <python_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct PythonGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/python_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &python_definition;
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
                std::cout << "DEBUG REMAINING TOKEN: kind=" << remaining->kind << " [" << remaining->start << ".." << remaining->end << "] (source snippet: " << (source + remaining->start) << ")" << std::endl;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, PythonGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(PythonGrammarFixture, parses_imports_modules_and_package_structure) {
    const char *source =
        "import sys, os as operating_system\n"
        "from math import pi, sqrt as square_root\n"
        "from typing import (List, Dict, Tuple, Optional, Any)\n"
        "from . import local_helper\n"
        "from ..subpackage.core import *\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ImportStatement"), nullptr);
    EXPECT_NE(find(node, "ImportFromStatement"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_functions_type_annotations_and_decorators) {
    const char *source =
        "@dataclass\n"
        "@functools.lru_cache(maxsize=128)\n"
        "def compute_metrics(x: int, y: float = 1.0, /, z: str = 'default', *args: Any, flag: bool = True, **kwargs: Any) -> Optional[Dict[str, float]]:\n"
        "    \"\"\"Compute and return metrics dictionary.\"\"\"\n"
        "    total = x + y\n"
        "    return {'total': total}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DecoratedDefinition"), nullptr);
    EXPECT_NE(find(node, "Decorator"), nullptr);
    EXPECT_NE(find(node, "FunctionDefinition"), nullptr);
    EXPECT_NE(find(node, "ParameterList"), nullptr);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_async_functions_and_await) {
    const char *source =
        "async def fetch_user(user_id: int) -> dict:\n"
        "    client = HttpClient()\n"
        "    response = await client.get(f'/users/{user_id}')\n"
        "    data = await response.json()\n"
        "    return data\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AsyncFunctionDefinition"), nullptr);
    EXPECT_NE(find(node, "AwaitExpression"), nullptr);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_classes_inheritance_and_methods) {
    const char *source =
        "class Animal:\n"
        "    species: str = 'Unknown'\n"
        "    def __init__(self, name: str) -> None:\n"
        "        self.name = name\n"
        "\n"
        "class Dog(Animal):\n"
        "    def speak(self) -> str:\n"
        "        return f'{self.name} says woof!'\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClassDefinition"), nullptr);
    EXPECT_NE(find(node, "FunctionDefinition"), nullptr);
    EXPECT_NE(find(node, "AnnotatedAssignmentStatement"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_control_flow_statements) {
    const char *source =
        "if x > 0:\n"
        "    result = 'positive'\n"
        "elif x < 0:\n"
        "    result = 'negative'\n"
        "else:\n"
        "    result = 'zero'\n"
        "\n"
        "while count > 0:\n"
        "    count -= 1\n"
        "else:\n"
        "    pass\n"
        "\n"
        "for item in items:\n"
        "    if item == 'skip':\n"
        "        continue\n"
        "    elif item == 'stop':\n"
        "        break\n"
        "    print(item)\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
    EXPECT_NE(find(node, "AugmentedAssignmentStatement"), nullptr);
    EXPECT_NE(find(node, "ContinueStatement"), nullptr);
    EXPECT_NE(find(node, "BreakStatement"), nullptr);
    EXPECT_NE(find(node, "PassStatement"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_try_except_finally_and_with) {
    const char *source =
        "try:\n"
        "    with open('file.txt', 'r') as f, open('out.txt', 'w') as out:\n"
        "        content = f.read()\n"
        "        out.write(content)\n"
        "except FileNotFoundError as err:\n"
        "    logger.error('File not found')\n"
        "except (IOError, ValueError):\n"
        "    logger.error('IO or value error')\n"
        "except* ExceptionGroup as eg:\n"
        "    logger.error('Exception group')\n"
        "else:\n"
        "    logger.info('Success')\n"
        "finally:\n"
        "    cleanup()\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TryStatement"), nullptr);
    EXPECT_NE(find(node, "WithStatement"), nullptr);
    EXPECT_NE(find(node, "ExceptClause"), nullptr);
    EXPECT_NE(find(node, "FinallyClause"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_pattern_matching_pep634) {
    const char *source =
        "match action:\n"
        "    case 'quit' | 'exit':\n"
        "        sys.exit(0)\n"
        "    case ['go', direction]:\n"
        "        player.move(direction)\n"
        "    case {'type': 'click', 'pos': (x, y)}:\n"
        "        handle_click(x, y)\n"
        "    case Point(x, y=0) if x > 0:\n"
        "        process_point(x)\n"
        "    case [first, *rest] as full_sequence:\n"
        "        process(first, rest)\n"
        "    case _:\n"
        "        default_action()\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "MatchStatement"), nullptr);
    EXPECT_NE(find(node, "CaseClause"), nullptr);
    EXPECT_NE(find(node, "OrPattern"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_comprehensions_and_lambdas) {
    const char *source =
        "squares = [x ** 2 for x in range(10) if x % 2 == 0]\n"
        "mapping = {k: v.strip() for k, v in pairs if k is not None}\n"
        "unique = {x.lower() for x in words}\n"
        "gen = (x * 3 for x in data)\n"
        "calc = lambda a, b=0, *c: a + b + sum(c)\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ListOrComprehensionExpression"), nullptr);
    EXPECT_NE(find(node, "ListComprehension"), nullptr);
    EXPECT_NE(find(node, "DictComprehension"), nullptr);
    EXPECT_NE(find(node, "SetComprehension"), nullptr);
    EXPECT_NE(find(node, "GeneratorComprehension"), nullptr);
    EXPECT_NE(find(node, "LambdaExpression"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_pratt_operator_precedence) {
    const char *source =
        "result = a if condition else b + c * d ** e\n"
        "flag = not a and b or c == d != e in f is g\n"
        "bits = a | b ^ c & d << e >> f\n"
        "walrus_check = (n := len(items)) > 10\n"
        "subscript = matrix[1:10:2, ..., 0]\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
    EXPECT_NE(find(node, "IfKeyword"), nullptr);
    EXPECT_NE(find(node, "Walrus"), nullptr);
    EXPECT_NE(find(node, "IndexOrSliceSuffix"), nullptr);
}

TEST_P(PythonGrammarFixture, parses_python_312_type_aliases) {
    const char *source =
        "type Point[T] = tuple[T, T]\n"
        "type StringList = list[str]\n"
        "type Matrix[T: (int, float)] = list[list[T]]\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeAliasStatement"), nullptr);
}

TEST_P(PythonGrammarFixture, recovers_from_syntax_errors_with_synchronization_tokens) {
    const char *source =
        "def buggy():\n"
        "    x = \n"
        "    y = 20\n";
    parser.reset();
    EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(parser.parse(get_definition()), 0);
    result = {};
    int exec_rc = parser.execute_language_grammar(get_definition(), &result);
    EXPECT_EQ(exec_rc, 0);
}

} // namespace

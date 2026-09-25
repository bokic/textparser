#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <matlab_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct MatlabGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/matlab_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &matlab_definition;
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
            (result.status == TEXTPARSER_MATCH_ERROR || textparser_get_diagnostic_count(parser.get()) != 0)) {
            result.status = TEXTPARSER_MATCH_NO;
        }
        if (textparser_get_diagnostic_count(parser.get()) != 0) {
            size_t diag_count = textparser_get_diagnostic_count(parser.get());
            std::cout << "Diagnostic count: " << diag_count << std::endl;
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] " << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
                }
            }
        }
        if (expected == TEXTPARSER_MATCH_OK && !allow_diagnostics) {
            EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u) << source;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, MatlabGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Basic Functions, Local Functions and Scripts ---

TEST_P(MatlabGrammarFixture, parses_basic_functions_and_scripts) {
    const char *source =
        "% MATLAB compute stats function\n"
        "function [mean_val, std_val] = compute_stats(data)\n"
        "    n = length(data);\n"
        "    if n == 0\n"
        "        mean_val = NaN;\n"
        "        std_val = NaN;\n"
        "        return;\n"
        "    end\n"
        "\n"
        "    total = sum(data);\n"
        "    mean_val = total / n;\n"
        "    sq_diff = sum((data - mean_val).^2);\n"
        "    std_val = sqrt(sq_diff / (n - 1));\n"
        "end\n"
        "\n"
        "function r = helper_square(x)\n"
        "    r = x * x;\n"
        "end\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDefinition"), nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "ReturnStatement"), nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
}

// --- Class Definitions and OOP ---

TEST_P(MatlabGrammarFixture, parses_classdef_and_oop) {
    const char *source =
        "classdef (Sealed, Hidden) Shape < handle & BaseShape\n"
        "    properties (Access = private)\n"
        "        x (1, 1) double = 0.0;\n"
        "        y (1, 1) double = 0.0;\n"
        "    end\n"
        "\n"
        "    properties (Dependent)\n"
        "        Area\n"
        "    end\n"
        "\n"
        "    events (NotifyAccess = protected)\n"
        "        ShapeChanged\n"
        "    end\n"
        "\n"
        "    enumeration\n"
        "        Circle (1)\n"
        "        Square (2)\n"
        "    end\n"
        "\n"
        "    methods (Static)\n"
        "        function obj = Shape(x, y)\n"
        "            obj.x = x;\n"
        "            obj.y = y;\n"
        "        end\n"
        "\n"
        "        function val = get.Area(this)\n"
        "            val = this.x * this.y;\n"
        "        end\n"
        "    end\n"
        "end\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClassDefinition"), nullptr);
    EXPECT_NE(find(node, "PropertiesBlock"), nullptr);
    EXPECT_NE(find(node, "MethodsBlock"), nullptr);
    EXPECT_NE(find(node, "EventsBlock"), nullptr);
    EXPECT_NE(find(node, "EnumerationBlock"), nullptr);
}

// --- Arguments Validation Blocks ---

TEST_P(MatlabGrammarFixture, parses_arguments_validation_blocks) {
    const char *source =
        "function res = scale_vector(v, factor, options)\n"
        "    arguments (Input)\n"
        "        v (1, :) double {mustBeNumeric, mustBeNonempty}\n"
        "        factor (1, 1) double = 1.0\n"
        "        options.Verbose (1, 1) logical = false\n"
        "    end\n"
        "    arguments (Output)\n"
        "        res (1, :)\n"
        "    end\n"
        "    res = v * factor;\n"
        "end\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ArgumentsBlock"), nullptr);
    EXPECT_NE(find(node, "FunctionDefinition"), nullptr);
}

// --- Control Flow Statements ---

TEST_P(MatlabGrammarFixture, parses_control_flow) {
    const char *source =
        "x = 10;\n"
        "if x > 10\n"
        "    y = 1;\n"
        "elseif x == 10\n"
        "    y = 0;\n"
        "else\n"
        "    y = -1;\n"
        "end\n"
        "\n"
        "switch x\n"
        "    case 1\n"
        "        disp('one');\n"
        "    case {2, 3, 10}\n"
        "        disp('match');\n"
        "    otherwise\n"
        "        disp('other');\n"
        "end\n"
        "\n"
        "while x > 0\n"
        "    x = x - 1;\n"
        "    if x == 5\n"
        "        continue;\n"
        "    end\n"
        "    if x == 2\n"
        "        break;\n"
        "    end\n"
        "end\n"
        "\n"
        "for i = 1:10\n"
        "    disp(i);\n"
        "end\n"
        "\n"
        "parfor (j = 1:5, 4)\n"
        "    a = j * 2;\n"
        "end\n"
        "\n"
        "spmd\n"
        "    lab = labindex;\n"
        "end\n"
        "\n"
        "try\n"
        "    error('oops');\n"
        "catch ME\n"
        "    disp(ME.message);\n"
        "end\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "SwitchStatement"), nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
    EXPECT_NE(find(node, "ParforStatement"), nullptr);
    EXPECT_NE(find(node, "SpmdStatement"), nullptr);
    EXPECT_NE(find(node, "TryStatement"), nullptr);
    EXPECT_NE(find(node, "BreakStatement"), nullptr);
    EXPECT_NE(find(node, "ContinueStatement"), nullptr);
}

// --- Matrices and Cell Arrays ---

TEST_P(MatlabGrammarFixture, parses_matrices_and_cell_arrays) {
    const char *source =
        "A = [1, 2, 3; 4, 5, 6; 7, 8, 9];\n"
        "B = [1 2 3; 4 5 6];\n"
        "empty_mat = [];\n"
        "col_vec = [1; 2; 3];\n"
        "C = {1, 'hello', [2, 3]; 4, \"world\", {5, 6}};\n"
        "empty_cell = {};\n"
        "\n"
        "elem = A(1, 2);\n"
        "row = A(1, :);\n"
        "last_elem = A(end);\n"
        "sub = A(1:2, 2:end);\n"
        "cell_elem = C{1, 2};\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "MatrixLiteral"), nullptr);
    EXPECT_NE(find(node, "CellLiteral"), nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
}

// --- Expressions, Operators and Function Handles ---

TEST_P(MatlabGrammarFixture, parses_expressions_and_operators) {
    const char *source =
        "a = 1 + 2 * 3 - 4 / 2;\n"
        "b = a^2 + (b .^ 3);\n"
        "c = A * B + C .* D;\n"
        "d = A / B + C ./ D + A \\ B + C .\\ D;\n"
        "flag = (a > 0) && (b <= 10) || ~(c == d) & (e ~= f) | (g >= h);\n"
        "range = 1:2:10;\n"
        "full_range = 1:100;\n"
        "\n"
        "anon = @(x, y) x.^2 + y.^2;\n"
        "f_sin = @sin;\n"
        "f_method = @MyClass.myStaticMethod;\n"
        "meta = ?MyClass;\n"
        "\n"
        "t1 = A.';\n"
        "t2 = A';\n"
        "str1 = 'Character vector with ''escaped'' quote';\n"
        "str2 = \"String array with \"\"escaped\"\" quote\";\n"
        "hex_val = 0xFF;\n"
        "bin_val = 0b1010;\n"
        "flt_val = 3.14159e-2;\n"
        "imag_val = 2.5i;\n"
        "disp(flag);\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AnonymousFunction"), nullptr);
    EXPECT_NE(find(node, "FunctionHandle"), nullptr);
    EXPECT_NE(find(node, "ExpressionStatement"), nullptr);
}

// --- Commands and System Statements ---

TEST_P(MatlabGrammarFixture, parses_commands_and_system_statements) {
    const char *source =
        "clear all\n"
        "close all\n"
        "clc\n"
        "hold on\n"
        "grid off\n"
        "format long\n"
        "global GLOBAL_A GLOBAL_B\n"
        "persistent PERSISTENT_VAR\n"
        "import matlab.io.*\n"
        "import mypkg.subpkg.MyClass\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CommandStatement"), nullptr);
    EXPECT_NE(find(node, "GlobalStatement"), nullptr);
    EXPECT_NE(find(node, "PersistentStatement"), nullptr);
    EXPECT_NE(find(node, "ImportStatement"), nullptr);
}

// --- Comments and Line Continuation ---

TEST_P(MatlabGrammarFixture, parses_comments_and_line_continuation) {
    const char *source =
        "% Line comment\n"
        "%{ Block comment line 1\n"
        "   Block comment line 2\n"
        "%}\n"
        "x = 1 + ... line continuation\n"
        "    2 + ...\n"
        "    3;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
}

// --- Multi-Assignment with Tilde ---

TEST_P(MatlabGrammarFixture, parses_multi_assignment_and_tilde) {
    const char *source =
        "[rows, cols] = size(A);\n"
        "[~, max_idx] = max(data);\n"
        "[a, ~, c] = deal(1, 2, 3);\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
}

// --- Error Recovery ---

TEST_P(MatlabGrammarFixture, recovers_from_syntax_errors) {
    const char *source =
        "function foo()\n"
        "    disp('Before');\n"
        "    invalid statement ??? @@@ ;\n"
        "    disp('After');\n"
        "end\n";

    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "FunctionDefinition"), nullptr);
}

// --- Rejects Malformed ---

TEST_P(MatlabGrammarFixture, rejects_invalid_syntax) {
    const char *source = "[[[@@@???";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

} // namespace

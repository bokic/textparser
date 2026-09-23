#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <vb_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct VBGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/vb_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &vb_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, VBGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Basic Program & Options ---

TEST_P(VBGrammarFixture, parses_basic_vb_program) {
    const char *source =
        "Option Explicit On\n"
        "Option Strict On\n"
        "Imports System\n"
        "Imports System.Collections.Generic\n"
        "\n"
        "Module Program\n"
        "    Sub Main(args As String())\n"
        "        Console.WriteLine(\"Hello, Visual Basic!\")\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "OptionStatement"), nullptr);
    EXPECT_NE(find(node, "ImportsStatement"), nullptr);
    EXPECT_NE(find(node, "ModuleDeclaration"), nullptr);
    EXPECT_NE(find(node, "SubDeclaration"), nullptr);
}

// --- Class with Members, Properties, Constructors ---

TEST_P(VBGrammarFixture, parses_class_with_members) {
    const char *source =
        "Public Class Greeter\n"
        "    Inherits Object\n"
        "    Implements IDisposable\n"
        "\n"
        "    Private _name As String\n"
        "    Public Property Count As Integer = 0\n"
        "\n"
        "    Public Sub New(name As String)\n"
        "        _name = name\n"
        "    End Sub\n"
        "\n"
        "    Public Function Greet() As String\n"
        "        Dim result As String\n"
        "        result = \"Hello, \" & _name & \"!\"\n"
        "        Return result\n"
        "    End Function\n"
        "\n"
        "    Public ReadOnly Property Name As String\n"
        "        Get\n"
        "            Return _name\n"
        "        End Get\n"
        "    End Property\n"
        "\n"
        "    Public Sub Dispose() Implements IDisposable.Dispose\n"
        "        _name = Nothing\n"
        "    End Sub\n"
        "End Class\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
    EXPECT_NE(find(node, "InheritsStatement"), nullptr);
    EXPECT_NE(find(node, "ImplementsStatement"), nullptr);
    EXPECT_NE(find(node, "FieldDeclaration"), nullptr);
    EXPECT_NE(find(node, "ConstructorDeclaration"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "PropertyDeclaration"), nullptr);
}

// --- Structures, Interfaces, Enums, Delegates ---

TEST_P(VBGrammarFixture, parses_structures_interfaces_enums) {
    const char *source =
        "Public Structure Point\n"
        "    Public X As Double\n"
        "    Public Y As Double\n"
        "End Structure\n"
        "\n"
        "Public Interface IGreeter\n"
        "    Function Greet() As String\n"
        "    Sub SayHello()\n"
        "End Interface\n"
        "\n"
        "Public Enum StatusCode As Integer\n"
        "    Success = 0\n"
        "    Warning = 1\n"
        "    Failed = 2\n"
        "End Enum\n"
        "\n"
        "Public Delegate Sub ActionHandler(message As String)\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StructureDeclaration"), nullptr);
    EXPECT_NE(find(node, "InterfaceDeclaration"), nullptr);
    EXPECT_NE(find(node, "EnumDeclaration"), nullptr);
    EXPECT_NE(find(node, "DelegateDeclaration"), nullptr);
}

// --- Namespaces ---

TEST_P(VBGrammarFixture, parses_namespace_declaration) {
    const char *source =
        "Namespace Utilities.Math\n"
        "    Public Class Calculator\n"
        "        Public Function Add(a As Integer, b As Integer) As Integer\n"
        "            Return a + b\n"
        "        End Function\n"
        "    End Class\n"
        "End Namespace\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "NamespaceDeclaration"), nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
}

// --- Control Flow Statements ---

TEST_P(VBGrammarFixture, parses_if_and_select_statements) {
    const char *source =
        "Module ControlFlow\n"
        "    Sub Test(x As Integer)\n"
        "        If x > 10 Then\n"
        "            Console.WriteLine(\"Large\")\n"
        "        ElseIf x > 0 Then\n"
        "            Console.WriteLine(\"Positive\")\n"
        "        Else\n"
        "            Console.WriteLine(\"Non-positive\")\n"
        "        End If\n"
        "\n"
        "        Select Case x\n"
        "            Case 1, 2, 3\n"
        "                Console.WriteLine(\"Small\")\n"
        "            Case 4 To 10\n"
        "                Console.WriteLine(\"Medium\")\n"
        "            Case Is > 100\n"
        "                Console.WriteLine(\"Huge\")\n"
        "            Case Else\n"
        "                Console.WriteLine(\"Other\")\n"
        "        End Select\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "SelectStatement"), nullptr);
    EXPECT_NE(find(node, "CaseClause"), nullptr);
}

TEST_P(VBGrammarFixture, parses_loop_statements) {
    const char *source =
        "Module Loops\n"
        "    Sub TestLoops()\n"
        "        Dim i As Integer\n"
        "        For i = 1 To 10 Step 2\n"
        "            Console.WriteLine(i)\n"
        "        Next i\n"
        "\n"
        "        For Each item As String In items\n"
        "            Console.WriteLine(item)\n"
        "        Next\n"
        "\n"
        "        While i > 0\n"
        "            i -= 1\n"
        "        End While\n"
        "\n"
        "        Do While i < 5\n"
        "            i += 1\n"
        "        Loop\n"
        "\n"
        "        Do\n"
        "            i += 1\n"
        "        Loop Until i >= 10\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "DoLoopStatement"), nullptr);
}

// --- Exception Handling: Try / Catch / Finally ---

TEST_P(VBGrammarFixture, parses_try_catch_finally) {
    const char *source =
        "Module ExceptionTest\n"
        "    Sub DoWork()\n"
        "        Try\n"
        "            Dim result As Integer = 10 / 0\n"
        "        Catch ex As DivideByZeroException When ex.Message IsNot Nothing\n"
        "            Console.WriteLine(\"Cannot divide by zero\")\n"
        "        Catch ex As Exception\n"
        "            Throw\n"
        "        Finally\n"
        "            Console.WriteLine(\"Clean up\")\n"
        "        End Try\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TryStatement"), nullptr);
    EXPECT_NE(find(node, "CatchClause"), nullptr);
    EXPECT_NE(find(node, "FinallyClause"), nullptr);
    EXPECT_NE(find(node, "ThrowStatement"), nullptr);
}

// --- Using, With, SyncLock ---

TEST_P(VBGrammarFixture, parses_with_using_synclock) {
    const char *source =
        "Module ContextStatements\n"
        "    Sub Test()\n"
        "        With person\n"
        "            .Name = \"Bob\"\n"
        "            .Age = 30\n"
        "        End With\n"
        "\n"
        "        SyncLock lockObj\n"
        "            counter += 1\n"
        "        End SyncLock\n"
        "\n"
        "        Using reader As New StreamReader(\"test.txt\")\n"
        "            Dim line As String = reader.ReadLine()\n"
        "        End Using\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WithStatement"), nullptr);
    EXPECT_NE(find(node, "SyncLockStatement"), nullptr);
    EXPECT_NE(find(node, "UsingStatement"), nullptr);
}

// --- Generics, Casts, TypeOf ---

TEST_P(VBGrammarFixture, parses_generics_and_casts) {
    const char *source =
        "Public Class Container(Of T As {Class, New})\n"
        "    Private _item As T\n"
        "\n"
        "    Public Sub SetItem(val As Object)\n"
        "        If TypeOf val Is T Then\n"
        "            _item = DirectCast(val, T)\n"
        "        Else\n"
        "            _item = TryCast(val, T)\n"
        "        End If\n"
        "        Dim tType As Type = GetType(T)\n"
        "        Dim num As Integer = CInt(123.45)\n"
        "    End Sub\n"
        "End Class\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClassDeclaration"), nullptr);
    EXPECT_NE(find(node, "TypeParameterList"), nullptr);
    EXPECT_NE(find(node, "TypeOfExpression"), nullptr);
    EXPECT_NE(find(node, "CastExpression"), nullptr);
    EXPECT_NE(find(node, "GetTypeExpression"), nullptr);
    EXPECT_NE(find(node, "ConversionExpression"), nullptr);
}

// --- Operator Precedence & Expressions ---

TEST_P(VBGrammarFixture, parses_pratt_operator_precedence) {
    const char *source =
        "Module ExprTest\n"
        "    Sub Test()\n"
        "        Dim a As Integer = 1 + 2 * 3 ^ 4\n"
        "        Dim b As Boolean = x > 10 AndAlso y <= 20 OrElse Not z\n"
        "        Dim c As String = \"Value: \" & (a Mod 5) & \" done\"\n"
        "        Dim d As Integer = (10 \\ 3) + (1 << 4)\n"
        "        Dim e As Boolean = obj Is Nothing OrElse obj IsNot other\n"
        "        a = (a + 10) * 2\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DimStatement"), nullptr);
    EXPECT_NE(find(node, "ExpressionStatement"), nullptr);
}

// --- Single Line If ---

TEST_P(VBGrammarFixture, parses_single_line_if) {
    const char *source =
        "Module SingleLine\n"
        "    Sub Test(x As Integer)\n"
        "        If x > 0 Then Console.WriteLine(\"Positive\") Else Console.WriteLine(\"Negative\")\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
}

// --- Error Recovery ---

TEST_P(VBGrammarFixture, recovers_from_syntax_errors) {
    const char *source =
        "Module Recovery\n"
        "    Sub Main()\n"
        "        Console.WriteLine(\"Before\")\n"
        "        invalid syntax ??? @@@ %%% :\n"
        "        Console.WriteLine(\"After\")\n"
        "    End Sub\n"
        "End Module\n";
    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "ModuleDeclaration"), nullptr);
    EXPECT_NE(find(node, "SubDeclaration"), nullptr);
}

// --- Rejects Malformed ---

TEST_P(VBGrammarFixture, rejects_invalid_syntax) {
    const char *source = "[[[@@@???";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

} // namespace

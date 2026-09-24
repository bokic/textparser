#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <ada_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct AdaGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/ada_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &ada_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, AdaGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Packages (Spec and Body) ---

TEST_P(AdaGrammarFixture, parses_package_spec_and_body) {
    const char *source =
        "with Ada.Text_IO;\n"
        "use Ada.Text_IO;\n"
        "\n"
        "package Geometry is\n"
        "    type Point is record\n"
        "        X : Float;\n"
        "        Y : Float;\n"
        "    end record;\n"
        "    procedure Draw(P : in Point);\n"
        "end Geometry;\n"
        "\n"
        "package body Geometry is\n"
        "    procedure Draw(P : in Point) is\n"
        "    begin\n"
        "        Put_Line(\"Drawing point\");\n"
        "    end Draw;\n"
        "end Geometry;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PackageDeclaration"), nullptr);
    EXPECT_NE(find(node, "PackageBody"), nullptr);
    EXPECT_NE(find(node, "SubprogramBody"), nullptr);
}

// --- Subprograms and Parameter Modes ---

TEST_P(AdaGrammarFixture, parses_subprograms_and_parameter_modes) {
    const char *source =
        "procedure Math_Ops is\n"
        "    function Add(A : in Integer; B : in Integer := 0) return Integer is\n"
        "    begin\n"
        "        return A + B;\n"
        "    end Add;\n"
        "\n"
        "    procedure Swap(X : in out Integer; Y : in out Integer) is\n"
        "        Temp : Integer := X;\n"
        "    begin\n"
        "        X := Y;\n"
        "        Y := Temp;\n"
        "    end Swap;\n"
        "\n"
        "    procedure Output(Val : out Integer) is\n"
        "    begin\n"
        "        Val := 42;\n"
        "    end Output;\n"
        "\n"
        "    function \"+\"(Left, Right : Float) return Float is\n"
        "    begin\n"
        "        return Left + Right;\n"
        "    end \"+\";\n"
        "begin\n"
        "    null;\n"
        "end Math_Ops;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SubprogramBody"), nullptr);
    EXPECT_NE(find(node, "NullStatement"), nullptr);
}

// --- Types and Subtypes ---

TEST_P(AdaGrammarFixture, parses_types_and_subtypes) {
    const char *source =
        "package Type_Defs is\n"
        "    type Day is (Mon, Tue, Wed, Thu, Fri, Sat, Sun);\n"
        "    subtype Weekday is Day range Mon .. Fri;\n"
        "    type Day_Number is range 1 .. 31;\n"
        "    type Byte is mod 256;\n"
        "    type Real is digits 6 range -1.0 .. 1.0;\n"
        "    type Vector is array (1 .. 3) of Float;\n"
        "    type Matrix is array (Integer range <>, Integer range <>) of Float;\n"
        "    type Node;\n"
        "    type Node_Ptr is access all Node;\n"
        "    type Node is record\n"
        "        Data : Integer;\n"
        "        Next : Node_Ptr;\n"
        "    end record;\n"
        "    type Shape is tagged record\n"
        "        ID : Integer;\n"
        "    end record;\n"
        "    type Circle is new Shape with record\n"
        "        Radius : Float;\n"
        "    end record;\n"
        "    type Printable is interface;\n"
        "    type Drawable is tagged null record;\n"
        "end Type_Defs;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PackageDeclaration"), nullptr);
    EXPECT_NE(find(node, "TypeDeclaration"), nullptr);
    EXPECT_NE(find(node, "SubtypeDeclaration"), nullptr);
}

// --- Control Flow ---

TEST_P(AdaGrammarFixture, parses_control_flow) {
    const char *source =
        "procedure Control_Flow is\n"
        "    X : Integer := 10;\n"
        "    Y : Integer := 20;\n"
        "begin\n"
        "    if X < Y then\n"
        "        X := X + 1;\n"
        "    elsif X = Y then\n"
        "        X := 0;\n"
        "    else\n"
        "        X := Y;\n"
        "    end if;\n"
        "\n"
        "    case X is\n"
        "        when 1 =>\n"
        "            Y := 100;\n"
        "        when 2 | 3 =>\n"
        "            Y := 200;\n"
        "        when 4 .. 10 =>\n"
        "            Y := 300;\n"
        "        when others =>\n"
        "            Y := 0;\n"
        "    end case;\n"
        "\n"
        "    loop\n"
        "        X := X - 1;\n"
        "        exit when X = 0;\n"
        "    end loop;\n"
        "\n"
        "    while X < 10 loop\n"
        "        X := X + 1;\n"
        "    end loop;\n"
        "\n"
        "    for I in 1 .. 10 loop\n"
        "        Y := Y + I;\n"
        "    end loop;\n"
        "\n"
        "    for I in reverse 1 .. 10 loop\n"
        "        Y := Y - I;\n"
        "    end loop;\n"
        "end Control_Flow;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "CaseStatement"), nullptr);
    EXPECT_NE(find(node, "LoopStatement"), nullptr);
    EXPECT_NE(find(node, "ExitStatement"), nullptr);
}

// --- Block Statements, Exception Handling, Labels, and Goto ---

TEST_P(AdaGrammarFixture, parses_blocks_exceptions_and_labels) {
    const char *source =
        "procedure Exceptions_And_Blocks is\n"
        "begin\n"
        "    <<Start_Label>>\n"
        "    declare\n"
        "        Local_Val : Integer := 5;\n"
        "    begin\n"
        "        Local_Val := Local_Val * 2;\n"
        "        raise Constraint_Error with \"Out of range\";\n"
        "    exception\n"
        "        when Constraint_Error =>\n"
        "            Local_Val := 0;\n"
        "        when others =>\n"
        "            null;\n"
        "    end;\n"
        "    goto Start_Label;\n"
        "exception\n"
        "    when others =>\n"
        "        null;\n"
        "end Exceptions_And_Blocks;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "BlockStatement"), nullptr);
    EXPECT_NE(find(node, "RaiseStatement"), nullptr);
    EXPECT_NE(find(node, "GotoStatement"), nullptr);
    EXPECT_NE(find(node, "Label"), nullptr);
    EXPECT_NE(find(node, "ExceptionHandler"), nullptr);
}

// --- Concurrency: Tasks and Protected Objects ---

TEST_P(AdaGrammarFixture, parses_concurrency) {
    const char *source =
        "package Concurrency_Test is\n"
        "    task Worker is\n"
        "        entry Start(ID : in Integer);\n"
        "        entry Stop;\n"
        "    end Worker;\n"
        "\n"
        "    task body Worker is\n"
        "    begin\n"
        "        select\n"
        "            accept Start(ID : in Integer) do\n"
        "                null;\n"
        "            end Start;\n"
        "        or\n"
        "            delay 1.0;\n"
        "        or\n"
        "            terminate;\n"
        "        end select;\n"
        "    end Worker;\n"
        "\n"
        "    protected Counter is\n"
        "        function Get return Integer;\n"
        "        procedure Inc;\n"
        "    private\n"
        "        Val : Integer := 0;\n"
        "    end Counter;\n"
        "\n"
        "    protected body Counter is\n"
        "        function Get return Integer is\n"
        "        begin\n"
        "            return Val;\n"
        "        end Get;\n"
        "        procedure Inc is\n"
        "        begin\n"
        "            Val := Val + 1;\n"
        "        end Inc;\n"
        "    end Counter;\n"
        "end Concurrency_Test;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TaskDeclaration"), nullptr);
    EXPECT_NE(find(node, "TaskBody"), nullptr);
    EXPECT_NE(find(node, "ProtectedDeclaration"), nullptr);
    EXPECT_NE(find(node, "ProtectedBody"), nullptr);
    EXPECT_NE(find(node, "SelectStatement"), nullptr);
    EXPECT_NE(find(node, "AcceptStatement"), nullptr);
    EXPECT_NE(find(node, "DelayStatement"), nullptr);
}

// --- Generics ---

TEST_P(AdaGrammarFixture, parses_generics) {
    const char *source =
        "generic\n"
        "    type Element is private;\n"
        "    Capacity : in Integer := 100;\n"
        "package Generic_Stack is\n"
        "    procedure Push(Item : in Element);\n"
        "    function Pop return Element;\n"
        "end Generic_Stack;\n"
        "\n"
        "generic\n"
        "    type Item_Type is private;\n"
        "function Generic_Swap(A, B : Item_Type) return Item_Type;\n"
        "\n"
        "package Int_Stack is new Generic_Stack(Element => Integer, Capacity => 50);\n"
        "function Swap_Ints is new Generic_Swap(Item_Type => Integer);\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "GenericDeclaration"), nullptr);
    EXPECT_NE(find(node, "PackageInstantiation"), nullptr);
    EXPECT_NE(find(node, "SubprogramInstantiation"), nullptr);
}

// --- Attributes, Based Numbers, and Aggregates ---

TEST_P(AdaGrammarFixture, parses_attributes_based_numbers_and_aggregates) {
    const char *source =
        "procedure Features is\n"
        "    Hex_Val : Integer := 16#FF_A0#;\n"
        "    Bin_Val : Integer := 2#1010_1111#;\n"
        "    Oct_Val : Integer := 8#77#;\n"
        "    Exp_Val : Float := 16#F.F#e+2;\n"
        "    Ch : Character := 'Z';\n"
        "    Quote_Ch : Character := '''';\n"
        "    Str : String := \"Ada \"\"rocks\"\"!\";\n"
        "    type Point is record X, Y : Float; end record;\n"
        "    P : Point := (X => 1.0, Y => 2.0);\n"
        "    Arr : array (1 .. 5) of Integer := (1, 2, others => 0);\n"
        "    Pos_Arr : array (1 .. 3) of Integer := (10, 20, 30);\n"
        "    Len : Integer;\n"
        "    Addr : System.Address;\n"
        "begin\n"
        "    Len := Arr'Length;\n"
        "    Addr := Arr'Address;\n"
        "    for I in Arr'Range loop\n"
        "        Arr(I) := Integer'Value(\"123\");\n"
        "    end loop;\n"
        "end Features;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AttributeReference"), nullptr);
    EXPECT_NE(find(node, "RecordAggregate"), nullptr);
    EXPECT_NE(find(node, "PositionalAggregate"), nullptr);
}

// --- Pragmas and Aspects ---

TEST_P(AdaGrammarFixture, parses_pragmas_and_aspects) {
    const char *source =
        "package Pragma_Defs is\n"
        "    pragma Inline(Fast_Func);\n"
        "    pragma Assert(True, \"Assertion message\");\n"
        "    function Fast_Func(X : Integer) return Integer with Inline, Pure;\n"
        "end Pragma_Defs;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PragmaDeclaration"), nullptr);
    EXPECT_NE(find(node, "AspectSpecification"), nullptr);
}

// --- Expressions and Operators ---

TEST_P(AdaGrammarFixture, parses_expressions_and_operators) {
    const char *source =
        "procedure Expressions is\n"
        "    A : Boolean := True and then False or else True;\n"
        "    B : Boolean := not (A xor True);\n"
        "    C : Integer := (10 * 2) + (100 / 5) - (7 mod 3) + (8 rem 3) ** 2;\n"
        "    D : Boolean := C in 1 .. 100;\n"
        "    E : Boolean := C not in 200 .. 300;\n"
        "    F : String := \"Hello \" & \"World\";\n"
        "    G : Integer := Integer'(42);\n"
        "begin\n"
        "    null;\n"
        "end Expressions;\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Plus"), nullptr);
    EXPECT_NE(find(node, "UnaryExpression"), nullptr);
    EXPECT_NE(find(node, "QualifiedExpression"), nullptr);
}

// --- Error Recovery ---

TEST_P(AdaGrammarFixture, recovers_from_syntax_errors) {
    const char *source =
        "procedure Recovery_Test is\n"
        "    X : Integer := 1;\n"
        "    invalid statement foo bar ;\n"
        "    Y : Integer := 2;\n"
        "begin\n"
        "    X := X + 1;\n"
        "    invalid bad foo bar ;\n"
        "    Y := Y + 1;\n"
        "end Recovery_Test;\n";

    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "SubprogramBody"), nullptr);
}

// --- Rejects Malformed ---

TEST_P(AdaGrammarFixture, rejects_invalid_syntax) {
    const char *source = "[[[@@@???";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

} // namespace

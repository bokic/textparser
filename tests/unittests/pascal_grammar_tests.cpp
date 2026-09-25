#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <pascal_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct PascalGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/pascal_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &pascal_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, PascalGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Basic Program ---

TEST_P(PascalGrammarFixture, parses_basic_pascal_program) {
    const char *source =
        "program HelloWorld(output);\n"
        "uses\n"
        "  Crt;\n"
        "var\n"
        "  message: string;\n"
        "begin\n"
        "  message := 'Hello, World!';\n"
        "  WriteLn(message);\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ProgramDeclaration"), nullptr);
    EXPECT_NE(find(node, "UsesClause"), nullptr);
    EXPECT_NE(find(node, "VarSection"), nullptr);
    EXPECT_NE(find(node, "CompoundStatement"), nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
    EXPECT_NE(find(node, "CallStatement"), nullptr);
}

// --- Unit with Interface and Implementation ---

TEST_P(PascalGrammarFixture, parses_unit_with_interface_and_implementation) {
    const char *source =
        "// Delphi unit with class definition\n"
        "{$HINT ON}\n"
        "{ Helper comment }\n"
        "(* Another comment *)\n"
        "unit MyUnit;\n"
        "\n"
        "interface\n"
        "\n"
        "uses\n"
        "  SysUtils, Classes;\n"
        "\n"
        "type\n"
        "  TMyClass = class(TObject)\n"
        "  private\n"
        "    FName: string;\n"
        "    FCount: Integer;\n"
        "  public\n"
        "    constructor Create(const AName: string);\n"
        "    destructor Destroy; override;\n"
        "    property Name: string read FName write FName;\n"
        "    property Count: Integer read FCount write FCount;\n"
        "    function GetStatus: Boolean;\n"
        "  end;\n"
        "\n"
        "implementation\n"
        "\n"
        "constructor TMyClass.Create(const AName: string);\n"
        "begin\n"
        "  FName := AName;\n"
        "  FCount := 0;\n"
        "end;\n"
        "\n"
        "destructor TMyClass.Destroy;\n"
        "begin\n"
        "  inherited;\n"
        "end;\n"
        "\n"
        "function TMyClass.GetStatus: Boolean;\n"
        "var\n"
        "  code: Integer;\n"
        "begin\n"
        "  code := $FF;\n"
        "  if FCount > 100 then\n"
        "    Result := True\n"
        "  else\n"
        "    Result := False;\n"
        "end;\n"
        "\n"
        "const\n"
        "  AppName = 'MyApp';\n"
        "\n"
        "initialization\n"
        "  WriteLn(#65#66#67);\n"
        "finalization\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "UnitDeclaration"), nullptr);
    EXPECT_NE(find(node, "InterfaceSection"), nullptr);
    EXPECT_NE(find(node, "ImplementationSection"), nullptr);
    EXPECT_NE(find(node, "ClassType"), nullptr);
    EXPECT_NE(find(node, "PropertyDeclaration"), nullptr);
    EXPECT_NE(find(node, "RoutineDefinition"), nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "ConstSection"), nullptr);
}

// --- Type Declarations and Structures ---

TEST_P(PascalGrammarFixture, parses_type_declarations_and_structures) {
    const char *source =
        "unit TypesUnit;\n"
        "interface\n"
        "type\n"
        "  TDay = 1..31;\n"
        "  TMonth = (Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec);\n"
        "  TCharSet = set of Char;\n"
        "  TIntArray = array[1..10] of Integer;\n"
        "  TMatrix = array[1..3, 1..3] of Real;\n"
        "  TDynArray = array of string;\n"
        "  PNode = ^TNode;\n"
        "  TNode = record\n"
        "    Data: Integer;\n"
        "    Next: PNode;\n"
        "  end;\n"
        "  TVariantRecord = record\n"
        "    ID: Integer;\n"
        "    case Kind: Integer of\n"
        "      1: (IntValue: Integer);\n"
        "      2: (FloatValue: Double);\n"
        "  end;\n"
        "  TFuncPtr = function(X, Y: Integer): Integer;\n"
        "  TMethodPtr = procedure(Sender: TObject) of object;\n"
        "implementation\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SubrangeType"), nullptr);
    EXPECT_NE(find(node, "EnumerationType"), nullptr);
    EXPECT_NE(find(node, "SetType"), nullptr);
    EXPECT_NE(find(node, "ArrayType"), nullptr);
    EXPECT_NE(find(node, "PointerType"), nullptr);
    EXPECT_NE(find(node, "RecordType"), nullptr);
    EXPECT_NE(find(node, "ProceduralType"), nullptr);
}

// --- Object Pascal Classes and Interfaces ---

TEST_P(PascalGrammarFixture, parses_object_pascal_classes_and_interfaces) {
    const char *source =
        "unit OOPUnit;\n"
        "interface\n"
        "type\n"
        "  IMyInterface = interface\n"
        "    ['{12345678-1234-1234-1234-123456789ABC}']\n"
        "    procedure Process;\n"
        "    function GetCount: Integer;\n"
        "  end;\n"
        "\n"
        "  TBase = class\n"
        "  strict private\n"
        "    FId: Integer;\n"
        "  public\n"
        "    constructor Create; virtual;\n"
        "    property Id: Integer read FId write FId;\n"
        "  end;\n"
        "\n"
        "  TDerived = class(TBase, IMyInterface)\n"
        "  private\n"
        "    FItems: array of string;\n"
        "    function GetItem(Index: Integer): string;\n"
        "  public\n"
        "    procedure Process;\n"
        "    function GetCount: Integer;\n"
        "    property Items[Index: Integer]: string read GetItem; default;\n"
        "  end;\n"
        "\n"
        "  TBaseHelper = class helper for TBase\n"
        "    procedure PrintId;\n"
        "  end;\n"
        "implementation\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InterfaceType"), nullptr);
    EXPECT_NE(find(node, "ClassType"), nullptr);
    EXPECT_NE(find(node, "HelperType"), nullptr);
    EXPECT_NE(find(node, "VisibilitySpecifier"), nullptr);
    EXPECT_NE(find(node, "PropertyDeclaration"), nullptr);
}

// --- Routine Declarations and Modifiers ---

TEST_P(PascalGrammarFixture, parses_routine_declarations_and_modifiers) {
    const char *source =
        "unit RoutinesUnit;\n"
        "interface\n"
        "procedure SimpleProc(X: Integer);\n"
        "function Calculate(const A: Integer; var B: Double; out Success: Boolean): string;\n"
        "procedure Overloaded(A: Integer); overload;\n"
        "procedure Overloaded(const S: string); overload;\n"
        "procedure LibFunc(Code: Integer); cdecl; external 'mylib.so' name 'lib_func';\n"
        "procedure ForwardProc; forward;\n"
        "implementation\n"
        "procedure SimpleProc(X: Integer);\n"
        "  procedure NestedProc;\n"
        "  begin\n"
        "    WriteLn(X);\n"
        "  end;\n"
        "begin\n"
        "  NestedProc;\n"
        "end;\n"
        "procedure ForwardProc;\n"
        "begin\n"
        "end;\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "RoutineDeclaration"), nullptr);
    EXPECT_NE(find(node, "RoutineDefinition"), nullptr);
    EXPECT_NE(find(node, "ParameterDeclaration"), nullptr);
}

// --- Control Flow Statements ---

TEST_P(PascalGrammarFixture, parses_control_flow_statements) {
    const char *source =
        "program ControlFlow;\n"
        "var\n"
        "  i, sum: Integer;\n"
        "  arr: array[1..5] of Integer;\n"
        "  item: Integer;\n"
        "label\n"
        "  exit_loop;\n"
        "begin\n"
        "  sum := 0;\n"
        "  if sum = 0 then\n"
        "    WriteLn('zero')\n"
        "  else\n"
        "    WriteLn('nonzero');\n"
        "\n"
        "  case sum of\n"
        "    0: WriteLn('zero');\n"
        "    1..5: WriteLn('small');\n"
        "    6, 7, 8: WriteLn('medium');\n"
        "    else WriteLn('large');\n"
        "  end;\n"
        "\n"
        "  for i := 1 to 10 do\n"
        "    sum := sum + i;\n"
        "\n"
        "  for i := 10 downto 1 do\n"
        "    sum := sum - i;\n"
        "\n"
        "  for item in arr do\n"
        "    sum += item;\n"
        "\n"
        "  while sum > 0 do\n"
        "    sum := sum - 1;\n"
        "\n"
        "  repeat\n"
        "    sum := sum + 1;\n"
        "  until sum >= 10;\n"
        "\n"
        "  goto exit_loop;\n"
        "exit_loop:\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "CaseStatement"), nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "RepeatStatement"), nullptr);
    EXPECT_NE(find(node, "GotoStatement"), nullptr);
}

// --- Exception Handling ---

TEST_P(PascalGrammarFixture, parses_exception_handling) {
    const char *source =
        "program Exceptions;\n"
        "var\n"
        "  obj: TObject;\n"
        "begin\n"
        "  try\n"
        "    obj := TObject.Create;\n"
        "    try\n"
        "      raise Exception.Create('Error occurred');\n"
        "    except\n"
        "      on E: Exception do\n"
        "        WriteLn(E.Message);\n"
        "      on EConvertError do\n"
        "        WriteLn('conversion error');\n"
        "      else\n"
        "        WriteLn('unknown error');\n"
        "    end;\n"
        "  finally\n"
        "    obj.Free;\n"
        "  end;\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TryStatement"), nullptr);
    EXPECT_NE(find(node, "RaiseStatement"), nullptr);
    EXPECT_NE(find(node, "ExceptionHandler"), nullptr);
}

// --- Expressions and Operator Precedence ---

TEST_P(PascalGrammarFixture, parses_expressions_and_operators) {
    const char *source =
        "program Expressions;\n"
        "var\n"
        "  a, b, c: Integer;\n"
        "  flag: Boolean;\n"
        "  s: string;\n"
        "  charSet: set of Char;\n"
        "  p: ^Integer;\n"
        "begin\n"
        "  a := 10 + 20 * 30 - 40 div 2;\n"
        "  flag := (a > 10) and (b <= 20) or not (c = 0);\n"
        "  s := 'Hello ' + 'World'#13#10;\n"
        "  charSet := ['a'..'z', 'A'..'Z', '_', '0'..'9'];\n"
        "  flag := 'x' in charSet;\n"
        "  p := @a;\n"
        "  b := p^;\n"
        "  c := Obj.Field.SubField;\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
    EXPECT_NE(find(node, "SetConstructor"), nullptr);
    EXPECT_NE(find(node, "MemberAccess"), nullptr);
    EXPECT_NE(find(node, "DereferenceExpression"), nullptr);
    EXPECT_NE(find(node, "UnaryExpression"), nullptr);
}

// --- Inline Assembly ---

TEST_P(PascalGrammarFixture, parses_inline_assembly) {
    const char *source =
        "program AsmExample;\n"
        "function AddAsm(A, B: Integer): Integer;\n"
        "begin\n"
        "  asm\n"
        "    mov eax, A;\n"
        "    add eax, B;\n"
        "    mov Result, eax;\n"
        "  end;\n"
        "end;\n"
        "begin\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AsmStatement"), nullptr);
}

// --- With Statement ---

TEST_P(PascalGrammarFixture, parses_with_statement) {
    const char *source =
        "program WithExample;\n"
        "var\n"
        "  p: TPoint;\n"
        "begin\n"
        "  with p do\n"
        "  begin\n"
        "    X := 10;\n"
        "    Y := 20;\n"
        "  end;\n"
        "end.\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WithStatement"), nullptr);
}

// --- Error Recovery ---

TEST_P(PascalGrammarFixture, recovers_from_syntax_errors) {
    const char *source =
        "program Recovery;\n"
        "begin\n"
        "  valid_call;\n"
        "  bad statement ??? !!! @@@;\n"
        "  another_valid_call;\n"
        "end.\n";
    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "CallStatement"), nullptr);
}

// --- Rejects Malformed ---

TEST_P(PascalGrammarFixture, rejects_invalid_syntax) {
    const char *source = "[[[@@@???";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

} // namespace

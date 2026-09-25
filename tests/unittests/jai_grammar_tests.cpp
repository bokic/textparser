#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <jai_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct JaiGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/jai_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &jai_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, JaiGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Basic Program ---

TEST_P(JaiGrammarFixture, parses_basic_program) {
    const char *source =
        "#import \"Basic\";\n"
        "\n"
        "/* A standard JAI block comment */\n"
        "main :: () {\n"
        "    // Print message\n"
        "    flag := true;\n"
        "    msg := \"Hello \\n \\\"World\\\"!\";\n"
        "    if flag {\n"
        "        x := 42 + 2;\n"
        "        arr: [2] int;\n"
        "        arr[0] = 0x2a;\n"
        "    }\n"
        "    defer {\n"
        "        print(\"Done!\\n\");\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ImportStatement"), nullptr);
    EXPECT_NE(find(node, "Declaration"), nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "DeferStatement"), nullptr);
}

// --- Variable and Constant Declarations ---

TEST_P(JaiGrammarFixture, parses_variable_and_constant_declarations) {
    const char *source =
        "x := 42;\n"
        "pi :: 3.14159;\n"
        "a, b := 1, 2;\n"
        "msg: string = \"hello\";\n"
        "count: int;\n"
        "typed_const: int : 100;\n"
        "arr: [4] int;\n"
        "d_arr: [..] float;\n"
        "slice: [] u8;\n"
        "ptr: *int;\n"
        "pptr: **int;\n"
        "uninit: int = ---;\n"
        "using obj: MyStruct;\n"
        "#as inner: BaseType;\n"
        "@Serialize id: int = 1;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Declaration"), nullptr);
}

// --- Procedure Declarations and Modifiers ---

TEST_P(JaiGrammarFixture, parses_procedures_and_modifiers) {
    const char *source =
        "add :: (a: int, b: int) -> int {\n"
        "    return a + b;\n"
        "}\n"
        "divide :: (a: float, b: float) -> (result: float, success: bool) {\n"
        "    if b == 0 return 0.0, false;\n"
        "    return a / b, true;\n"
        "}\n"
        "polymorphic :: (val: $T) -> T {\n"
        "    return val;\n"
        "}\n"
        "baked :: (x: $$T) {\n"
        "}\n"
        "varargs :: (fmt: string, args: ..Any) {\n"
        "}\n"
        "c_func :: (x: int) -> int #c_call #no_context {\n"
        "    return x * 2;\n"
        "}\n"
        "external :: () #foreign my_lib \"c_ext\";\n"
        "inlined :: (x: int) -> int #inline {\n"
        "    return x;\n"
        "}\n"
        "must_check :: () -> #must bool {\n"
        "    return true;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Declaration"), nullptr);
    EXPECT_NE(find(node, "ProcedureDefinition"), nullptr);
}

// --- Structs, Unions and Parameterized Types ---

TEST_P(JaiGrammarFixture, parses_structs_and_unions) {
    const char *source =
        "Vector3 :: struct {\n"
        "    x: float = 0.0;\n"
        "    y: float = 0.0;\n"
        "    z: float = 0.0;\n"
        "}\n"
        "Pair :: struct (First: Type, Second: Type) {\n"
        "    first: First;\n"
        "    second: Second;\n"
        "}\n"
        "TaggedUnion :: struct {\n"
        "    tag: int;\n"
        "    union {\n"
        "        as_int: int;\n"
        "        as_float: float;\n"
        "    };\n"
        "}\n"
        "PackedData :: struct #no_padding {\n"
        "    a: u8;\n"
        "    b: u32;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StructDefinition"), nullptr);
}

// --- Enums and Flags ---

TEST_P(JaiGrammarFixture, parses_enums_and_flags) {
    const char *source =
        "Direction :: enum {\n"
        "    NORTH;\n"
        "    SOUTH;\n"
        "    EAST;\n"
        "    WEST;\n"
        "}\n"
        "ByteEnum :: enum u8 #specified {\n"
        "    FIRST :: 1;\n"
        "    SECOND :: 2;\n"
        "}\n"
        "Permissions :: enum_flags {\n"
        "    READ;\n"
        "    WRITE;\n"
        "    EXECUTE;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "EnumDefinition"), nullptr);
}

// --- Control Flow ---

TEST_P(JaiGrammarFixture, parses_control_flow) {
    const char *source =
        "test_flow :: () {\n"
        "    if x > 10 {\n"
        "        print(\"greater\\n\");\n"
        "    } else if x == 10 {\n"
        "        print(\"equal\\n\");\n"
        "    } else {\n"
        "        print(\"less\\n\");\n"
        "    }\n"
        "    if x > 5 then print(\"yes\\n\");\n"
        "    #if DEBUG {\n"
        "        print(\"debug\\n\");\n"
        "    }\n"
        "    if x == {\n"
        "        case 1;\n"
        "            print(\"one\\n\");\n"
        "        case 2;\n"
        "            print(\"two\\n\");\n"
        "        case;\n"
        "            print(\"default\\n\");\n"
        "    }\n"
        "    while running {\n"
        "        update();\n"
        "        if done break;\n"
        "        if skip continue;\n"
        "    }\n"
        "    for items {\n"
        "        print(\"%\\n\", it);\n"
        "    }\n"
        "    for item, index: items {\n"
        "        print(\"[%] = %\\n\", index, item);\n"
        "    }\n"
        "    for < items {\n"
        "    }\n"
        "    for * items {\n"
        "    }\n"
        "    for 0..9 {\n"
        "    }\n"
        "    defer clean_up();\n"
        "    push_context my_context {\n"
        "        run();\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "IfCaseStatement"), nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
    EXPECT_NE(find(node, "DeferStatement"), nullptr);
    EXPECT_NE(find(node, "PushContextStatement"), nullptr);
}

// --- Directives ---

TEST_P(JaiGrammarFixture, parses_directives) {
    const char *source =
        "#import \"Basic\";\n"
        "#import,dir \"MyModule\";\n"
        "#load \"other_file.jai\";\n"
        "#run {\n"
        "    print(\"Compiling...\\n\");\n"
        "}\n"
        "#assert x > 0;\n"
        "#assert x > 0, \"x must be positive\";\n"
        "#scope_file\n"
        "file_scope_var := 1;\n"
        "#scope_export\n"
        "exported_var := 2;\n"
        "#scope_module\n"
        "module_var := 3;\n"
        "#placeholder external_sym;\n"
        "#module_parameters (DEBUG := true);\n"
        "#add_context custom_allocator: *void;\n"
        "#asm {\n"
        "    mov rax, rbx;\n"
        "    add rax, rcx;\n"
        "}\n"
        "c := #char \"Z\";\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ImportStatement"), nullptr);
    EXPECT_NE(find(node, "LoadStatement"), nullptr);
    EXPECT_NE(find(node, "RunStatement"), nullptr);
    EXPECT_NE(find(node, "AssertStatement"), nullptr);
    EXPECT_NE(find(node, "ScopeDirectiveStatement"), nullptr);
    EXPECT_NE(find(node, "PlaceholderStatement"), nullptr);
    EXPECT_NE(find(node, "AddContextStatement"), nullptr);
    EXPECT_NE(find(node, "AsmStatement"), nullptr);
}

// --- Here String ---

TEST_P(JaiGrammarFixture, parses_here_strings) {
    const char *source =
        "str := #string END\n"
        "This is a multiline\n"
        "here string in Jai.\n"
        "END;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Declaration"), nullptr);
}

// --- Expressions, Precedence and Casts ---

TEST_P(JaiGrammarFixture, parses_expressions_and_precedence) {
    const char *source =
        "test_exprs :: () {\n"
        "    a := 1 + 2 * 3 - 4 / 2;\n"
        "    b := (a << 2) | (a >> 1) ^ 0xff;\n"
        "    c := a > 0 && b <= 100 || !a;\n"
        "    r := 0..10;\n"
        "    ternary := ifx a > 0 then 1 else 2;\n"
        "    ternary2 := ifx a > 0 1 else 2;\n"
        "    f := cast(float) a;\n"
        "    f2 := cast,no_check(float) a;\n"
        "    f3 := cast(float, a);\n"
        "    auto := xx a;\n"
        "    deref := << ptr;\n"
        "    deref2 := ptr.*;\n"
        "    pcast := a.(float);\n"
        "    lit1 := Vector3.{1.0, 2.0, 3.0};\n"
        "    lit2 := Vector3.{x = 1.0, y = 2.0};\n"
        "    anon_lit := .{1, 2, 3};\n"
        "    arr_lit := int.[1, 2, 3];\n"
        "    anon_arr := .[1, 2, 3];\n"
        "    color := .RED;\n"
        "    lambda := (x: int) => x * 2;\n"
        "    print(\"done\\n\");\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ExpressionStatement"), nullptr);
    EXPECT_NE(find(node, "CastExpression"), nullptr);
    EXPECT_NE(find(node, "AutoCastExpression"), nullptr);
    EXPECT_NE(find(node, "IfxExpression"), nullptr);
}

// --- Operator Overloading ---

TEST_P(JaiGrammarFixture, parses_operator_overloads) {
    const char *source =
        "operator + :: (a: Vector3, b: Vector3) -> Vector3 {\n"
        "    return .{a.x + b.x, a.y + b.y, a.z + b.z};\n"
        "}\n"
        "operator == :: (a: Vector3, b: Vector3) -> bool {\n"
        "    return a.x == b.x && a.y == b.y && a.z == b.z;\n"
        "}\n"
        "operator []= :: (v: *Vector3, idx: int, val: float) {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Declaration"), nullptr);
    EXPECT_NE(find(node, "OperatorName"), nullptr);
}

// --- Corner Cases and Edge Conditions ---

TEST_P(JaiGrammarFixture, parses_empty_and_comments_only) {
    const char *sources[] = {
        "",
        "// single line comment\n",
        "/* standard block comment */",
        "/* nested /* block */ comment */\n",
        ";;;",
        "#scope_file\n",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(JaiGrammarFixture, parses_complex_declarations_and_types) {
    const char *source =
        "fn_type: (a: int, b: *string) -> (res: bool, err: string);\n"
        "poly_fn: (x: $T/interface) -> T;\n"
        "matrix: [4][4] float;\n"
        "dyn_matrix: [..][..] int;\n"
        "custom_type: #type,isa MyType;\n"
        "ctx: #Context;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Declaration"), nullptr);
}

TEST_P(JaiGrammarFixture, rejects_invalid_syntax) {
    const char *sources[] = {
        "x := ;;",
        "main :: ( { }",
        "if > 0 { }",
        "operator unknown :: () {}",
    };
    for (const char *source : sources) {
        parse_source(source, TEXTPARSER_MATCH_NO);
    }
}

// --- Diagnostic Recovery ---

TEST_P(JaiGrammarFixture, recovers_from_statement_errors) {
    const char *source =
        "main :: () {\n"
        "    good_one := 1;\n"
        "    bad_one := ;\n"
        "    good_two := 2;\n"
        "}\n";
    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "Declaration"), nullptr);
}

// --- Parity between JSON definition and Static header ---

TEST(JaiDefinitionParity, identical_tree_and_recovery_diagnostics) {
    textparser_language_definition *json = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                  "definitions/jai_definition.json", &json), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(json, nullptr);
    const char *source =
        "Vector3 :: struct { x: float = 0.0; }\n"
        "main :: () {\n"
        "    good := 1;\n"
        "    bad := ;\n"
        "    result := good + 42;\n"
        "}\n";
    std::string trees[2], diagnostics[2];
    const textparser_language_definition *definitions[] = { &jai_definition, json };
    for (int i = 0; i < 2; ++i) {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
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
            diagnostics[i] += std::string(d.code ? d.code : "") + ":" +
                (d.message ? d.message : "") + ":" +
                std::to_string(d.start_pos) + ":" + std::to_string(d.length);
        }
    }
    EXPECT_EQ(trees[0], trees[1]);
    EXPECT_EQ(diagnostics[0], diagnostics[1]);
    textparser_free_language_definition(json);
}

} // namespace

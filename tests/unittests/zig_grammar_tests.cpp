#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <zig_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct ZigGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/zig_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &zig_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, ZigGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Function declarations ---

TEST_P(ZigGrammarFixture, parses_simple_function) {
    const char *source =
        "fn hello() void {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_pub_function) {
    const char *source =
        "pub fn main() !void {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "ContainerDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_function_with_parameters) {
    const char *source =
        "fn add(a: i32, b: i32) i32 {\n"
        "    return a;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "ParamDeclList"), nullptr);
    EXPECT_NE(find(node, "PrimaryExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_extern_function) {
    const char *source =
        "extern fn printf(fmt: [*:0]const u8, ...) i32;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_export_function) {
    const char *source =
        "export fn add(a: i32, b: i32) i32 {\n"
        "    return a;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_inline_function) {
    const char *source =
        "inline fn square(x: i32) i32 {\n"
        "    return x;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_function_with_optional_return) {
    const char *source =
        "fn find(val: i32) ?i32 {\n"
        "    return null;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "TypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_function_with_noalias_param) {
    const char *source =
        "fn copy(noalias dst: []u8, noalias src: []const u8) void {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

// --- Variable and const declarations ---

TEST_P(ZigGrammarFixture, parses_const_declaration) {
    const char *source =
        "const std = @import(\"std\");\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
    EXPECT_NE(find(node, "PrimaryTypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_var_declaration) {
    const char *source =
        "var counter: u32 = 0;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_pub_const) {
    const char *source =
        "pub const version: []const u8 = \"1.0.0\";\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
    EXPECT_NE(find(node, "ContainerDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_threadlocal_var) {
    const char *source =
        "threadlocal var tls_value: u32 = 0;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
}

// --- Struct declarations ---

TEST_P(ZigGrammarFixture, parses_simple_struct) {
    const char *source =
        "const Point = struct {\n"
        "    x: f32,\n"
        "    y: f32,\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
    EXPECT_NE(find(node, "ContainerMembers"), nullptr);
    EXPECT_NE(find(node, "ContainerField"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_struct_with_methods) {
    const char *source =
        "const Vec2 = struct {\n"
        "    x: f32,\n"
        "    y: f32,\n"
        "    pub fn length(self: Vec2) f32 {\n"
        "        return self.x;\n"
        "    }\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_packed_struct) {
    const char *source =
        "const Flags = packed struct {\n"
        "    active: bool,\n"
        "    debug: bool,\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_struct_with_default_field) {
    const char *source =
        "const Config = struct {\n"
        "    timeout: u32 = 5000,\n"
        "    retries: u8 = 3,\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
}

// --- Enum declarations ---

TEST_P(ZigGrammarFixture, parses_simple_enum) {
    const char *source =
        "const Color = enum {\n"
        "    red,\n"
        "    green,\n"
        "    blue,\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
    EXPECT_NE(find(node, "ContainerMembers"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_enum_with_backing_type) {
    const char *source =
        "const Status = enum(u8) {\n"
        "    ok = 0,\n"
        "    err = 1,\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_enum_with_methods) {
    const char *source =
        "const Direction = enum {\n"
        "    north,\n"
        "    south,\n"
        "    pub fn opposite(self: Direction) Direction {\n"
        "        return self;\n"
        "    }\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
}

// --- Union declarations ---

TEST_P(ZigGrammarFixture, parses_tagged_union) {
    const char *source =
        "const Value = union(enum) {\n"
        "    int: i64,\n"
        "    float: f64,\n"
        "    bool: bool,\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
    EXPECT_NE(find(node, "ContainerMembers"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_simple_union) {
    const char *source =
        "const Data = union {\n"
        "    i: i32,\n"
        "    f: f32,\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
}

// --- Test declarations ---

TEST_P(ZigGrammarFixture, parses_test_declaration) {
    const char *source =
        "test \"basic addition\" {\n"
        "    const x: i32 = 1;\n"
        "    const y: i32 = 2;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TestDeclaration"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_unnamed_test) {
    const char *source =
        "test {\n"
        "    const x = 42;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TestDeclaration"), nullptr);
}

// --- Comptime ---

TEST_P(ZigGrammarFixture, parses_comptime_block) {
    const char *source =
        "comptime {\n"
        "    const x = 42;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ComptimeDeclaration"), nullptr);
}

// --- Control flow ---

TEST_P(ZigGrammarFixture, parses_if_else) {
    const char *source =
        "fn check(x: i32) void {\n"
        "    if (x > 0) {\n"
        "        return;\n"
        "    } else {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_if_with_optional_capture) {
    const char *source =
        "fn process(opt: ?i32) void {\n"
        "    if (opt) |val| {\n"
        "        return;\n"
        "    } else {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_while_loop) {
    const char *source =
        "fn count() void {\n"
        "    var i: u32 = 0;\n"
        "    while (i < 10) {\n"
        "        i += 1;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_while_with_continue_expr) {
    const char *source =
        "fn count() void {\n"
        "    var i: u32 = 0;\n"
        "    while (i < 10) : (i += 1) {\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
    EXPECT_NE(find(node, "WhileContinueExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_for_loop) {
    const char *source =
        "fn iterate(arr: []i32) void {\n"
        "    for (arr) |item| {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_for_with_index) {
    const char *source =
        "fn enumerate(arr: []i32) void {\n"
        "    for (arr, 0..) |item, i| {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_switch_expression) {
    const char *source =
        "fn classify(x: i32) void {\n"
        "    const label = switch (x) {\n"
        "        0 => \"zero\",\n"
        "        1...9 => \"small\",\n"
        "        else => \"other\",\n"
        "    };\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SwitchExpression"), nullptr);
    EXPECT_NE(find(node, "SwitchCase"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_defer_statement) {
    const char *source =
        "fn cleanup() void {\n"
        "    defer return;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "BlockStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_errdefer_with_capture) {
    const char *source =
        "fn open() !void {\n"
        "    errdefer |err| return;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "BlockStatement"), nullptr);
}

// --- Expressions ---

TEST_P(ZigGrammarFixture, parses_builtin_call) {
    const char *source =
        "const std = @import(\"std\");\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrimaryTypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_try_expression) {
    const char *source =
        "fn read() !void {\n"
        "    const x = try getValue();\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrefixExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_field_access) {
    const char *source =
        "fn use_stdout() void {\n"
        "    const out = std.io.getStdOut();\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SuffixOp"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_array_index) {
    const char *source =
        "fn get(arr: []i32) i32 {\n"
        "    return arr[0];\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SuffixOp"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_slice_expression) {
    const char *source =
        "fn sub(arr: []i32) []i32 {\n"
        "    return arr[1..3];\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SuffixOp"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_function_call) {
    const char *source =
        "fn caller() void {\n"
        "    const x = getValue();\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CallSuffix"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_optional_unwrap) {
    const char *source =
        "fn unwrap(opt: ?i32) i32 {\n"
        "    return opt.?;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SuffixExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_pointer_deref) {
    const char *source =
        "fn deref(ptr: *i32) i32 {\n"
        "    return ptr.*;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SuffixExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_orelse_expression) {
    const char *source =
        "fn fallback(opt: ?i32) i32 {\n"
        "    return opt orelse 0;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "BitwiseExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_catch_expression) {
    const char *source =
        "fn safe_read() i32 {\n"
        "    return read() catch 0;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "BitwiseOp"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_catch_with_capture) {
    const char *source =
        "fn safe_read() i32 {\n"
        "    return read() catch |err| 0;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "BitwiseOp"), nullptr);
}

// --- Literals ---

TEST_P(ZigGrammarFixture, parses_number_literals) {
    const char *source =
        "fn nums() void {\n"
        "    const a: u32 = 42;\n"
        "    const b: u32 = 0xFF;\n"
        "    const c: u32 = 0b1010;\n"
        "    const d: u32 = 0o77;\n"
        "    const e: f64 = 3.14;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrimaryTypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_string_literal) {
    const char *source =
        "const msg = \"Hello, World!\";\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_char_literal) {
    const char *source =
        "const ch: u8 = 'a';\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrimaryTypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_multiline_string) {
    const char *source =
        "const text =\n"
        "    \\\\Line 1\n"
        "    \\\\Line 2\n"
        ";\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_boolean_literals) {
    const char *source =
        "fn bools() void {\n"
        "    const t: bool = true;\n"
        "    const f: bool = false;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrimaryTypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_null_undefined) {
    const char *source =
        "fn nullable() void {\n"
        "    var p: ?i32 = null;\n"
        "    var u: i32 = undefined;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrimaryTypeExpr"), nullptr);
}

// --- Struct initialization ---

TEST_P(ZigGrammarFixture, parses_struct_init) {
    const char *source =
        "fn make_point() void {\n"
        "    const p = Point{ .x = 1.0, .y = 2.0 };\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InitExpression"), nullptr);
    EXPECT_NE(find(node, "StructInitField"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_anonymous_struct_literal) {
    const char *source =
        "fn anon() void {\n"
        "    const p = .{ .x = 1, .y = 2 };\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InitList"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_array_literal) {
    const char *source =
        "fn arr_lit() void {\n"
        "    var arr = [_]u32{ 1, 2, 3 };\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InitExpression"), nullptr);
}

// --- Type expressions ---

TEST_P(ZigGrammarFixture, parses_pointer_type) {
    const char *source =
        "fn ptr_fn(p: *i32) void {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrefixTypeOp"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_optional_type) {
    const char *source =
        "fn opt_fn(p: ?i32) void {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_slice_type) {
    const char *source =
        "fn slice_fn(s: []u8) void {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SliceTypeStart"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_array_type) {
    const char *source =
        "fn arr_fn(a: [10]u8) void {\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ArrayTypeStart"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_fn_pointer_type) {
    const char *source =
        "const Callback = fn (i32) void;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FnProtoType"), nullptr);
}

// --- Assignment ---

TEST_P(ZigGrammarFixture, parses_assignment_operators) {
    const char *source =
        "fn ops() void {\n"
        "    var x: i32 = 0;\n"
        "    x += 1;\n"
        "    x -= 1;\n"
        "    x *= 2;\n"
        "    x /= 2;\n"
        "    x %= 3;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AssignExpression"), nullptr);
    EXPECT_NE(find(node, "AssignExpression"), nullptr);
}

// --- Usingnamespace ---

TEST_P(ZigGrammarFixture, rejects_usingnamespace) {
    const char *source =
        "pub usingnamespace @import(\"std\");\n";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

// --- Full programs ---

TEST_P(ZigGrammarFixture, parses_hello_world) {
    const char *source =
        "const std = @import(\"std\");\n"
        "\n"
        "pub fn main() !void {\n"
        "    const stdout = std.io.getStdOut().writer();\n"
        "    try stdout.print(\"Hello, World!\\n\", .{});\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "PrefixExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_fibonacci) {
    const char *source =
        "pub fn fib(n: u64) u64 {\n"
        "    if (n < 2) return n;\n"
        "    return fib(n - 1) + fib(n - 2);\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "PrimaryExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_generic_struct_pattern) {
    const char *source =
        "const ArrayList = struct {\n"
        "    items: []u8,\n"
        "    len: usize,\n"
        "\n"
        "    pub fn init() ArrayList {\n"
        "        return ArrayList{ .items = undefined, .len = 0 };\n"
        "    }\n"
        "\n"
        "    pub fn deinit(self: ArrayList) void {\n"
        "        return;\n"
        "    }\n"
        "};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "InitExpression"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_empty_source) {
    auto *node = parse_source("");
    // Empty source is valid - all declarations are optional
    (void)node;
}

// --- Edge cases ---

TEST_P(ZigGrammarFixture, parses_break_with_value) {
    const char *source =
        "fn labeled_block() i32 {\n"
        "    const x = blk: {\n"
        "        break :blk 42;\n"
        "    };\n"
        "    return x;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "PrimaryExpr"), nullptr);
    EXPECT_NE(find(node, "LabeledTypeExpr"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_nested_blocks) {
    const char *source =
        "fn nested() void {\n"
        "    {\n"
        "        {\n"
        "            const x: i32 = 1;\n"
        "        }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "Block"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_opaque_type) {
    const char *source =
        "const Handle = opaque {};\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_while_else) {
    const char *source =
        "fn find_item(items: []i32) ?i32 {\n"
        "    var i: usize = 0;\n"
        "    while (i < 10) : (i += 1) {\n"
        "        if (items[i] == 42) break;\n"
        "    } else {\n"
        "        return null;\n"
        "    }\n"
        "    return items[i];\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WhileStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_for_else) {
    const char *source =
        "fn search(arr: []i32) void {\n"
        "    for (arr) |item| {\n"
        "        if (item == 0) break;\n"
        "    } else {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_switch_with_range) {
    const char *source =
        "fn range_switch(x: u8) void {\n"
        "    switch (x) {\n"
        "        0...9 => return,\n"
        "        10...99 => return,\n"
        "        else => return,\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SwitchExpression"), nullptr);
    EXPECT_NE(find(node, "SwitchItem"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_inline_for_loop) {
    const char *source =
        "fn unroll(arr: []i32) void {\n"
        "    inline for (arr) |item| {\n"
        "        return;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ForStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_comptime_in_function) {
    const char *source =
        "fn generic_fn() void {\n"
        "    comptime {\n"
        "        const check = 42;\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ExprStatement"), nullptr);
}

TEST_P(ZigGrammarFixture, parses_multiple_top_level_decls) {
    const char *source =
        "const MAX: u32 = 100;\n"
        "\n"
        "const Point = struct {\n"
        "    x: f32,\n"
        "    y: f32,\n"
        "};\n"
        "\n"
        "pub fn distance(a: Point, b: Point) f32 {\n"
        "    const dx = a.x - b.x;\n"
        "    const dy = a.y - b.y;\n"
        "    return dx;\n"
        "}\n"
        "\n"
        "test \"distance\" {\n"
        "    const p1 = Point{ .x = 0.0, .y = 0.0 };\n"
        "    const p2 = Point{ .x = 3.0, .y = 4.0 };\n"
        "    const d = distance(p1, p2);\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
    EXPECT_NE(find(node, "ContainerDecl"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "TestDeclaration"), nullptr);
}

// --- Recovery tests ---

TEST_P(ZigGrammarFixture, recovers_from_syntax_errors) {
    const char *source = "fn broken() void { const x = ; return; } fn after() void {}";
    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "PrimaryExpr"), nullptr);
    textparser_diagnostic diagnostic = {};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "ZIG1003");
    EXPECT_GT(diagnostic.length, 0u);
    EXPECT_LT(diagnostic.start_pos, std::strlen(source));
    const auto *declaration = find(node, "FunctionDeclaration");
    ASSERT_NE(declaration, nullptr);
    EXPECT_GT(node->source_end, declaration->source_end);
}

// Zig 0.16.0 syntax and lexical edge cases, checked with the installed compiler.

TEST_P(ZigGrammarFixture, parses_declaration_attributes) {
    const char *sources[] = {
        "extern \"c\" fn puts([*:0]const u8) callconv(.c) c_int;",
        "export fn entry() align(16) linksection(\".text\") callconv(.c) void {}",
        "extern \"c\" threadlocal var counter: u32;",
        "var memory: [64]u8 align(16) addrspace(.generic) linksection(\".data\") = undefined;",
        "const F = fn (comptime T: type, noalias p: *T, anytype, ...) !void;",
        "test example {} fn example() void {}",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(ZigGrammarFixture, parses_containers_and_root_fields) {
    const char *sources[] = {
        "//! Root doc\n//! Second doc\n/// A field\na: u32, b: u8 = 0, pub fn f() void {}",
        "const S = packed struct(u16) { a: u3, b: u13, };",
        "const S = extern struct { a: u32 align(4), b: u8, };",
        "const T = struct { u32, []const u8, };",
        "const E = enum(u8) { a = 1, _, };",
        "const U = union(enum(u8)) { a: u32 = 1, b: void = 2, };",
        "const E = error {\n /// one\n A,\n /// two\n B };",
        "const O = opaque { pub fn f() void {} };",
        "/// first\n/// second\nfn f(\n /// param\n x: u32) void {}",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(ZigGrammarFixture, parses_pointer_and_array_qualifiers) {
    const char *sources[] = {
        "const P = *allowzero align(8:0:32) addrspace(.generic) const volatile u32;",
        "const P = [*:0]allowzero align(8) addrspace(.generic) const volatile u8;",
        "const P = [*c]const u8; const c = 1;",
        "const P = [:0]const u8; const A = [4:0]u8;",
        "const P = **const u8; const O = ?*const u8;",
        "const E = error{Oops}![]const u8;",
        "const A = [_:0]u8{1, 2, 3};",
        "const T = @TypeOf(value).Child;",
        "const T = if (flag) u8 else u16;",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(ZigGrammarFixture, parses_control_flow_edges) {
    const char *sources[] = {
        "fn f() void { defer cleanup(); errdefer |err| { log(err); } }",
        "fn f() void { if (v) |*p| p.* = 2 else |err| log(err); }",
        "fn f() void { outer: inline while (next()) |*v| : (i += 1) { continue :outer; } else |err| log(err); }",
        "fn f() void { inline for (a, 0.., b[0..3]) |*x, i, y,| { use(x, i, y); } else done(); }",
        "fn f() void { const x = for (a) |v| { if (v) break v; } else 0; }",
        "fn f() void { const x = while (next()) |v| { break v; } else |err| return err; }",
        "fn f() void { const x = blk: { break :blk 42; }; }",
        "fn f() void { again: switch (x) { 0 => continue :again 1, inline 1, 2, => |v, tag| use(v, tag), else => {}, } }",
        "fn f() void { const x = optional orelse return; const y = failed() catch |err| return err; }",
        "fn f() void { comptime var x = 1; comptime x += 1; nosuspend call(); suspend {} resume frame; }",
        "fn f() void { var a: u32, const b, c = tuple; a, c = other; }",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(ZigGrammarFixture, parses_assembly) {
    const char *sources[] = {
        "fn f() void { asm volatile (\"nop\"); }",
        "fn f() usize { return asm (\"\" : [out] \"=r\" (-> usize) : [in] \"r\" (value) : .{ .memory = true }); }",
        "fn f() void { asm volatile (\"\" : : [in] \"r\" (value) : .{ .memory = true }); }",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(ZigGrammarFixture, parses_operators_and_suffixes) {
    const char *sources[] = {
        "const x = a +% b -% c *% d +| e -| f *| g <<| h;",
        "const x = a || b; const y = a ++ b ** 2;",
        "fn f() void { x +%= 1; x -%= 1; x *%= 2; x +|= 1; x -|= 1; x *|= 2; x <<|= 1; x <<= 1; x >>= 1; x &= 1; x |= 1; x ^= 1; }",
        "const x = -%a; const y = &value; const z = try call();",
        "const x = call(a, b,).field[0..len :0].ptr.*.?;",
        "const x = .{1, true, \"hi\"}; const y = T{ .x = 1, .y = 2, };",
        "const x = (a < b) == (c > d);",
        "const x = a . ?;",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(ZigGrammarFixture, parses_lexical_edges) {
    const char *sources[] = {
        "const @\"hello world\" = @\"not a keyword\";",
        "const a = '\\n'; const b = '\\x41'; const c = '\\u{1f600}'; const d = 'é';",
        "const a = \"\\n\\t\\r\\\\\\\"\\x41\\u{1f600}\";",
        "const a = 0x1.fp+1_0; const b = 1.25e+1_0; const c = 0b1_0; const d = 0o7_0;",
        "const s = \\\\first\n    \\\\second\n;",
        "const usingnamespace = 1; const await = 2; const async = 3;",
        "//// ordinary comment\nconst x = 1; // end\n",
    };
    for (const char *source : sources) {
        parse_source(source);
    }
}

TEST_P(ZigGrammarFixture, rejects_invalid_syntax) {
    const char *sources[] = {
        "const x = +1;",
        "const x = a < b < c;",
        "const x = a == b != c;",
        "const x = a orelse |v| b;",
        "const x = try;",
        "const x = a +;",
        "const x = fn () {};",
        "fn f() {}",
        "fn f() void { const x; }",
        "fn f() void { for (xs) {} }",
        "fn f() void { for () |x| {} }",
        "fn f() void { defer const x = 1; }",
        "const x = [*wrong]u8;",
        "const x = [0...3]u8;",
        "const x = arr[..3];",
        "const x = a . *;",
        "const x = @import;",
        "const x = async f();",
        "const x = await f();",
        "usingnamespace other;",
        "const x = 1; $",
        "const x = 1",
        "const x = \"unterminated;",
        "const x = /* comment */ 1;",
        "fn f() void { x = y = z; }",
        "const S = struct { x: u8; };",
        "fn f() void { ; }",
        "fn f() void { return;; }",
    };
    for (const char *source : sources) {
        parse_source(source, TEXTPARSER_MATCH_NO);
    }
}

TEST_P(ZigGrammarFixture, rejects_invalid_literals) {
    const char *sources[] = {
        "const x = 0XFF;",
        "const x = 0b102;",
        "const x = 1__2;",
        "const x = 1_;",
        "const x = 1e;",
        "const x = 0x;",
        "const x = '\\q';",
        "const x = 'ab';",
        "const x = \"\\q\";",
    };
    for (const char *source : sources) {
        parse_source(source, TEXTPARSER_MATCH_NO);
    }
}

TEST_P(ZigGrammarFixture, arithmetic_precedence_and_associativity) {
    auto *node = parse_source("const x = a + b * c << d - e - f;");
    const auto *shift = find(node, "LShift");
    ASSERT_NE(shift, nullptr);
    ASSERT_NE(shift->child, nullptr);
    ASSERT_NE(shift->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), shift->child), "Plus");
    const auto *multiply = find(shift->child, "Star");
    ASSERT_NE(multiply, nullptr);
    const auto *subtract = shift->child->next;
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), subtract), "Minus");
    ASSERT_NE(subtract->child, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), subtract->child), "Minus");
}

TEST_P(ZigGrammarFixture, catch_and_bitwise_share_precedence) {
    const char *source = "const x = a & b ^ c | d catch |err| e + f orelse g;";
    auto *node = parse_source(source);
    const auto *bitwise = find(node, "BitwiseExpr");
    ASSERT_NE(bitwise, nullptr);
    // A single flat left-fold layer owns &, ^, |, catch, and orelse.
    ASSERT_NE(bitwise->child, nullptr);
    const auto *tail = bitwise->child->next;
    ASSERT_NE(tail, nullptr);
    int count = 0;
    for (auto *op = tail->child; op; op = op->next) ++count;
    EXPECT_EQ(count, 5);
    EXPECT_NE(find(bitwise, "Payload"), nullptr);
    EXPECT_NE(find(bitwise, "Plus"), nullptr);
}

TEST_P(ZigGrammarFixture, comments_and_keyword_boundaries) {
    const char *sources[] = {
        "", "// ordinary comment", "//! module doc\n//! more\n",
        "const inline_value = 1; const iffy = 2; const c = 3;",
        "const text = \"// not a comment\"; // real comment\n",
    };
    for (const char *source : sources) parse_source(source);
}

TEST(ZigDefinitionParity, identical_tree_and_recovery_diagnostics) {
    textparser_language_definition *json = nullptr;
    ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                  "definitions/zig_definition.json", &json), TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(json, nullptr);
    const char *source =
        "const S = packed struct(u8) { a: u8, }; "
        "fn f() void { const bad = ; return a + b * c; } fn next() void {}";
    std::string trees[2], diagnostics[2];
    const textparser_language_definition *definitions[] = { &zig_definition, json };
    for (int i = 0; i < 2; ++i) {
        textparser::Parser parser;
        ASSERT_EQ(parser.openmem(source, std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
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
            diagnostics[i] += std::string(d.code) + ":" + d.message + ":" +
                std::to_string(d.start_pos) + ":" + std::to_string(d.length);
        }
    }
    EXPECT_EQ(trees[0], trees[1]);
    EXPECT_EQ(diagnostics[0], diagnostics[1]);
    textparser_free_language_definition(json);
}

TEST_P(ZigGrammarFixture, parses_documentation_positions) {
    const char *sources[] = {
        "/// root field\nfield: u32,",
        "  /// declaration\nconst x = 1;",
        "//! container\n/// declaration\nconst x = 1;",
    };
    for (const char *source : sources) parse_source(source);
}

TEST_P(ZigGrammarFixture, rejects_invalid_documentation_positions) {
    const char *sources[] = {
        "const x = 1; /// invalid\nconst y = 2;",
        "const E = error { /// same line\nA };",
        "fn f(/// same line\nx: u8) void {}",
        "/// orphan",
    };
    for (const char *source : sources) parse_source(source, TEXTPARSER_MATCH_NO);
}

} // namespace

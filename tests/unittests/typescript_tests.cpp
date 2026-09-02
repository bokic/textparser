#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <cstring>

namespace {

struct TypeScriptExpressionFixture : testing::Test {
    textparser_language_definition *definition = nullptr;

    void SetUp() override {
        ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                      "definitions/typescript_definition.json", &definition),
                  TEXTPARSER_JSON_NO_ERROR);
        ASSERT_NE(definition, nullptr);
        ASSERT_NE(definition->grammar, nullptr);
        ASSERT_STREQ(definition->initial_lexer_mode, "default");
    }

    void TearDown() override { textparser_free_language_definition(definition); }

    int token(const char *name) const {
        for (int i = 0; definition->tokens[i].name != nullptr; i++)
            if (std::strcmp(definition->tokens[i].name, name) == 0) return i;
        return -1;
    }

    int production(const char *name) const {
        for (size_t i = 0; i < definition->grammar->production_count; i++)
            if (std::strcmp(definition->grammar->productions[i].name, name) == 0)
                return definition->grammar->productions[i].id;
        return -1;
    }

    textparser_node *parse_expression(const char *source,
                                      textparser_match_status expected = TEXTPARSER_MATCH_OK,
                                      const char *filename = nullptr) {
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        if (filename != nullptr) textparser_set_filename(parser.get(), filename);
        EXPECT_EQ(parser.parse(definition), 0)
            << (textparser_parse_error(parser.get()) ? textparser_parse_error(parser.get()) : "")
            << " at " << textparser_parse_error_position(parser.get());
        for (size_t i = 0; i < definition->operator_definition_count; i++)
            EXPECT_EQ(textparser_register_operator(parser.get(), &definition->operator_definitions[i]), 0);
        result = {};
        EXPECT_EQ(parser.execute_production(definition->grammar->productions,
                                            definition->grammar->production_count,
                                            production("Expression"), &result), 0);
        EXPECT_EQ(result.status, expected) << source;
        return result.node;
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK,
                                  const char *filename = nullptr) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        if (filename != nullptr) textparser_set_filename(parser.get(), filename);
        EXPECT_EQ(parser.parse(definition), 0)
            << (textparser_parse_error(parser.get()) ? textparser_parse_error(parser.get()) : "")
            << " at " << textparser_parse_error_position(parser.get());
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(definition, &result), 0);
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

    textparser_node *parse_type(const char *source,
                                textparser_match_status expected = TEXTPARSER_MATCH_OK) {
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(definition), 0);
        textparser_set_lexical_goal(parser.get(), "Type");
        result = {};
        EXPECT_EQ(parser.execute_production(definition->grammar->productions,
                                            definition->grammar->production_count,
                                            production("Type"), &result), 0);
        textparser_parser_state_view state{};
        EXPECT_EQ(parser.parser_state(&state), 0);
        if (result.status == TEXTPARSER_MATCH_OK && state.source_offset != std::strlen(source)) {
            result = {};
            result.status = TEXTPARSER_MATCH_NO;
        }
        EXPECT_EQ(result.status, expected) << source;
        if (result.status == TEXTPARSER_MATCH_OK) {
            EXPECT_EQ(parser.parser_state(&state), 0);
            EXPECT_EQ(state.source_offset, std::strlen(source)) << source;
        }
        return result.node;
    }

    textparser::Parser parser;
    textparser_match_result result{};
};

size_t count_nodes_with_flag(const textparser_node *node, uint32_t flag) {
    size_t count = 0;
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if ((item->node_flags & flag) != 0) count++;
        count += count_nodes_with_flag(item->child, flag);
    }
    return count;
}

const textparser_node *find_cst_kind(textparser_t parser, const textparser_node *node,
                                    const char *kind, uint32_t required_flags = 0) {
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        textparser_cst_node_view view{};
        if (textparser_get_cst_node_view(parser, item, &view) == 0 && view.kind != nullptr &&
            std::strcmp(view.kind, kind) == 0 && (view.flags & required_flags) == required_flags)
            return item;
        if (const textparser_node *nested =
                find_cst_kind(parser, item->child, kind, required_flags)) return nested;
    }
    return nullptr;
}

} // namespace

TEST_F(TypeScriptExpressionFixture, loads_schema_v2_expression_profile) {
    EXPECT_GT(definition->lexer_mode_count, 0u);
    EXPECT_EQ(definition->lexer_goal_count, 8u);
    EXPECT_GT(definition->operator_definition_count, 30u);
    EXPECT_STREQ(definition->grammar->productions[definition->grammar->start_production].name,
                 "SourceFile");
}

TEST_F(TypeScriptExpressionFixture, exposes_stable_typescript_cst_kinds_and_exact_spans) {
    const char *source = "  const value: number = 1 + 2";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);

    textparser_cst_node_view source_view{};
    ASSERT_EQ(textparser_get_cst_node_view(parser.get(), root, &source_view), 0);
    EXPECT_STREQ(source_view.kind, "SourceFile");
    EXPECT_EQ(source_view.start, 0u);
    EXPECT_EQ(source_view.end, std::strlen(source));
    EXPECT_FALSE(source_view.terminal);
    EXPECT_EQ(textparser_typescript_cst_category_of(parser.get(), root),
              TEXTPARSER_TS_CST_SOURCE_FILE);

    const textparser_node *declaration =
        find_cst_kind(parser.get(), root, "VariableStatement");
    ASSERT_NE(declaration, nullptr);
    EXPECT_EQ(textparser_typescript_cst_category_of(parser.get(), declaration),
              TEXTPARSER_TS_CST_DECLARATION);

    const textparser_node *type = find_cst_kind(parser.get(), root, "TypeAnnotation");
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(textparser_typescript_cst_category_of(parser.get(), type), TEXTPARSER_TS_CST_TYPE);

    const textparser_node *expression = find_cst_kind(parser.get(), root, "Plus");
    ASSERT_NE(expression, nullptr);
    EXPECT_EQ(textparser_typescript_cst_category_of(parser.get(), expression),
              TEXTPARSER_TS_CST_EXPRESSION);

    const textparser_node *identifier = find_cst_kind(parser.get(), root, "Identifier");
    ASSERT_NE(identifier, nullptr);
    textparser_cst_node_view identifier_view{};
    ASSERT_EQ(textparser_get_cst_node_view(parser.get(), identifier, &identifier_view), 0);
    EXPECT_EQ(identifier_view.start, 8u);
    EXPECT_EQ(identifier_view.end, 13u);
    EXPECT_TRUE(identifier_view.terminal);
    EXPECT_EQ(textparser_typescript_cst_category_of(parser.get(), identifier),
              TEXTPARSER_TS_CST_TOKEN);

    const textparser_node *missing = find_cst_kind(
        parser.get(), root, "Semicolon", TEXTPARSER_NODE_MISSING);
    ASSERT_NE(missing, nullptr);
    textparser_cst_node_view missing_view{};
    ASSERT_EQ(textparser_get_cst_node_view(parser.get(), missing, &missing_view), 0);
    EXPECT_EQ(missing_view.start, std::strlen(source));
    EXPECT_EQ(missing_view.end, std::strlen(source));
    EXPECT_FALSE(missing_view.terminal);
}

TEST_F(TypeScriptExpressionFixture, parses_control_flow_and_iteration_statements) {
    const char *source =
        "if (ready) { value = 1; } else value = 2;\n"
        "while (ready) value--;\n"
        "do value++; while (ready)\n"
        "for (;;);\n"
        "for (item in items) break;\n"
        "for (item of items) continue;";
    ASSERT_NE(parse_source(source), nullptr);
    ASSERT_NE(parse_source("if (a) if (b) value = 1; else value = 2;"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_switch_try_with_labels_and_debugger) {
    const char *source =
        "outer: while (ready) {\n"
        "  switch (value) { case 1: break outer; default: debugger; }\n"
        "}\n"
        "try { value = 1; } catch (error) { value = 2; } finally { value = 3; }\n"
        "with (context) value = 4;";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, inserts_semicolons_at_line_brace_and_eof_boundaries) {
    textparser_node *root = parse_source(
        "value = 1\nother = 2\nif (ready) { result = 3\n}\nreturn");
    ASSERT_NE(root, nullptr);
    EXPECT_GE(count_nodes_with_flag(root, TEXTPARSER_NODE_MISSING), 4u);
    ASSERT_NE(parse_source("if (ready) { result = 3 }"), nullptr);
    EXPECT_EQ(parse_source("first second", TEXTPARSER_MATCH_NO), nullptr);
}

TEST_F(TypeScriptExpressionFixture, honors_restricted_line_terminator_productions) {
    ASSERT_NE(parse_source("return\nvalue\nbreak\nlabel: debugger"), nullptr);
    EXPECT_EQ(parse_source("throw\nerror", TEXTPARSER_MATCH_ERROR), nullptr);
    ASSERT_NE(parse_source("throw error\n"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, line_terminator_prevents_postfix_increment) {
    ASSERT_NE(parse_source("value\n++other"), nullptr);
    textparser_node *same_line = parse_source("value++;\nother--; ");
    ASSERT_NE(same_line, nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_variable_and_function_declarations) {
    const char *source =
        "let count: number = 1, name: string = 'text'\n"
        "const nested: Promise<Map<string, number[]>> = value;\n"
        "var enabled = true;\n"
        "function identity<T extends object>(value: T): T { return value; }\n"
        "declare function parse<T>(input: string): Promise<T>;\n"
        "async function work(value?: number): void {}";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, restores_expression_goal_after_type_annotations) {
    ASSERT_NE(parse_source("const value: Box<Box<number>> = left >> right;"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_classes_and_typed_members) {
    const char *source =
        "abstract class Base<T> { abstract value: T; }\n"
        "class Box<T> extends Base<T> implements Readable<T>, Writable<T> {\n"
        "  #value: T;\n"
        "  constructor(public initial: T) { value = initial; }\n"
        "  get current(): T { return value; }\n"
        "  set current(next: T) { value = next; }\n"
        "  static { ready = true; }\n"
        "}";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_interfaces_aliases_enums_and_namespaces) {
    const char *source =
        "interface Pair<T> extends Base<T> { readonly first: T; map<U>(value: U): Pair<U>; }\n"
        "type Result<T> = T extends string ? Promise<T> : never;\n"
        "const enum State { Ready = 1, Done, 'waiting' }\n"
        "namespace Models { type Id = number; interface User { id: Id } }\n"
        "declare module 'pkg' { interface Entry {} }";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_using_and_await_using_declarations) {
    const char *source =
        "using resource: Disposable = acquire(), other = acquire();\n"
        "await using asyncResource = acquireAsync();\n"
        "using { handle, close: dispose } = acquirePair();\n"
        "for (using item of resources) { item.run(); }\n"
        "for (await using item of asyncResources) { item.run(); }";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_modern_class_declaration_edges) {
    const char *source =
        "@sealed export class Service<const T, in K, out V> {\n"
        "  constructor(value: string);\n"
        "  constructor(public value: string) {}\n"
        "  @inject static accessor current: T;\n"
        "  @memo() [methodKey](@token input: K): V { return value; }\n"
        "  declare readonly optional?: V;\n"
        "}\n"
        "interface Producer<out T> { readonly [valueKey]: T; [methodKey](): T; }";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_class_expressions_and_decorated_default_exports) {
    ASSERT_NE(parse_source(
        "const Factory = @sealed class<T> { accessor value: T; };"), nullptr);
    ASSERT_NE(parse_source(
        "@sealed export default class { @field accessor value: number; }"), nullptr);
    ASSERT_NE(parse_source("export default @sealed class Named {}"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, rejects_incomplete_modern_declarations) {
    for (const char *source : {
             "using resource;",
             "await\nusing resource = acquire();",
             "@ class Missing {}",
             "class Invalid { @decorator static {} }",
         }) {
        EXPECT_EQ(parse_source(source, TEXTPARSER_MATCH_NO), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, declarations_work_in_for_headers_and_reject_incomplete_forms) {
    ASSERT_NE(parse_source(
        "for (let index: number = 0; index < 3; index++) ;\n"
        "for (const item: Item of items) ;"), nullptr);
    for (const char *source : {
             "const ;", "function missing(", "class Missing { value: string",
         }) {
        textparser_match_status expected = std::strcmp(source, "function missing(") == 0
            ? TEXTPARSER_MATCH_ERROR : TEXTPARSER_MATCH_NO;
        EXPECT_EQ(parse_source(source, expected), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_static_and_type_only_imports) {
    const char *source =
        "import 'polyfill';\n"
        "import defaultValue from 'default';\n"
        "import defaultValue, * as helpers from 'helpers';\n"
        "import { value, original as alias, type Model, type as typeName } from 'named';\n"
        "import type DefaultModel from 'types';\n"
        "import type { Input, Output as Result } from 'types';\n"
        "import type from 'default-named-type';\n"
        "import type, { Model } from 'default-and-named';\n"
        "import { type, default as defaultValue, 'arbitrary-name' as arbitrary } from 'names';";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_import_aliases_and_attributes) {
    const char *source =
        "import legacy = require('legacy');\n"
        "import Alias = Namespace.Member;\n"
        "import defer * as deferred from 'feature';\n"
        "import data from 'data.json' with { type: 'json' };\n"
        "import config from 'config.json' assert { type: 'json', mode: 'strict' };";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_export_forms) {
    const char *source =
        "export const value: number = 1;\n"
        "export function identity<T>(input: T): T { return input; }\n"
        "export default function () {}\n"
        "export default class { value: number; }\n"
        "export default interface NamedDefault<T> { value: T }\n"
        "export default interface { anonymous: true }\n"
        "export declare const ambientValue: string;\n"
        "export import Legacy = require('legacy');\n"
        "export { value, type, type Model as PublicModel, type as typeName, default as 'default-value' };\n"
        "export type { Input, Output as Result } from 'types';\n"
        "export * from 'all';\n"
        "export type * as Types from 'types';\n"
        "export = value;\n"
        "export as namespace Library;";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_nested_namespaces_ambient_modules_and_global_augmentation) {
    const char *source =
        "namespace Company.Models { export interface User { id: number } }\n"
        "module Internal { export type Id = number; export import Alias = External.Value; }\n"
        "declare module 'package' { export interface Options { ready: boolean } }\n"
        "declare global { interface Window { application: string } }";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_dynamic_import_and_import_meta_primaries) {
    for (const char *source : {
             "import('feature')", "import(moduleName)", "import('data.json', options)",
             "import('feature',)", "import.meta",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
    for (const char *source : {"import()", "import.module", "import('feature'"}) {
        EXPECT_EQ(parse_expression(source, TEXTPARSER_MATCH_NO), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_call_member_and_index_expression_chains) {
    for (const char *source : {
             "service.getValue(input)",
             "values[index + 1]",
             "factory()(first, ...rest).result",
             "instance.#field",
             "service.get() + other[index] * 2",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_optional_chains_and_non_null_assertions) {
    for (const char *source : {
             "user?.profile?.name", "callback?.(value)", "collection?.[index]",
             "value!.property", "lookup(key)!.result",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_new_expression_precedence_and_chains) {
    for (const char *source : {
             "new Client(options)", "new Namespace.Client().value",
             "new new Factory()()", "new (factory())(value)",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, incomplete_postfix_expressions_roll_back) {
    for (const char *source : {
             "service.", "service[", "service(", "user?.", "callback?.(", "new",
         }) {
        textparser_match_status expected = std::strcmp(source, "new") == 0
            ? TEXTPARSER_MATCH_NO : TEXTPARSER_MATCH_ERROR;
        EXPECT_EQ(parse_expression(source, expected), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_array_literals_holes_and_spreads) {
    for (const char *source : {
             "[]", "[,,,]", "[first, second,]", "[, first, , second]",
             "[...values]", "[head, ...tail,]", "[[one], { value: two }]",
             "[first, second].map(callback)",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_object_literal_property_forms) {
    for (const char *source : {
             "{}", "{ value }", "{ value: 1, 'label': text, 2: item }",
             "{ [key + suffix]: value }", "{ ...defaults, value, trailing: true, }",
             "{ nested: { items: [first, ...rest] } }",
             "{ value: 1 }.value",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_object_methods_generators_and_accessors) {
    for (const char *source : {
             "{ run(value: number): string { return value; } }",
             "{ *values() { return item; } }", "{ async load() { return result; } }",
             "{ async *stream(input) { return input; } }",
             "{ get value(): number { return current; }, set value(next: number) { current = next; } }",
             "{ get: 1, set, async: callback }",
             "{ [methodName](arg) { return arg; } }",
             "{ convert<T>(value: T): T { return value; } }",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, rejects_incomplete_literal_expressions) {
    for (const char *source : {
             "[", "[value", "[... ]", "{", "{ value:", "{ ... }",
             "{ method( }", "{ get value( }", "{ set value() {} }", "{ [key: value }",
             "{ first,, second }",
         }) {
        EXPECT_EQ(parse_expression(source, TEXTPARSER_MATCH_ERROR), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_function_expressions) {
    for (const char *source : {
             "function (value: number): number { return value; }",
             "function named<T>(value: T): T { return value; }",
             "function *values() { return item; }",
             "async function load(input) { return await input; }",
             "async function *stream<T>(input: T) { return input; }",
             "(function factory() {})()",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_arrow_function_heads_and_bodies) {
    for (const char *source : {
             "value => value + 1", "async => async", "(left, right) => left + right",
             "(value: number): number => value", "<T>(value: T): T => value",
             "async value => await value", "async (value: number) => value",
             "async <T>(value: T): T => value", "() => ({ ready: true })",
             "(...values) => values[0]", "callback => { return callback(); }",
             "(value => value)(input)",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, arrow_speculation_preserves_parenthesized_and_jsx_expressions) {
    for (const char *source : {
             "(value)", "(left + right)", "(factory())", "<Component value={item} />",
         }) {
        ASSERT_NE(parse_expression(source, TEXTPARSER_MATCH_OK,
                      source[0] == '<' ? "component.tsx" : nullptr), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, rejects_incomplete_function_and_arrow_expressions) {
    for (const char *source : {
             "function ()", "function *()", "value =>", "() =>", "async value =>",
             "(value: number) =>", "<T>(value: T) =>",
         }) {
        EXPECT_EQ(parse_expression(source, TEXTPARSER_MATCH_ERROR), nullptr) << source;
    }
    for (const char *source : {
             "consume(async\nvalue => value)",
             "consume((value)\n=> value)",
         }) {
        EXPECT_EQ(parse_expression(source, TEXTPARSER_MATCH_ERROR), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_nested_binding_patterns_in_declarations) {
    const char *source =
        "const { id, name: displayName = 'unknown', nested: { value }, [key]: computed, ...rest } = record;\n"
        "let [first, , third = 3, ...tail]: Items = values;\n"
        "var { 0: initial, type: kind, child: [head, ...remaining] } = source;";
    ASSERT_NE(parse_source(source), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_binding_patterns_in_parameters_arrows_catch_and_loops) {
    for (const char *source : {
             "function configure({ host = 'localhost', port }: Options, [first, ...remaining]: Items = items) {}",
             "const select = ({ value: result }, [index]) => result;",
             "for (const [key, value] of entries) {}",
             "for (let { id } in records) {}",
             "try { run(); } catch ({ message, ...details }: unknown) { report(message); }",
         }) {
        ASSERT_NE(parse_source(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, binding_patterns_cover_elisions_defaults_and_empty_forms) {
    for (const char *source : {
             "const [] = values;", "const {} = value;", "const [,,,] = values;",
             "const [value,] = values;", "const { value, } = record;",
             "function read([, value = fallback,], { key: alias = defaultValue }) {}",
             "const nested = ([{ value }, ...rest]) => value;",
         }) {
        ASSERT_NE(parse_source(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, rejects_malformed_binding_patterns) {
    for (const char *source : {
             "const [value = source;", "const { value = source;",
             "const [...rest, value] = source;", "const {...rest, value} = source;",
             "const { key: } = source;", "const [value,, = fallback] = source;",
             "function invalid({ value,) {}", "try {} catch ([value) {}",
             "const { value };", "function invalid(...rest, value) {}",
             "function invalid(...rest = values) {}",
         }) {
        textparser_match_status expected = std::strncmp(source, "function", 8) == 0
            ? TEXTPARSER_MATCH_ERROR : TEXTPARSER_MATCH_NO;
        EXPECT_EQ(parse_source(source, expected), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, validates_simple_member_and_destructuring_assignment_targets) {
    for (const char *source : {
             "value = next", "object.property += amount", "items[index] = value",
             "factory().result = value", "(value) = next", "value! = next",
             "[first, object.value, , fallback = defaultValue, ...rest] = values",
             "({ value, renamed: target, nested: [head, ...tail], [key]: computed, ...remaining } = source)",
             "++counter", "object.value--",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, rejects_non_assignable_and_malformed_assignment_targets) {
    for (const char *source : {
             "1 = value", "'text' += value", "factory() = value",
             "object?.property = value", "object?.property.nested = value",
             "[factory()] = values", "[first] += values", "[...rest, last] = values",
             "[...rest,] = values", "({ value: 1 } = source)",
             "({ method() {} } = source)", "({ ...rest, value } = source)",
             "({ ...rest, } = source)", "++1", "factory()++", "object?.value++",
         }) {
        EXPECT_EQ(parse_expression(source, TEXTPARSER_MATCH_ERROR), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, reports_typescript_syntax_codes_and_exact_spans) {
    struct Case {
        const char *source;
        const char *code;
        size_t start;
        size_t length;
    };
    for (const Case &item : {
             Case{"1 = value", "TS2364", 0, 1},
             Case{"object?.value = next", "TS2779", 0, 13},
             Case{"factory()++", "TS2357", 0, 9},
             Case{"service(", "TS1005", 8, 0},
             Case{"service(, value)", "TS1005", 8, 1},
         }) {
        EXPECT_EQ(parse_expression(item.source, TEXTPARSER_MATCH_ERROR), nullptr) << item.source;
        ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u) << item.source;
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, item.code) << item.source;
        EXPECT_EQ(diagnostic.severity, TEXTPARSER_SEVERITY_ERROR) << item.source;
        EXPECT_EQ(diagnostic.start_pos, item.start) << item.source;
        EXPECT_EQ(diagnostic.length, item.length) << item.source;
    }
}

TEST_F(TypeScriptExpressionFixture, reports_furthest_source_error_instead_of_abandoned_branch) {
    EXPECT_EQ(parse_source("const value = ;", TEXTPARSER_MATCH_NO), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TS1109");
    EXPECT_STREQ(diagnostic.message, "Expression expected.");
    EXPECT_EQ(diagnostic.start_pos, 14u);
    EXPECT_EQ(diagnostic.length, 1u);

    EXPECT_EQ(parse_source("function missing(", TEXTPARSER_MATCH_ERROR), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TS1005");
    EXPECT_STREQ(diagnostic.message, "')' expected.");
    EXPECT_EQ(diagnostic.start_pos, 17u);
    EXPECT_EQ(diagnostic.length, 0u);

    EXPECT_EQ(parse_source("const value =\n;", TEXTPARSER_MATCH_NO), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TS1109");
    EXPECT_EQ(diagnostic.start_pos, 14u);
    EXPECT_EQ(diagnostic.length, 1u);
    EXPECT_EQ(diagnostic.line, 1u);
    EXPECT_EQ(diagnostic.column, 0u);
}

TEST_F(TypeScriptExpressionFixture, synchronizes_statements_and_continues_after_multiple_errors) {
    const char *source =
        ") broken ; const middle = 1; ] damaged ; let tail = 2;";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(count_nodes_with_flag(root, TEXTPARSER_NODE_RECOVERED), 2u);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 2u);

    textparser_diagnostic first{};
    textparser_diagnostic second{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &first), 0);
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 1, &second), 0);
    EXPECT_STREQ(first.code, "TS1128");
    EXPECT_STREQ(second.code, "TS1128");
    EXPECT_EQ(first.start_pos, 0u);
    EXPECT_EQ(first.length, 8u);
    EXPECT_EQ(second.start_pos, 28u);
    EXPECT_EQ(second.length, 10u);
    EXPECT_NE(find_cst_kind(parser.get(), root, "VariableStatement"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, uses_context_specific_class_type_and_switch_boundaries) {
    struct Case {
        const char *source;
        const char *code;
        const char *kind;
    } cases[] = {
        {"class Box { ) broken ; value: number; }", "TS1068", "ClassElement"},
        {"interface Shape { ) broken; good: string; }", "TS1131", "TypeMember"},
        {"switch (value) { ) broken; case 1: break; }", "TS1130", "CaseClause"},
    };
    for (const auto &item : cases) {
        textparser_node *root = parse_source(item.source);
        ASSERT_NE(root, nullptr) << item.source;
        ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u) << item.source;
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, item.code) << item.source;
        EXPECT_NE(find_cst_kind(parser.get(), root, item.kind, TEXTPARSER_NODE_RECOVERED), nullptr)
            << item.source;
    }
}

TEST_F(TypeScriptExpressionFixture, does_not_recover_without_a_resumption_point) {
    EXPECT_EQ(parse_source(") terminal junk", TEXTPARSER_MATCH_NO), nullptr);
    EXPECT_EQ(count_nodes_with_flag(result.node, TEXTPARSER_NODE_RECOVERED), 0u);
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);

    EXPECT_EQ(parse_source("const value = ;", TEXTPARSER_MATCH_NO), nullptr);
    EXPECT_EQ(count_nodes_with_flag(result.node, TEXTPARSER_NODE_RECOVERED), 0u);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TS1109");
}

TEST_F(TypeScriptExpressionFixture, reports_control_flow_early_errors_with_keyword_spans) {
    ASSERT_NE(parse_source("return;\nbreak;\ncontinue;"), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 3u);

    struct Expected { const char *code; size_t start; size_t length; } expected[] = {
        {"TS1108", 0, 6}, {"TS1105", 8, 5}, {"TS1104", 15, 8},
    };
    for (size_t i = 0; i < 3; i++) {
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, expected[i].code);
        EXPECT_EQ(diagnostic.start_pos, expected[i].start);
        EXPECT_EQ(diagnostic.length, expected[i].length);
    }
}

TEST_F(TypeScriptExpressionFixture, honors_control_flow_context_and_function_boundaries) {
    ASSERT_NE(parse_source(
        "function valid(value) {\n"
        "  while (value) { continue; break; }\n"
        "  switch (value) { case 1: break; }\n"
        "  return;\n"
        "}\n"
        "outer: while (ready) { continue outer; }"), nullptr);
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);

    ASSERT_NE(parse_source(
        "while (ready) {\n"
        "  function nested() { break; continue; }\n"
        "}"), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 2u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TS1105");
    EXPECT_EQ(diagnostic.start_pos, 38u);
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 1, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TS1104");
    EXPECT_EQ(diagnostic.start_pos, 45u);

    ASSERT_NE(parse_source(
        "function outer() {\n"
        "  while (ready) { class Nested { static { return; break; continue; } } }\n"
        "}"), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 3u);
    const char *codes[] = {"TS1108", "TS1105", "TS1104"};
    for (size_t i = 0; i < 3; i++) {
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, codes[i]);
    }
}

TEST_F(TypeScriptExpressionFixture, resolves_labels_and_rejects_invalid_jump_targets) {
    ASSERT_NE(parse_source(
        "outer: inner: while (ready) { continue outer; break inner; }\n"
        "block: { break block; }\n"
        "same: while (ready) { function nested() { same: break same; } }"), nullptr);
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);

    ASSERT_NE(parse_source(
        "block: { continue block; }\n"
        "break missing;\n"
        "again: again: while (ready) break again;\n"
        "outer: while (ready) { function nested() { break outer; } }"), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 4u);
    const char *codes[] = {"TS1115", "TS1116", "TS1114", "TS1107"};
    for (size_t i = 0; i < 4; i++) {
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, codes[i]);
    }
}

TEST_F(TypeScriptExpressionFixture, checks_await_and_yield_against_nearest_function_kind) {
    ASSERT_NE(parse_source(
        "async function load() { await source; }\n"
        "function *values() { yield; yield* source; }\n"
        "const task = async () => await source;\n"
        "const iterator = { *items() { yield value; } };"), nullptr);
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);

    ASSERT_NE(parse_source(
        "function plain(value = left * right) { await value; yield value; }\n"
        "async function outer() { function nested() { await value; } }\n"
        "async function wrapper() { const nested = () => await value; }\n"
        "function *generator() { function nested() { yield value; } }"), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 5u);
    const char *codes[] = {"TS1308", "TS1163", "TS1308", "TS1308", "TS1163"};
    for (size_t i = 0; i < 5; i++) {
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, codes[i]);
        EXPECT_GT(diagnostic.length, 0u);
    }

    ASSERT_NE(parse_source("await topLevel;\nyield topLevel;"), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "TS1163");
    EXPECT_EQ(diagnostic.start_pos, 16u);
    EXPECT_EQ(diagnostic.length, 5u);
}

TEST_F(TypeScriptExpressionFixture, validates_class_modifiers_and_accessor_shapes) {
    ASSERT_NE(parse_source(
        "class Invalid {\n"
        "  public private value: number;\n"
        "  static static duplicate() {}\n"
        "  readonly method() {}\n"
        "  accessor wrong() {}\n"
        "  async field: number;\n"
        "  get read(value: number) {}\n"
        "  set write() {}\n"
        "  set typed(value: number): void {}\n"
        "  get generic<T>() {}\n"
        "}"), nullptr);
    const char *codes[] = {
        "TS1028", "TS1030", "TS1024", "TS1031", "TS1042",
        "TS1054", "TS1049", "TS1095", "TS1094",
    };
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 9u);
    for (size_t i = 0; i < 9; i++) {
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, codes[i]);
        EXPECT_GT(diagnostic.length, 0u);
    }
}

TEST_F(TypeScriptExpressionFixture, validates_ambient_bodies_initializers_and_nested_declare) {
    ASSERT_NE(parse_source(
        "declare const initialized: number = 1;\n"
        "declare async function implemented() { return; }"), nullptr);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 3u);
    const char *direct_codes[] = {"TS1039", "TS1183", "TS1040"};
    for (size_t i = 0; i < 3; i++) {
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, direct_codes[i]);
    }

    ASSERT_NE(parse_source(
        "declare namespace Ambient {\n"
        "  declare const nested: number;\n"
        "  expression;\n"
        "}\n"
        "declare class Shape {\n"
        "  value: number = 1;\n"
        "  get current(): number;\n"
        "}", TEXTPARSER_MATCH_OK, "types.d.ts"), nullptr);
    const char *file_codes[] = {"TS1038", "TS1036", "TS1039", "TS1086"};
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 4u);
    for (size_t i = 0; i < 4; i++) {
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, file_codes[i]);
    }

    ASSERT_NE(parse_source(
        "declare const value: number;\n"
        "declare function call(input: string): void;",
        TEXTPARSER_MATCH_OK, "valid.d.ts"), nullptr);
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);
}

TEST_F(TypeScriptExpressionFixture, rejects_incomplete_module_declarations) {
    for (const char *source : {
             "import { value from 'pkg';", "import value from;", "export default ;",
             "import defer defaultValue from 'pkg';", "import defer { value } from 'pkg';",
             "import { 'arbitrary-name' } from 'pkg';", "import { value as 'bad-local' } from 'pkg';",
             "export * 'pkg';", "declare module 'pkg' {",
         }) {
        EXPECT_EQ(parse_source(source, TEXTPARSER_MATCH_NO), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_jsx_elements_fragments_and_attributes) {
    for (const char *source : {
             "<div />",
             "<></>",
             "<input disabled data-id='field' />",
             "<UI.Button value={count + 1} {...props} />",
             "<svg:path stroke-width=\"2\" />",
         }) {
        ASSERT_NE(parse_expression(source, TEXTPARSER_MATCH_OK, "component.tsx"), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_nested_jsx_text_and_expression_children) {
    const char *source =
        "<section>Hello &amp; <strong>{value + 1}</strong>"
        "{condition ? <yes /> : <no />}{/* empty expression */}</section>";
    ASSERT_NE(parse_expression(source, TEXTPARSER_MATCH_OK, "component.tsx"), nullptr);
    ASSERT_NE(parse_source("const view = <><Header /><main>{content}</main></>;",
                           TEXTPARSER_MATCH_OK, "component.tsx"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, jsx_modes_roll_back_and_reject_incomplete_forms) {
    for (const char *source : {
             "<div", "<div>", "<div attr= />", "< /> trailing",
             "<Panel></Pane>", "<UI.Panel></UI.Pane>", "<Panel></panel>",
         }) {
        EXPECT_EQ(parse_expression(source, TEXTPARSER_MATCH_NO, "component.tsx"), nullptr) << source;
    }
    EXPECT_EQ(parse_expression("<div>{value</div>", TEXTPARSER_MATCH_ERROR, "component.tsx"), nullptr);
    ASSERT_NE(parse_expression("left < right", TEXTPARSER_MATCH_OK, "component.tsx"), nullptr);
    ASSERT_STREQ(textparser_get_current_mode(parser.get()), "default");
}

TEST_F(TypeScriptExpressionFixture, selects_typescript_source_profiles_from_filename) {
    ASSERT_NE(parse_source("const value: number = 1;", TEXTPARSER_MATCH_OK, "source.ts"), nullptr);
    ASSERT_NE(parse_source("declare const value: number;", TEXTPARSER_MATCH_OK, "types.d.ts"), nullptr);
    ASSERT_NE(parse_source("export interface Value { item: string }", TEXTPARSER_MATCH_OK,
                           "types.D.MTS"), nullptr);
    ASSERT_NE(parse_expression("<number>value", TEXTPARSER_MATCH_OK, "source.ts"), nullptr);
    EXPECT_EQ(parse_expression("<number>value", TEXTPARSER_MATCH_NO, "source.tsx"), nullptr);
    EXPECT_EQ(parse_source("const value: number = 1;", TEXTPARSER_MATCH_NO, "source.js"), nullptr);
    EXPECT_EQ(parse_source("function read(value: number) {}", TEXTPARSER_MATCH_ERROR,
                           "source.cjs"), nullptr);
    ASSERT_NE(parse_source("const value = 1;", TEXTPARSER_MATCH_OK, "source.mjs"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, jsx_is_available_only_in_tsx_and_jsx_profiles) {
    ASSERT_NE(parse_expression("<View value={item} />", TEXTPARSER_MATCH_OK, "view.tsx"), nullptr);
    ASSERT_NE(parse_expression("<View value={item} />", TEXTPARSER_MATCH_OK, "view.jsx"), nullptr);
    EXPECT_EQ(parse_expression("<View />", TEXTPARSER_MATCH_NO, "view.ts"), nullptr);
    EXPECT_EQ(parse_expression("<View />", TEXTPARSER_MATCH_NO, "view.js"), nullptr);
    EXPECT_EQ(parse_expression("<View />", TEXTPARSER_MATCH_NO, "view.d.ts"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, tsx_generic_arrows_require_disambiguating_syntax) {
    ASSERT_NE(parse_expression("<T,>(value: T) => value", TEXTPARSER_MATCH_OK, "arrow.tsx"), nullptr);
    ASSERT_NE(parse_expression("<T extends {}>(value: T) => value", TEXTPARSER_MATCH_OK,
                               "arrow.tsx"), nullptr);
    ASSERT_NE(parse_expression("<T>(value: T) => value", TEXTPARSER_MATCH_OK, "arrow.ts"), nullptr);
    EXPECT_EQ(parse_expression("<T>(value: T) => value", TEXTPARSER_MATCH_NO, "arrow.tsx"), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_keyword_reference_and_nested_generic_types) {
    for (const char *source : {
             "any", "unknown", "never", "void", "undefined", "null", "number",
             "bigint", "boolean", "string", "symbol", "object",
             "Namespace.Model<string, number[]>",
             "readonly Promise<Map<string, number[]>>",
             "Outer<Middle<Inner<string>>>",
         }) {
        ASSERT_NE(parse_type(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_composed_and_conditional_types) {
    for (const char *source : {
             "string | number & null", "keyof (A | B)", "T[K][]",
             "T extends readonly unknown[] ? T[number] : never",
             "-1 | 'ready' | true | 42n",
         }) {
        ASSERT_NE(parse_type(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_tuple_function_and_constructor_types) {
    for (const char *source : {
             "readonly [name: string, count?: number, ...rest: boolean[]]",
             "<T extends object = {}>(value: T, ...rest: readonly string[]) => Promise<T>",
             "abstract new <T>(value: T) => Box<T>",
         }) {
        ASSERT_NE(parse_type(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_object_mapped_and_indexed_types) {
    for (const char *source : {
             "{ readonly id?: number; get<T>(value: T): T; [key: string]: unknown }",
             "{ readonly [K in keyof T as K]?: T[K]; }",
             "{ new <T>(value: T): Box<T>; (value: string): number; }",
         }) {
        ASSERT_NE(parse_type(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_queries_predicates_imports_and_infer) {
    for (const char *source : {
             "typeof namespace.value", "asserts value is string", "value is number",
             "import('pkg').Model<string>", "T extends infer U ? U : never",
         }) {
        ASSERT_NE(parse_type(source), nullptr) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, rejects_incomplete_types) {
    for (const char *source : {
             "Promise<string", "string |", "(value: string) number", "{ [K in keyof T]: T[K]",
         }) {
        parse_type(source, TEXTPARSER_MATCH_NO);
    }
}

TEST_F(TypeScriptExpressionFixture, parses_literal_families) {
    struct Case { const char *source; const char *kind; } cases[] = {
        {"0", "NumericLiteral"}, {"1_000.25e-2", "NumericLiteral"},
        {"0xCA_FE", "NumericLiteral"}, {"0b1010n", "BigIntLiteral"},
        {"'hello\\nworld'", "StringLiteral"}, {"\"hello\"", "StringLiteral"},
        {"`plain template`", "NoSubstitutionTemplateLiteral"},
        {"true", "TrueKeyword"}, {"false", "FalseKeyword"},
        {"null", "NullKeyword"}, {"undefined", "UndefinedKeyword"},
    };
    for (const auto &item : cases) {
        textparser_node *node = parse_expression(item.source);
        ASSERT_NE(node, nullptr) << item.source;
        EXPECT_EQ(node->token_id, token(item.kind)) << item.source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_unicode_and_escaped_identifiers) {
    ASSERT_NE(parse_expression("π"), nullptr);
    for (const char *source : {
             "const π = 1;", "const 变量 = π;", "const café = 2;",
             "const café = café;", "const \\u0061lias = 3;",
             "const \\u{10400}name = 4;",
         }) {
        ASSERT_NE(parse_source(source), nullptr) << source;
    }
    EXPECT_EQ(parse_source("const #ignored = 0;", TEXTPARSER_MATCH_NO), nullptr);
}

TEST_F(TypeScriptExpressionFixture, parses_hashbang_and_complete_literal_edges) {
    for (const char *source : {
             "#!/usr/bin/env node\nconst value = 1;",
             "const trailing = 1.;",
             "const continued = 'left\\\nright';",
             "const template = `line one\nline two \\` value`;",
         }) {
        ASSERT_NE(parse_source(source), nullptr) << source;
    }
    EXPECT_EQ(parse_source("value;\n#!not-at-start", TEXTPARSER_MATCH_NO), nullptr);

    size_t token_count = 0;
    const textparser_lex_token *tokens =
        textparser_get_lexer_tokens(parser.get(), &token_count);
    ASSERT_NE(tokens, nullptr);
    EXPECT_GT(token_count, 0u);
}

TEST_F(TypeScriptExpressionFixture, reports_malformed_lexer_tokens) {
    for (const char *source : {
             "1__0;", "0x;", "'unterminated", "`unterminated", "\\u0030bad;",
             "\\u2603bad;",
         }) {
        ASSERT_NE(parse_source(source), nullptr) << source;
        ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u) << source;
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, "TS_LEXICAL") << source;
        EXPECT_EQ(diagnostic.severity, TEXTPARSER_SEVERITY_ERROR) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_template_substitutions_and_tagged_templates) {
    for (const char *source : {
             "`hello ${name}`",
             "`left ${first} middle ${second + 1} right`",
             "`object ${{ value: 1 }.value}`",
             "`nested ${`inner ${value}`}`",
             "tag`value ${item}`",
             "obj.tag<string>`value ${item}`",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
        textparser_parser_state_view state{};
        ASSERT_EQ(parser.parser_state(&state), 0);
        EXPECT_EQ(state.source_offset, std::strlen(source)) << source;
    }
}

TEST_F(TypeScriptExpressionFixture, parses_generic_calls_and_expression_assertions) {
    for (const char *source : {
             "identity<string>(value)",
             "factory<Map<string, number>>()",
             "object.method<Result>(value).property",
             "factory<Result>",
             "factory<Result>.create()",
             "value as Result",
             "value satisfies Constraint",
             "value as const",
             "<Result>value",
             "(value as Result).property",
         }) {
        ASSERT_NE(parse_expression(source), nullptr) << source;
        textparser_parser_state_view state{};
        ASSERT_EQ(parser.parser_state(&state), 0);
        EXPECT_EQ(state.source_offset, std::strlen(source)) << source;
    }

    const char *comparison = "left < right > value";
    ASSERT_NE(parse_expression(comparison), nullptr);
    textparser_parser_state_view comparison_state{};
    ASSERT_EQ(parser.parser_state(&comparison_state), 0);
    EXPECT_EQ(comparison_state.source_offset, std::strlen(comparison));
}

TEST_F(TypeScriptExpressionFixture, rejects_incomplete_templates_calls_and_assertions) {
    struct Case { const char *source; textparser_match_status status; } cases[] = {
        {"`unterminated ${value", TEXTPARSER_MATCH_NO},
        {"tag`unterminated ${value", TEXTPARSER_MATCH_OK},
        {"identity<string>(", TEXTPARSER_MATCH_ERROR},
        {"value as", TEXTPARSER_MATCH_OK},
        {"value satisfies", TEXTPARSER_MATCH_OK},
        {"<Result>", TEXTPARSER_MATCH_NO},
    };
    for (const auto &item : cases) {
        parse_expression(item.source, item.status);
        textparser_parser_state_view state{};
        ASSERT_EQ(parser.parser_state(&state), 0);
        EXPECT_NE(state.source_offset, std::strlen(item.source)) << item.source;
    }
}

TEST_F(TypeScriptExpressionFixture, implements_typescript_operator_precedence) {
    textparser_node *root = parse_expression("a + b * c ** d");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->token_id, token("Plus"));
    ASSERT_NE(root->child, nullptr);
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_EQ(root->child->next->token_id, token("Multiply"));
    ASSERT_NE(root->child->next->child, nullptr);
    ASSERT_NE(root->child->next->child->next, nullptr);
    EXPECT_EQ(root->child->next->child->next->token_id, token("Exponent"));

    root = parse_expression("a = b = c");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->token_id, token("Assign"));
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_EQ(root->child->next->token_id, token("Assign"));
}

TEST_F(TypeScriptExpressionFixture, parses_prefix_postfix_conditional_and_parentheses) {
    textparser_node *root = parse_expression("typeof (++value) ? value-- : false");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->token_id, token("Question"));
    ASSERT_NE(root->child, nullptr);
    EXPECT_EQ(root->child->token_id, token("TypeofKeyword"));
    ASSERT_NE(root->child->next, nullptr);
    EXPECT_EQ(root->child->next->token_id, token("Decrement"));
    ASSERT_NE(root->child->next->next, nullptr);
    EXPECT_EQ(root->child->next->next->token_id, token("FalseKeyword"));
}

TEST_F(TypeScriptExpressionFixture, lexical_goals_distinguish_regex_from_division) {
    textparser_node *regex = parse_expression("/[a-z/]+/gi");
    ASSERT_NE(regex, nullptr);
    EXPECT_EQ(regex->token_id, token("RegularExpressionLiteral"));

    textparser_node *division = parse_expression("a / b / c");
    ASSERT_NE(division, nullptr);
    EXPECT_EQ(division->token_id, token("Slash"));
    ASSERT_NE(division->child, nullptr);
    EXPECT_EQ(division->child->token_id, token("Slash"));
}

TEST_F(TypeScriptExpressionFixture, preserves_comment_and_newline_trivia) {
    textparser_node *root = parse_expression("a /* comment */ +\n b");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->token_id, token("Plus"));
    size_t token_count = 0;
    const textparser_lex_token *tokens = textparser_get_lexer_tokens(parser.get(), &token_count);
    ASSERT_NE(tokens, nullptr);
    ASSERT_GE(token_count, 3u);
    bool found_line_terminator = false;
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].kind == token("Identifier") && tokens[i].start > 0 &&
            (tokens[i].flags & TEXTPARSER_LEX_FLAG_CONTAINS_LINE_TERMINATOR) != 0) {
            found_line_terminator = true;
        }
    }
    EXPECT_TRUE(found_line_terminator);
    size_t trivia_count = 0;
    const textparser_lex_trivia *trivia = textparser_get_lexer_trivia(parser.get(), &trivia_count);
    ASSERT_NE(trivia, nullptr);
    EXPECT_GE(trivia_count, 3u);
}

TEST_F(TypeScriptExpressionFixture, incomplete_expressions_roll_back) {
    struct Case { const char *source; textparser_match_status status; } cases[] = {
        {"a +", TEXTPARSER_MATCH_ERROR},
        {"a ? b", TEXTPARSER_MATCH_ERROR},
        {"(a + b", TEXTPARSER_MATCH_NO},
    };
    for (const auto &item : cases) {
        parse_expression(item.source, item.status);
        textparser_parser_state_view state{};
        ASSERT_EQ(parser.parser_state(&state), 0);
        EXPECT_EQ(state.token_index, 0u) << item.source;
    }
}

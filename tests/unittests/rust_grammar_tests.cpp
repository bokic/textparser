#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <rust_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct RustGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/rust_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &rust_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, RustGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(RustGrammarFixture, parses_modules_and_use_declarations) {
    const char *source =
        "mod outer;\n"
        "pub mod inner {\n"
        "    pub fn hello() {}\n"
        "}\n"
        "use std::collections::HashMap;\n"
        "use std::io::{self, Read, Write};\n"
        "use crate::inner::*;\n"
        "pub use self::inner as my_inner;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ModDeclaration"), nullptr);
    EXPECT_NE(find(node, "UseDeclaration"), nullptr);
}

TEST_P(RustGrammarFixture, parses_functions_with_signatures_and_attributes) {
    const char *source =
        "#[inline(always)]\n"
        "pub async fn fetch_data<'a, T: Clone>(url: &'a str, fallback: T) -> Result<T, String>\n"
        "where\n"
        "    T: Send + 'static,\n"
        "{\n"
        "    Ok(fallback)\n"
        "}\n"
        "pub const fn max(a: i32, b: i32) -> i32 {\n"
        "    if a > b { a } else { b }\n"
        "}\n"
        "pub unsafe fn raw_access(ptr: *const u8) -> u8 {\n"
        "    *ptr\n"
        "}\n"
        "extern \"C\" fn native_cb() {}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "OuterAttribute"), nullptr);
    EXPECT_NE(find(node, "WhereClause"), nullptr);
}

TEST_P(RustGrammarFixture, parses_structs_record_tuple_and_unit) {
    const char *source =
        "#[derive(Debug, Clone)]\n"
        "pub struct Point<T> {\n"
        "    pub x: T,\n"
        "    pub y: T,\n"
        "}\n"
        "pub struct Color(pub u8, pub u8, pub u8);\n"
        "pub struct UnitMarker;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StructDeclaration"), nullptr);
    EXPECT_NE(find(node, "RecordField"), nullptr);
    EXPECT_NE(find(node, "TupleField"), nullptr);
}

TEST_P(RustGrammarFixture, parses_enums_with_variants_and_discriminants) {
    const char *source =
        "pub enum WebEvent {\n"
        "    PageLoad,\n"
        "    KeyPress(char),\n"
        "    Click { x: i64, y: i64 },\n"
        "    Status = 200,\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "EnumDeclaration"), nullptr);
    EXPECT_NE(find(node, "EnumVariant"), nullptr);
}

TEST_P(RustGrammarFixture, parses_traits_and_impls) {
    const char *source =
        "pub trait Summary: Clone {\n"
        "    type Item;\n"
        "    const VERSION: u32;\n"
        "    fn summarize(&self) -> String;\n"
        "}\n"
        "impl<T: Clone> Summary for Point<T> {\n"
        "    type Item = T;\n"
        "    const VERSION: u32 = 1;\n"
        "    fn summarize(&self) -> String {\n"
        "        String::new()\n"
        "    }\n"
        "}\n"
        "impl Point<f64> {\n"
        "    pub fn origin() -> Self {\n"
        "        Point { x: 0.0, y: 0.0 }\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TraitDeclaration"), nullptr);
    EXPECT_NE(find(node, "ImplDeclaration"), nullptr);
    EXPECT_NE(find(node, "AssociatedTypeDeclaration"), nullptr);
    EXPECT_NE(find(node, "AssociatedConstDeclaration"), nullptr);
}

TEST_P(RustGrammarFixture, parses_unions_type_aliases_consts_and_statics) {
    const char *source =
        "pub union FloatOrInt {\n"
        "    pub f: f32,\n"
        "    pub i: u32,\n"
        "}\n"
        "pub type Kilometers = i32;\n"
        "pub const MAX_POINTS: u32 = 100_000;\n"
        "pub static mut COUNTER: i64 = 0;\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "UnionDeclaration"), nullptr);
    EXPECT_NE(find(node, "TypeAliasDeclaration"), nullptr);
    EXPECT_NE(find(node, "ConstDeclaration"), nullptr);
    EXPECT_NE(find(node, "StaticDeclaration"), nullptr);
}

TEST_P(RustGrammarFixture, parses_control_flow_expressions) {
    const char *source =
        "fn control_flow() {\n"
        "    if x > 10 {\n"
        "        foo();\n"
        "    } else if x > 5 {\n"
        "        bar();\n"
        "    } else {\n"
        "        baz();\n"
        "    }\n"
        "    if let Some(val) = opt {\n"
        "        consume(val);\n"
        "    }\n"
        "    let res = match number {\n"
        "        0 => \"zero\",\n"
        "        1..=9 => \"small\",\n"
        "        n if n > 100 => \"huge\",\n"
        "        _ => \"other\",\n"
        "    };\n"
        "    'outer: loop {\n"
        "        break 'outer 42;\n"
        "    }\n"
        "    while flag {\n"
        "        flag = false;\n"
        "    }\n"
        "    while let Some(i) = iter.next() {\n"
        "        process(i);\n"
        "    }\n"
        "    for item in collection {\n"
        "        print(item);\n"
        "    }\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfExpression"), nullptr);
    EXPECT_NE(find(node, "MatchExpression"), nullptr);
    EXPECT_NE(find(node, "LoopExpression"), nullptr);
    EXPECT_NE(find(node, "WhileExpression"), nullptr);
    EXPECT_NE(find(node, "ForExpression"), nullptr);
}

TEST_P(RustGrammarFixture, parses_closures_and_blocks) {
    const char *source =
        "fn closure_test() {\n"
        "    let add = |a: i32, b: i32| -> i32 { a + b };\n"
        "    let no_args = || 42;\n"
        "    let captured = move |x| x + 1;\n"
        "    let u = unsafe { *raw_ptr };\n"
        "    let a = async { 100 };\n"
        "    let c = const { 1 + 2 };\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ClosureExpression"), nullptr);
    EXPECT_NE(find(node, "UnsafeBlock"), nullptr);
    EXPECT_NE(find(node, "AsyncBlock"), nullptr);
    EXPECT_NE(find(node, "ConstBlock"), nullptr);
}

TEST_P(RustGrammarFixture, parses_struct_expressions_tuples_arrays_and_indexing) {
    const char *source =
        "fn complex_expr() {\n"
        "    let p = Point { x: 10, y: 20 };\n"
        "    let p2 = Point { x: 5, ..p };\n"
        "    let arr = [1, 2, 3];\n"
        "    let zeroes = [0; 100];\n"
        "    let tup = (1, \"hello\", true);\n"
        "    let x = p.x;\n"
        "    let first = arr[0];\n"
        "    let t_first = tup.0;\n"
        "    let casted = x as u64;\n"
        "    let question = result?;\n"
        "    let borrowed = &mut p;\n"
        "    let deref = *borrowed;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StructExpression"), nullptr);
    EXPECT_NE(find(node, "ArrayExpression"), nullptr);
    EXPECT_NE(find(node, "TupleOrParenthesizedExpression"), nullptr);
    EXPECT_NE(find(node, "IndexSuffix"), nullptr);
    EXPECT_NE(find(node, "AsCastSuffix"), nullptr);
    EXPECT_NE(find(node, "TrySuffix"), nullptr);
}

TEST_P(RustGrammarFixture, parses_turbofish_and_method_chaining) {
    const char *source =
        "fn method_chaining() {\n"
        "    let v = Vec::<i32>::new();\n"
        "    let res = list.iter().map(|x| x * 2).collect::<Vec<_>>();\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TurbofishArgs"), nullptr);
    EXPECT_NE(find(node, "FieldAccessSuffix"), nullptr);
}

TEST_P(RustGrammarFixture, parses_literals_strings_and_numbers) {
    const char *source =
        "fn literals() {\n"
        "    let s = \"Hello, world!\";\n"
        "    let r1 = r#\"raw \"quote\" string\"#;\n"
        "    let r2 = r##\"deep #\"nested\"# raw\"##;\n"
        "    let b = b\"byte string\";\n"
        "    let ch = 'a';\n"
        "    let hex = 0xDEAD_BEEF_u64;\n"
        "    let bin = 0b1010_0101;\n"
        "    let oct = 0o755;\n"
        "    let flt = 3.14159_f64;\n"
        "    let sci = 1.23e-4;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "LiteralExpression"), nullptr);
}

TEST_P(RustGrammarFixture, parses_macros_invocations_and_rules) {
    const char *source =
        "macro_rules! my_vec {\n"
        "    ( $( $x:expr ),* ) => {\n"
        "        {\n"
        "            let mut temp_vec = Vec::new();\n"
        "            $( temp_vec.push($x); )*\n"
        "            temp_vec\n"
        "        }\n"
        "    };\n"
        "}\n"
        "fn test_macro() {\n"
        "    println!(\"Number: {}\", 42);\n"
        "    let v = vec![1, 2, 3];\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "MacroRulesDeclaration"), nullptr);
    EXPECT_NE(find(node, "MacroInvocationItem"), nullptr);
}

TEST_P(RustGrammarFixture, parses_pattern_matching_complex) {
    const char *source =
        "fn patterns() {\n"
        "    match p {\n"
        "        Point { x: 0, y } => handle_y(y),\n"
        "        Point { x, .. } => handle_x(x),\n"
        "    }\n"
        "    match opt {\n"
        "        Some(val @ 1..=10) => in_range(val),\n"
        "        Some(ref mut inner) => modify(inner),\n"
        "        None => (),\n"
        "    }\n"
        "    let (a, b, ..) = tuple_val;\n"
        "    let [head, tail @ ..] = slice_val;\n"
        "    let Red | Blue | Green = color;\n"
        "}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "StructPattern"), nullptr);
    EXPECT_NE(find(node, "RangePattern"), nullptr);
    EXPECT_NE(find(node, "IdentifierPattern"), nullptr);
}

TEST_P(RustGrammarFixture, recovers_from_missing_semicolons_in_declarations) {
    const char *source =
        "fn buggy() {\n"
        "    let a = 10\n"
        "    let b = 20;\n"
        "}\n";
    parser.reset();
    EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    EXPECT_EQ(parser.parse(get_definition()), 0);
    result = {};
    int exec_rc = parser.execute_language_grammar(get_definition(), &result);
    EXPECT_EQ(exec_rc, 0);
}

} // namespace

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <javascript_definition.json.h>
#include <cstring>
#include <typescript.h>

namespace {
struct JavaScriptGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json = nullptr;
    textparser::Parser parser;
    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                "definitions/javascript_definition.json", &json), TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json, nullptr);
        }
    }
    void TearDown() override { if (json) textparser_free_language_definition(json); }
    const textparser_language_definition *definition() { return GetParam() ? json : &javascript_definition; }
    bool accepts(const char *source, const char *filename = nullptr) {
        parser.reset();
        if (parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8) != 0)
            return false;
        textparser_typescript_register_validators(parser.get());
        if (filename) textparser_set_filename(parser.get(), filename);
        if (parser.parse(definition()) != 0) return false;
        textparser_match_result result = {};
        if (parser.execute_language_grammar(definition(), &result) != 0 ||
            result.status != TEXTPARSER_MATCH_OK || textparser_get_diagnostic_count(parser.get()))
            return false;
        const textparser_lex_token *remaining = nullptr;
        return textparser_lexer_peek(parser.get(), 0,
            textparser_get_lexical_goal(parser.get()), &remaining) == 1 && !remaining;
    }
    const textparser_node *find(const textparser_node *node, const char *kind) {
        if (!node) return nullptr;
        const char *name = textparser_grammar_node_name(parser.get(), node);
        if (name && std::strcmp(name, kind) == 0) return node;
        for (auto *child = node->child; child; child = child->next)
            if (auto *found = find(child, kind)) return found;
        return nullptr;
    }
};
INSTANTIATE_TEST_SUITE_P(DefinitionSources, JavaScriptGrammarFixture, testing::Bool(),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "JSON" : "Static"; });

TEST_P(JavaScriptGrammarFixture, empty_trivia_hashbang_and_literals) {
    for (const char *s : {"", " \t\r\n", "// comment\n/* block */", "#!/usr/bin/env node\nlet x=1;",
        "const x = [0xff,0b101,0o77,1_000,1.5e-3,123n,true,false,null];",
        "const café = 'hello'; const \\u0061 = \"world\";"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(JavaScriptGrammarFixture, declarations_functions_and_binding_patterns) {
    for (const char *s : {"var a; let b=1,c=2; const d=3;",
        "const {a:b=1, ...rest}=obj; let [first,,third,...tail]=items;",
        "function f(a=1,{b},...rest){return [a,b,...rest];}",
        "for(const x=1;x<2;)break;",
        "async function f(x){return await x;} function* g(){yield 1;yield* items;}",
        "const f=(a,b=1,...rest)=>({a,b,rest}); const g=async x=>await x;",
        "const type=1, interface=2, readonly=3; type + interface + readonly;"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(JavaScriptGrammarFixture, classes_objects_and_members) {
    for (const char *s : {"class A {constructor(x){this.x=x;} method(){return this.x;} }",
        "class B extends A { #x=1; static count=0; static {this.count++;} get value(){return this.#x;} set value(x){this.#x=x;} }",
        "class B extends mixin(A) {constructor(){super();} method(){return super.method();}}",
        "const C=class extends A {}; const x=new C();",
        "class A { static(){} async(){} get(){} set(){} static=1; }",
        "const o={async(){},get(){},set(){},static(){}};",
        "const obj={a:1, ['x'+1]:2, ...rest, method(a){return a;}, get x(){return 1;}, set x(v){this.y=v;}};",
        "function F(){return new.target;}"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(JavaScriptGrammarFixture, statements_loops_and_exceptions) {
    for (const char *s : {"if(x){a();}else b(); while(x){break;} do{x--;}while(x);",
        "for(let i=0;i<10;i++){if(i===2)continue;} for(const x of xs)use(x); for(let k in obj)use(k);",
        "async function f(){for await(const x of xs){await use(x);}}",
        "switch(x){case 1: a();break;default:b();}",
        "try{f();}catch({message}){log(message);}finally{cleanup();}",
        "try{f();}catch{g();} label:for(;;){break label;} debugger;"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(JavaScriptGrammarFixture, modules) {
    for (const char *s : {"import 'side-effect'; import x,{a as b,c} from 'm'; import * as ns from 'n';",
        "import data from './data.json' with {type:'json'};",
        "export const a=1; export {a as b}; export * from 'm'; export * as ns from 'n';",
        "export default function(){}", "export default class extends A {}",
        "const m=import('m'); const url=import.meta.url;",
        "import type from 'm'; export {type};"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(JavaScriptGrammarFixture, operators_regex_templates_and_asi) {
    for (const char *s : {"let x=a+b*c**d**e; x>>>=1; x&&=y; x?" "?=z; const y=x?.a?.[0]?.()??false;",
        "const re=/a[b/]+\\/c/gi; const n=a/b/c; if(x)/foo/.test(x);",
        "const s=`hello ${name} ${`nested ${value}`} ${ {a:1}.a }`; tag`x${a+b}`;",
        "let a=1\nlet b=2\na++\nb--\n",
        "function f(){return\n1;} function g(){return/*\n*/2;}",
        "const x=[,,1,...items,]; const y={a:1,};"}) {
        SCOPED_TRACE(s); EXPECT_TRUE(accepts(s));
    }
}
TEST_P(JavaScriptGrammarFixture, malformed_syntax_and_type_extensions_are_rejected) {
    for (const char *s : {"const x=;", "const x;", "let {x};", "var [x];", "function f( {}", "class A {m();}",
        "const x='unterminated", "const x=`unfinished", "const x=0xGG;", "/* unfinished", "let x=1; @",
        "throw\nvalue;", "const f=(x)\n=>x;", "const x=a?.;",
        "let x:number=1;", "interface A {}", "type T=number;", "enum E {A}",
        "function f<T>(x:T):T{return x;}", "const x=y as number;", "const x=y!;",
        "class A implements B {}", "class A {public x=1;}", "import type {A} from 'm';",
        "const f=(...xs,)=>xs;", "const [...xs,]=a;", "1=2;", "a?.b=1;", "++1;"}) {
        SCOPED_TRACE(s); EXPECT_FALSE(accepts(s));
    }
}
TEST_P(JavaScriptGrammarFixture, structured_tree_and_mode_restoration) {
    ASSERT_TRUE(accepts("function f(x){const y=`a${{b:x}.b}`; return x+2*3;}"));
    EXPECT_STREQ(textparser_get_current_mode(parser.get()), "default");
    textparser_match_result result = {};
    ASSERT_EQ(parser.execute_language_grammar(definition(), &result), 0);
    for (const char *kind : {"SourceFile", "FunctionDeclaration", "InitializedVariableDeclaration", "ReturnStatement", "TemplateLiteral"}) {
        SCOPED_TRACE(kind); EXPECT_NE(find(result.node, kind), nullptr);
    }
    EXPECT_EQ(textparser_node_get_category(result.node), TEXTPARSER_CST_SOURCE_FILE);
}
TEST_P(JavaScriptGrammarFixture, recovery_retains_following_statement) {
    EXPECT_FALSE(accepts("const broken=; const good=42;"));
    textparser_match_result result = {};
    ASSERT_EQ(parser.execute_language_grammar(definition(), &result), 0);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(result.node, "InitializedVariableDeclaration"), nullptr);
}
TEST_P(JavaScriptGrammarFixture, jsx_filename_profile_and_nested_modes) {
    for (const char *source : {"const x=<View value={item}><span>{`a${value}`}</span></View>;",
        "const x=<><A {...props}/>{items.map(x=><B key={x}/>)}</>;"}) {
        SCOPED_TRACE(source);
        EXPECT_TRUE(accepts(source, "view.jsx"));
        EXPECT_STREQ(textparser_get_current_mode(parser.get()), "default");
        EXPECT_FALSE(accepts(source, "view.js"));
    }
    EXPECT_FALSE(accepts("const x=<A></B>;", "view.jsx"));
    // Reject TS syntax independently of the supplied file name or lack of one.
    EXPECT_FALSE(accepts("let x:number=1;", "wrong.ts"));
}
} // namespace

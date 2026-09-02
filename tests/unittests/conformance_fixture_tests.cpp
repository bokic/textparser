/**
 * Task 10p: TypeScript differential conformance fixtures and regression coverage.
 *
 * The C engine is the golden standard. This harness:
 *  - iterates the fixture corpus under tests/docker/fixtures/typescript/valid   (*.ts)
 *    and tests/docker/fixtures/typescript/invalid (*.ts),
 *  - parses every fixture with the C grammar backend,
 *  - for valid fixtures asserts parse-to-completion, zero error diagnostics, and
 *    a byte-for-byte match of the canonical CST against the committed golden file,
 *  - for invalid fixtures asserts the recorded status and exact diagnostic list
 *    against the committed golden file.
 *
 * Set TEXTPARSER_REGENERATE_GOLDEN=1 to regenerate golden files from the C engine
 * (see tests/docker/fixtures/regenerate_golden.sh).
 */

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

const char *kFixtureRoot = "tests/docker/fixtures/typescript";

textparser_language_definition *load_typescript_definition() {
    textparser_language_definition *definition = nullptr;
    if (textparser_json_load_language_definition_from_json_file(
            "definitions/typescript_definition.json", &definition) != TEXTPARSER_JSON_NO_ERROR)
        return nullptr;
    return definition;
}

std::string read_whole_file(const fs::path &path) {
    std::ifstream stream(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

void write_whole_file(const fs::path &path, const std::string &content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << content;
}

std::string json_escape(const char *value) {
    std::ostringstream out;
    if (value == nullptr) return out.str();
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(value); *p != '\0'; ++p) {
        switch (*p) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (*p < 0x20) {
                    char hex[8];
                    std::snprintf(hex, sizeof(hex), "\\u%04x", *p);
                    out << hex;
                } else {
                    out << static_cast<char>(*p);
                }
        }
    }
    return out.str();
}

struct CstNode {
    std::string kind;
    size_t start;
    size_t end;
    uint32_t flags;
    uint32_t category;
    std::vector<CstNode> children;
};

CstNode describe_cst(textparser_t handle, const textparser_node *node) {
    CstNode out{};
    textparser_cst_node_view view{};
    if (textparser_get_cst_node_view(handle, node, &view) == 0 && view.kind != nullptr) {
        out.kind = view.kind;
        out.start = view.start;
        out.end = view.end;
        out.flags = view.flags;
    } else {
        out.kind = node->cst_kind ? node->cst_kind : "Unknown";
        out.start = node->source_start;
        out.end = node->source_end;
        out.flags = node->node_flags;
    }
    out.category = (uint32_t)textparser_typescript_cst_category_of(handle, node);
    for (const textparser_node *child = node->child; child != nullptr; child = child->next)
        out.children.push_back(describe_cst(handle, child));
    return out;
}

std::string serialize_cst(const CstNode &node, const std::string &indent = "") {
    std::ostringstream out;
    out << indent << "{\n";
    out << indent << "  \"kind\": \"" << node.kind << "\",\n";
    out << indent << "  \"start\": " << node.start << ",\n";
    out << indent << "  \"end\": " << node.end << ",\n";
    out << indent << "  \"flags\": " << node.flags << ",\n";
    out << indent << "  \"category\": " << node.category;
    if (!node.children.empty()) {
        out << ",\n" << indent << "  \"children\": [";
        for (size_t i = 0; i < node.children.size(); i++)
            out << (i ? ", " : "\n") << serialize_cst(node.children[i], indent + "    ");
        out << "\n" << indent << "  ]";
    }
    out << "\n" << indent << "}";
    return out.str();
}

struct DiagnosticRecord {
    int severity;
    std::string code;
    size_t start;
    size_t length;
    uint32_t line;
    uint32_t column;
};

std::vector<DiagnosticRecord> collect_diagnostics(textparser_t handle) {
    std::vector<DiagnosticRecord> records;
    size_t count = textparser_get_diagnostic_count(handle);
    records.reserve(count);
    for (size_t i = 0; i < count; i++) {
        textparser_diagnostic diagnostic{};
        if (textparser_get_diagnostic(handle, i, &diagnostic) != 0) continue;
        records.push_back({(int)diagnostic.severity,
                           diagnostic.code ? diagnostic.code : "",
                           diagnostic.start_pos,
                           diagnostic.length,
                           diagnostic.line,
                           diagnostic.column});
    }
    return records;
}

std::string serialize_diagnostic(const DiagnosticRecord &record) {
    std::ostringstream out;
    out << "{\"severity\":" << record.severity
        << ",\"code\":\"" << json_escape(record.code.c_str()) << "\""
        << ",\"start\":" << record.start
        << ",\"length\":" << record.length
        << ",\"line\":" << record.line
        << ",\"column\":" << record.column << "}";
    return out.str();
}

std::string serialize_diagnostics(const std::vector<DiagnosticRecord> &records) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < records.size(); i++)
        out << (i ? ",\n" : "\n") << "  " << serialize_diagnostic(records[i]);
    out << (records.empty() ? "]" : "\n]");
    return out.str();
}

std::string match_status_name(textparser_match_status status) {
    switch (status) {
        case TEXTPARSER_MATCH_OK: return "OK";
        case TEXTPARSER_MATCH_NO: return "NO";
        case TEXTPARSER_MATCH_ERROR: return "ERROR";
        case TEXTPARSER_MATCH_ABORT: return "ABORT";
    }
    return "UNKNOWN";
}

struct ParseOutcome {
    textparser_match_status status;
    std::string canonical;
    std::vector<DiagnosticRecord> diagnostics;
};

// Parses fixture text with the C grammar backend and produces the canonical
// CST / status / diagnostics snapshot used for the golden comparison.
ParseOutcome parse_fixture(textparser_language_definition *definition,
                           const std::string &source,
                           const std::string &filename) {
    ParseOutcome outcome;
    textparser::Parser parser;
    parser.openmem(source.c_str(), (int)source.size(), TEXTPARSER_ENCODING_UTF_8);
    if (!filename.empty()) textparser_set_filename(parser.get(), filename.c_str());
    parser.parse(definition);

    textparser_match_result result{};
    parser.execute_language_grammar(definition, &result);
    outcome.status = result.status;

    if (result.status == TEXTPARSER_MATCH_OK) {
        const textparser_lex_token *remaining = nullptr;
        int peek = textparser_lexer_peek(
            parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
        if (peek == 0 && remaining != nullptr) outcome.status = TEXTPARSER_MATCH_NO;
    }

    if (result.status == TEXTPARSER_MATCH_OK && result.node != nullptr)
        outcome.canonical = serialize_cst(describe_cst(parser.get(), result.node));
    outcome.diagnostics = collect_diagnostics(parser.get());
    return outcome;
}

bool regenerate_golden() { return std::getenv("TEXTPARSER_REGENERATE_GOLDEN") != nullptr; }

fs::path golden_for(const fs::path &fixture) {
    return kFixtureRoot / fs::path("golden") / (fixture.stem().string() + ".json");
}

std::string build_golden_int(const std::string &canonical, const ParseOutcome &outcome) {
    std::ostringstream out;
    out << "{\n  \"status\": \"" << match_status_name(outcome.status) << "\",\n";
    if (!canonical.empty())
        out << "  \"cst\": " << canonical << "\n";
    else
        out << "  \"cst\": null\n";
    out << "}";
    return out.str();
}

std::string build_golden_string(const std::string &, const ParseOutcome &outcome) {
    std::ostringstream out;
    out << "{\n  \"status\": \"" << match_status_name(outcome.status) << "\",\n"
        << "  \"diagnostics\": " << serialize_diagnostics(outcome.diagnostics) << "\n}";
    return out.str();
}

class TypeScriptFixtureConformance : public testing::Test {
protected:
    void SetUp() override {
        definition = load_typescript_definition();
        ASSERT_NE(definition, nullptr);
        ASSERT_NE(definition->grammar, nullptr);
    }

    void TearDown() override {
        textparser_free_language_definition(definition);
        definition = nullptr;
    }

    textparser_language_definition *definition = nullptr;
};

void test_valid_fixture(textparser_language_definition *definition, const fs::path &fixture) {
    SCOPED_TRACE(fixture.string());
    const std::string source = read_whole_file(fixture);
    ParseOutcome outcome = parse_fixture(definition, source, fixture.filename().string());

    // Valid fixtures must parse to completion.
    EXPECT_EQ(outcome.status, TEXTPARSER_MATCH_OK) << fixture.string();
    if (outcome.status != TEXTPARSER_MATCH_OK)
        GTEST_SKIP() << "Fixture does not currently parse to completion: " << fixture.string();

    // Valid fixtures must produce no error diagnostics.
    EXPECT_TRUE(outcome.diagnostics.empty())
        << "Valid fixture produced diagnostics: " << fixture.string();

    std::string golden = build_golden_int(outcome.canonical, outcome);
    fs::path golden_path = golden_for(fixture);

    if (regenerate_golden()) {
        write_whole_file(golden_path, golden + "\n");
        return;
    }

    ASSERT_TRUE(fs::exists(golden_path))
        << "Missing golden file. Regenerate with: "
           "TEXTPARSER_REGENERATE_GOLDEN=1 ./bin/unittests --gtest_filter=TypeScriptFixtureConformance.*";
    std::string expected = read_whole_file(golden_path);
    std::string expected_stripped = expected;
    while (!expected_stripped.empty() && std::isspace((unsigned char)expected_stripped.back()))
        expected_stripped.pop_back();
    // Regression lock: the C engine must still produce the same canonical CST.
    EXPECT_EQ(expected_stripped, golden) << "Canonical CST drifted for " << fixture.string();
}

void test_invalid_fixture(textparser_language_definition *definition, const fs::path &fixture) {
    SCOPED_TRACE(fixture.string());
    const std::string source = read_whole_file(fixture);
    ParseOutcome outcome = parse_fixture(definition, source, fixture.filename().string());

    std::string golden = build_golden_string(outcome.canonical, outcome);
    fs::path golden_path = golden_for(fixture);

    if (regenerate_golden()) {
        write_whole_file(golden_path, golden + "\n");
        return;
    }

    ASSERT_TRUE(fs::exists(golden_path))
        << "Missing golden file for invalid fixture. Regenerate as described above.";
    std::string expected = read_whole_file(golden_path);
    std::string expected_stripped = expected;
    while (!expected_stripped.empty() && std::isspace((unsigned char)expected_stripped.back()))
        expected_stripped.pop_back();
    EXPECT_EQ(expected_stripped, golden) << "Diagnostic/status drift for " << fixture.string();
}

TEST_F(TypeScriptFixtureConformance, valid_fixture_corpus_matches_golden) {
    fs::path valid_dir = fs::path(kFixtureRoot) / "valid";
    ASSERT_TRUE(fs::is_directory(valid_dir)) << valid_dir.string();

    std::vector<fs::path> fixtures;
    for (const auto &entry : fs::directory_iterator(valid_dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string ext = entry.path().extension().string();
        if (ext != ".ts" && ext != ".tsx" && ext != ".d.ts" && ext != ".mts" && ext != ".cts")
            continue;
        fixtures.push_back(entry.path());
    }
    ASSERT_FALSE(fixtures.empty()) << "No valid fixtures found in " << valid_dir.string();
    std::sort(fixtures.begin(), fixtures.end());

    for (const auto &fixture : fixtures) test_valid_fixture(definition, fixture);
}

TEST_F(TypeScriptFixtureConformance, invalid_fixture_corpus_matches_golden) {
    fs::path invalid_dir = fs::path(kFixtureRoot) / "invalid";
    ASSERT_TRUE(fs::is_directory(invalid_dir)) << invalid_dir.string();

    std::vector<fs::path> fixtures;
    for (const auto &entry : fs::directory_iterator(invalid_dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension().string() != ".ts") continue;
        fixtures.push_back(entry.path());
    }
    ASSERT_FALSE(fixtures.empty()) << "No invalid fixtures found in " << invalid_dir.string();
    std::sort(fixtures.begin(), fixtures.end());

    for (const auto &fixture : fixtures) test_invalid_fixture(definition, fixture);
}

// Regression tests for the engine bugs that blocked the valid fixture corpus.
class TypeScriptLegalityRegression : public testing::Test {
protected:
    void SetUp() override {
        definition = load_typescript_definition();
        ASSERT_NE(definition, nullptr);
    }

    void TearDown() override { textparser_free_language_definition(definition); }

    // Parses source and returns only the diagnostic codes, in order.
    std::vector<std::string> codes(const char *source, const char *filename = "sample.ts") {
        ParseOutcome outcome = parse_fixture(definition, source, filename);
        std::vector<std::string> result;
        for (const auto &record : outcome.diagnostics) result.push_back(record.code);
        return result;
    }

    void expect_clean(const char *source, const char *filename = "sample.ts") {
        EXPECT_TRUE(codes(source, filename).empty())
            << "Expected zero diagnostics for: " << source;
    }

    std::vector<std::string> expect_one(const char *source, const char *code) {
        std::vector<std::string> got = codes(source);
        EXPECT_EQ(got, std::vector<std::string>{code}) << " for: " << source;
        return got;
    }

    textparser_language_definition *definition = nullptr;
};

TEST_F(TypeScriptLegalityRegression, for_loops_increment_iteration_depth) {
    // Bug #1: for-loops used to emit an anonymous Sequence so the legality
    // checker never saw an iteration boundary. All for forms must allow break
    // and continue, and a labeled continue must target a for-loop.
    expect_clean("function f(){ for(;;) break; }");
    expect_clean("function f(){ for(;;) continue; }");
    expect_clean("function f(){ for(const k in o) continue; }");
    expect_clean("function f(){ for(const v of l) break; }");
    expect_clean("function f(){ label: for(;;){ continue label; } }");
    expect_clean("function f(){ label: for(const k of l){ break label; } }");

    // break outside any loop still errors.
    expect_one("function f(){ break; }", "TS1105");
    expect_one("function f(){ continue; }", "TS1104");

    // while / do / for all share the iteration depth.
    expect_clean("function f(){ while(cond){ break; } }");
    expect_clean("function f(){ do { continue; } while (cond); }");
}

TEST_F(TypeScriptLegalityRegression, declare_does_not_leak_across_sibling_statements) {
    // Bug #2: a module-level `declare` leaked ambient context into a sibling
    // class, and a `declare` class member leaked into sibling members.
    expect_clean("declare var g: number;\nexport class S { origin = 1; }");
    expect_clean("declare const c: number;\nclass S { v = 1 }");
    expect_clean("class D { declare f: number; protected a: string = \"x\"; }");
    expect_clean("class D { protected a: string = \"x\"; declare f: number; }");
    expect_clean("function f(){ declare var x: number; class C { y = 2; } }");

    // Genuine ambient contexts still reject initializers.
    expect_one("declare const initialized: number = 1;", "TS1039");
    expect_one("declare class S { value: number = 1; }", "TS1039");
    expect_clean("declare const value: number;\n");
}

TEST_F(TypeScriptLegalityRegression, switch_case_fallthrough_accepts_multiple_clauses) {
    // Bug #3: a `StatementList` inside a case greedily recovered across the
    // next `case`/`default` boundary, producing a false TS1128.
    expect_clean("function f(){ switch(m){ case 1: break; case 2: break; } }");
    expect_clean("function f(){ switch(m){ case 2: case 3: break; } }");
    expect_clean("function f(){ switch(m){ case 1: foo(); case 2: default: break; } }");
    expect_clean("function f(){ switch(m){ default: break; case 1: break; } }");
    expect_clean("function f(){ switch(m) { case 1: break; } }");
}

TEST_F(TypeScriptLegalityRegression, parenthesized_contextual_keyword_parameters) {
    // Bug #4: `(async) => async` was rejected because `async` is lexed as a
    // keyword and BindingParameter only accepted a plain Identifier.
    expect_clean("const arrow7 = (async) => async;");
    expect_clean("const g = (get) => get;");
    expect_clean("const s = (set) => set;");
    expect_clean("const f = (from) => from;");
    expect_clean("const arrow8 = (async, x) => async;");
    expect_clean("function call(async) { return async; }");
    expect_clean("call(get);");
    // The async modifier and single-param arrow forms must still work.
    expect_clean("const b = async (x) => x;");
    expect_clean("const c = async => async;");
    expect_clean("const d = () => 0;");
}

TEST_F(TypeScriptLegalityRegression, automatic_semicolon_insertion_is_silent) {
    // ASI: a semicolon inserted across a line terminator must not emit a
    // diagnostic and must allow the next statement to parse.
    expect_clean("const a = 1\nconst b = 2\nfoo();");
    expect_clean("function f(){ const x = 1\n  return x + 1; }");
    expect_clean("function f(){ if (a)\n  single();\n} even();");
    expect_clean("const asiBeforeClosingBrace = 5\nwhile (asiLoop) { break }");

    // A genuinely missing terminator on the same line is still an error (ASI
    // cannot insert a semicolon without a line terminator, `}`, or EOF).
    EXPECT_FALSE(codes("a = 1 b = 2", "sample.ts").empty());
}

TEST_F(TypeScriptLegalityRegression, compound_assignment_operators) {
    // All fifteen compound-assignment operators parse and require an assignable
    // left-hand side (TS2364 / TS2779).
    for (const char *op : {
             "+=", "-=", "*=", "/=", "%=", "**=", "<<=", ">>=", ">>>=", "|=",
             "&=", "^=", "??=", "||=", "&&=",
         }) {
        std::string source = std::string("let x = 5; x ") + op + " 2;";
        expect_clean(source.c_str());
    }

    // Chaining and nested member/index targets are assignable.
    expect_clean("let x = 1; x += 1; x **= x %= 3;");
    expect_clean("o.a.b **= 2;");
    expect_clean("o[\"k\"] ||= 1;");
    expect_clean("x >>= 1; x >>>= 1;");
    expect_clean("let y = f<string>; y ??= 1;");

    // Invalid left-hand sides keep the assignment diagnostics.
    EXPECT_EQ(codes("1 **= 2;"), std::vector<std::string>{"TS2364"});
    EXPECT_EQ(codes("let x = (a + b) <<= 2;"), std::vector<std::string>{"TS2364"});
    EXPECT_EQ(codes("f() &&= 2;"), std::vector<std::string>{"TS2364"});
    EXPECT_EQ(codes("x?.y ??= 1;"), std::vector<std::string>{"TS2779"});
}

TEST_F(TypeScriptLegalityRegression, jsx_namespaced_attributes) {
    // Namespaced attribute names parse in JSX-capable files (element names with
    // namespaces/members already worked).
    expect_clean("const e = <tag ns:attr=\"x\" />;", "component.tsx");
    expect_clean("const e = <tag ns:attr=\"x\" y=\"z\" />;", "component.tsx");
    expect_clean("const e = <tag ns:attr={value} />;", "component.tsx");
    expect_clean("const e = <svg:path xlink:href=\"#i\" stroke=\"c\" />;",
                 "component.tsx");
    expect_clean("const e = <tag ns:attr>text</tag>;", "component.tsx");

    // A bare namespace without an attribute name is malformed.
    EXPECT_FALSE(codes("const e = <tag ns: />;", "component.tsx").empty());
}

TEST_F(TypeScriptLegalityRegression, template_literal_types) {
    // Template literal types with substitutions parse in type positions.
    expect_clean("type T = `user:${string}`;");
    expect_clean("type T = `${string}`;");
    expect_clean("type T = `a${number}b${string}c`;");
    expect_clean("type T = `k-${'a' | 'b'}`;");
    expect_clean("type T = `${`inner-${string}`}`;");
    expect_clean("type T = `x${string}` | `y${number}`;");
    expect_clean("let s: `p${string}` = \"px\";");
    expect_clean("type Cond = T extends `f${string}` ? 1 : 2;");
    expect_clean("type Keys = { [K in `pre${string}`]: K };");
    expect_clean("type Tuple = [`a`, `b${number}`];");
    // The no-substitution form remains a plain literal type.
    expect_clean("type T = `plain`;");
}

} // namespace
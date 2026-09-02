// Parse a file with the textparser TypeScript grammar and print the canonical
// CST JSON (same form as the fixture golden files). Mirrors the C++ harness in
// tests/unittests/conformance_fixture_tests.cpp.
#include <textparser.hpp>
#include <textparser-json.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static std::string read_whole_file(const std::string &path) {
    std::ifstream stream(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

struct CstNode {
    std::string kind;
    size_t start;
    size_t end;
    uint32_t flags;
    uint32_t category;
    std::vector<CstNode> children;
};

static CstNode describe_cst(textparser_t handle, const textparser_node *node) {
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

static void serialize_cst(const CstNode &node, std::ostringstream &out, const std::string &indent = "") {
    out << indent << "{\n";
    out << indent << "  \"kind\": \"" << node.kind << "\",\n";
    out << indent << "  \"start\": " << node.start << ",\n";
    out << indent << "  \"end\": " << node.end << ",\n";
    out << indent << "  \"flags\": " << node.flags << ",\n";
    out << indent << "  \"category\": " << node.category;
    if (!node.children.empty()) {
        out << ",\n" << indent << "  \"children\": [";
        for (size_t i = 0; i < node.children.size(); i++) {
            if (i) out << ", ";
            out << "\n";
            serialize_cst(node.children[i], out, indent + "    ");
        }
        out << "\n" << indent << "  ]";
    }
    out << "\n" << indent << "}";
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.ts|.tsx|.d.ts|.mts|.cts>\n", argv[0]);
        return 1;
    }
    std::string source = read_whole_file(argv[1]);
    textparser_language_definition *definition = nullptr;
    if (textparser_json_load_language_definition_from_json_file(
            "definitions/typescript_definition.json", &definition) != TEXTPARSER_JSON_NO_ERROR) {
        fprintf(stderr, "cannot load typescript_definition.json\n");
        return 1;
    }
    textparser::Parser parser;
    parser.openmem(source.c_str(), (int)source.size(), TEXTPARSER_ENCODING_UTF_8);
    textparser_set_filename(parser.get(), argv[1]);
    parser.parse(definition);
    textparser_match_result result{};
    parser.execute_language_grammar(definition, &result);

    const char *status = "UNKNOWN";
    switch (result.status) {
        case TEXTPARSER_MATCH_OK: status = "OK"; break;
        case TEXTPARSER_MATCH_NO: status = "NO"; break;
        case TEXTPARSER_MATCH_ERROR: status = "ERROR"; break;
        case TEXTPARSER_MATCH_ABORT: status = "ABORT"; break;
    }
    if (result.status == TEXTPARSER_MATCH_OK) {
        const textparser_lex_token *remaining = nullptr;
        int peek = textparser_lexer_peek(parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
        if (peek == 0 && remaining != nullptr) status = "NO";
    }
    printf("{\n  \"status\": \"%s\",\n", status);
    if (result.status == TEXTPARSER_MATCH_OK && result.node != nullptr) {
        std::ostringstream out;
        serialize_cst(describe_cst(parser.get(), result.node), out);
        printf("  \"cst\": %s,\n", out.str().c_str());
    } else {
        printf("  \"cst\": null,\n");
    }
    printf("  \"diagnostics\": [");
    size_t emitted = 0;
    for (size_t i = 0; i < textparser_get_diagnostic_count(parser.get()); i++) {
        textparser_diagnostic d{};
        if (textparser_get_diagnostic(parser.get(), i, &d) != 0) continue;
        printf("%s{\"code\":\"%s\",\"start\":%zu,\"length\":%zu}",
               emitted ? "," : "", d.code ? d.code : "", d.start_pos, d.length);
        emitted++;
    }
    printf("]\n}\n");
    textparser_free_language_definition(definition);
    return 0;
}

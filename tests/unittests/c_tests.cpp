#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <cstring>
#include <vector>

#include <c_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

static const textparser_node *find_node(textparser_t handle, const textparser_node *node, const char *kind) {
    if (!node) return nullptr;
    const char *name = textparser_grammar_node_name(handle, node);
    if (name && std::strcmp(name, kind) == 0) return node;
    for (auto *child = node->child; child; child = child->next) {
        if (auto *found = find_node(handle, child, kind)) return found;
    }
    return nullptr;
}

static void collect_nodes(textparser_t handle, const textparser_node *node, const char *kind, std::vector<const textparser_node*> &out) {
    if (!node) return;
    const char *name = textparser_grammar_node_name(handle, node);
    if (name && std::strcmp(name, kind) == 0) out.push_back(node);
    for (auto *child = node->child; child; child = child->next) {
        collect_nodes(handle, child, kind, out);
    }
}

TEST(parse_C, basic_program) {
    auto tokens = TextParser(R"(
#include <stdio.h>

/* A standard block comment */
int main(void) {
    // Print message
    _Bool flag = true;
    char c = 'a';
    printf("Hello \n \"World\"!");
    if (flag == false || c != '\n') {
        int x = 42 + 2;
    }
    return 0;
}
)", &c_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Preprocessor"));
    EXPECT_TRUE(found.contains("IntKeyword"));
    EXPECT_TRUE(found.contains("VoidKeyword"));
    EXPECT_TRUE(found.contains("BoolKeyword"));
    EXPECT_TRUE(found.contains("CharKeyword"));
    EXPECT_TRUE(found.contains("CharacterLiteral"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("IfKeyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Equal"));
    EXPECT_TRUE(found.contains("NotEqual"));
    EXPECT_TRUE(found.contains("Plus"));
    EXPECT_TRUE(found.contains("ReturnKeyword"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Identifier"));
}

TEST(parse_C, unary_and_binary_expression_parsing) {
    const char *text = "int x = -1; int y = 10 - 10; int z = !3;";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(text, (int)strlen(text), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    textparser_match_result result = {};
    ASSERT_EQ(parser.execute_language_grammar(&c_definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_NE(result.node, nullptr);

    EXPECT_NE(find_node(parser.get(), result.node, "Declaration"), nullptr);
    EXPECT_NE(find_node(parser.get(), result.node, "Minus"), nullptr);
}

TEST(parse_C, scientific_notation) {
    auto tokens = TextParser("double a = 1e-9; double b = 2e+5; double c = 1.5E-3;", &c_definition);

    bool found_neg = false;
    bool found_pos = false;
    bool found_upper = false;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Number") == 0) {
            if (item.value == "1e-9") found_neg = true;
            if (item.value == "2e+5") found_pos = true;
            if (item.value == "1.5E-3") found_upper = true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };
    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }
    EXPECT_TRUE(found_neg);
    EXPECT_TRUE(found_pos);
    EXPECT_TRUE(found_upper);
}

TEST(parse_C, defer_macro_cpp_guard) {
#ifndef textparser_defer
    FAIL() << "textparser_defer should be defined when including textparser.hpp in C++ mode";
#endif
#ifndef textparser_parser_state_defer
    FAIL() << "textparser_parser_state_defer should be defined when including textparser.hpp in C++ mode";
#endif
}

TEST(parse_C, raii_wrapper_class) {
    const char *src = "int x = 10;";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(src, strlen(src), TEXTPARSER_ENCODING_LATIN1), 0);
    ASSERT_TRUE(static_cast<bool>(parser));
    ASSERT_EQ(parser.parse(&c_definition), 0);

    textparser_token_item *first = parser.get_first_token();
    EXPECT_NE(first, nullptr);

    textparser::State state = textparser::State::create(parser.get());
    EXPECT_NE(state.get(), nullptr);
}

TEST(parse_C, type_cast_vs_call_disambiguation) {
    // 1. (int)(x) should be parsed as CastExpression
    {
        textparser::Parser parser;
        const char *code = "void f(void) { (int)(x); }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&c_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&c_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "CastExpression"), nullptr);
    }

    // 2. (uint32_t)(*ptr) should be parsed as CastExpression
    {
        textparser::Parser parser;
        const char *code = "void f(void) { (uint32_t)(*ptr); }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&c_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&c_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_NE(find_node(parser.get(), result.node, "CastExpression"), nullptr);
    }

    // 3. (my_callback)(x) should remain a call expression, not a CastExpression
    {
        textparser::Parser parser;
        const char *code = "void f(void) { (my_callback)(x); }";
        ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(parser.parse(&c_definition), 0);
        textparser_match_result result = {};
        ASSERT_EQ(parser.execute_language_grammar(&c_definition, &result), 0);
        EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(find_node(parser.get(), result.node, "CastExpression"), nullptr);
    }
}

TEST(parse_C, declaration_disambiguates_parameter_types) {
    const char *code = R"(
void adv_regex_free(adv_regex_context *ctx, void **regex, int encoding);
result_type transform(input_context value, const options_context **options, int count);
void caller(void) {
    consume(left * right);
}
)";
    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);
    textparser_match_result result = {};
    ASSERT_EQ(parser.execute_language_grammar(&c_definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);

    std::vector<const textparser_node*> params;
    collect_nodes(parser.get(), result.node, "ParameterDeclaration", params);
    EXPECT_GE(params.size(), 6u);
}

TEST(parse_C, token_colors_distinct) {
    auto find_token = [](const char *name) -> const textparser_token * {
        for (size_t i = 0; c_definition.tokens[i].name != nullptr; ++i) {
            if (strcmp(c_definition.tokens[i].name, name) == 0) {
                return &c_definition.tokens[i];
            }
        }
        return nullptr;
    };

    const textparser_token *tok_preproc = find_token("Preprocessor");
    const textparser_token *tok_keyword = find_token("IfKeyword");
    const textparser_token *tok_type = find_token("StandardType");
    const textparser_token *tok_var = find_token("Identifier");
    const textparser_token *tok_op = find_token("Plus");
    const textparser_token *tok_str = find_token("StringLiteral");
    const textparser_token *tok_num = find_token("Number");
    const textparser_token *tok_bool = find_token("Boolean");

    ASSERT_NE(tok_preproc, nullptr);
    ASSERT_NE(tok_keyword, nullptr);
    ASSERT_NE(tok_type, nullptr);
    ASSERT_NE(tok_var, nullptr);
    ASSERT_NE(tok_op, nullptr);
    ASSERT_NE(tok_str, nullptr);
    ASSERT_NE(tok_num, nullptr);
    ASSERT_NE(tok_bool, nullptr);

    EXPECT_EQ(tok_preproc->text_color, 0xd7ba7d);
    EXPECT_EQ(tok_keyword->text_color, 0xc586c0);
    EXPECT_EQ(tok_type->text_color, 0x4ec9b0);
    EXPECT_EQ(tok_var->text_color, 0x9cdcfe);
    EXPECT_EQ(tok_op->text_color, 0xd4d4d4);
    EXPECT_EQ(tok_str->text_color, 0xce9178);
    EXPECT_EQ(tok_num->text_color, 0xb5cea8);
    EXPECT_EQ(tok_bool->text_color, 0x569cd6);
}

TEST(parse_C, c23_keywords_and_types) {
    const char *code = R"(
        #embed "data.bin"
        #elifdef FEATURE
        #elifndef OTHER_FEATURE

        [[nodiscard]] constexpr int get_val(void) {
            return 42;
        }

        [[maybe_unused, deprecated("reason")]] static_assert(sizeof(int) >= 4, "size check");

        int main(void) {
            nullptr_t np = nullptr;
            bool flag = true;
            constexpr auto x = 100;
            typeof(x) y = 200;
            typeof_unqual(const int) z = 300;
            _BitInt(128) big_val = 0;
            thread_local int tl_val = 1;
            alignas(16) int aligned_val = 2;
            size_t sz = alignof(int);
            return (int)sz;
        }
    )";

    auto tokens = TextParser(code, &c_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Preprocessor"));
    EXPECT_TRUE(found.contains("ConstexprKeyword"));
    EXPECT_TRUE(found.contains("NullptrKeyword"));
    EXPECT_TRUE(found.contains("TypeofKeyword"));
    EXPECT_TRUE(found.contains("TypeofUnqualKeyword"));
    EXPECT_TRUE(found.contains("StaticAssertKeyword"));
    EXPECT_TRUE(found.contains("ThreadLocalKeyword"));
    EXPECT_TRUE(found.contains("AlignasKeyword"));
    EXPECT_TRUE(found.contains("AlignofKeyword"));
    EXPECT_TRUE(found.contains("BitIntKeyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
}

TEST(parse_C, c23_literals_and_separators) {
    const char *code = R"(
        int bin = 0b1010'1100;
        int hex = 0xFF'EE'DD;
        int dec = 1'000'000;
        double float_dec = 3.1415'9265;
        double float_hex = 0x1.f'ap-2;
        unsigned int u = 42u;
        unsigned long long ull = 1'000ULL;
        size_t sz = 100uz;
        _BitInt(64) wb = 100wb;
        const char *u8str = u8"UTF-8 string with \x41 \u0041 \U00000041 \e \a \b \f \v \0";
        const char16_t *u16str = u"UTF-16 string";
        const char32_t *u32str = U"UTF-32 string";
        const wchar_t *wstr = L"Wide string";
        char u8c = u8'a';
        char16_t u16c = u'b';
        char32_t u32c = U'c';
        wchar_t wc = L'd';
    )";

    auto tokens = TextParser(code, &c_definition);

    bool found_bin = false;
    bool found_hex = false;
    bool found_dec = false;
    bool found_float_dec = false;
    bool found_float_hex = false;
    bool found_ull = false;
    bool found_uz = false;
    bool found_wb = false;

    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Number") == 0) {
            if (item.value == "0b1010'1100") found_bin = true;
            if (item.value == "0xFF'EE'DD") found_hex = true;
            if (item.value == "1'000'000") found_dec = true;
            if (item.value == "3.1415'9265") found_float_dec = true;
            if (item.value == "0x1.f'ap-2") found_float_hex = true;
            if (item.value == "1'000ULL") found_ull = true;
            if (item.value == "100uz") found_uz = true;
            if (item.value == "100wb") found_wb = true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };

    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }

    EXPECT_TRUE(found_bin);
    EXPECT_TRUE(found_hex);
    EXPECT_TRUE(found_dec);
    EXPECT_TRUE(found_float_dec);
    EXPECT_TRUE(found_float_hex);
    EXPECT_TRUE(found_ull);
    EXPECT_TRUE(found_uz);
    EXPECT_TRUE(found_wb);
}

TEST(parse_C, function_definition_and_calls) {
    const char *code = R"(
int compute_sum(int a, int b) {
    return a + b;
}

void process_data(void) {
    int result = compute_sum(10, 20);
    printf("Result: %d\n", result);
    custom_log(result);
}
)";

    textparser::Parser parser;
    ASSERT_EQ(parser.openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(&c_definition), 0);

    textparser_match_result result = {};
    ASSERT_EQ(parser.execute_language_grammar(&c_definition, &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);

    std::vector<const textparser_node*> func_defs;
    collect_nodes(parser.get(), result.node, "FunctionDefinition", func_defs);
    EXPECT_EQ(func_defs.size(), 2u);

    std::vector<const textparser_node*> calls;
    collect_nodes(parser.get(), result.node, "CallSuffix", calls);
    EXPECT_GE(calls.size(), 2u);
}

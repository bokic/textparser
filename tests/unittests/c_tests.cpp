#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <c_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
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

    // Root should find: Preprocessor (#include), BlockComment, Keyword (int, void, return), Variable (main, printf), CodeBlock, etc.
    bool found_preprocessor = false;
    bool found_block_comment = false;
    bool found_code_block = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        const char *t = tokens[i].type;
        std::string type = t ? t : "";
        if (type == "Preprocessor") {
            found_preprocessor = true;
        }
        if (type == "BlockComment") {
            found_block_comment = true;
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    EXPECT_TRUE(found_preprocessor);
    EXPECT_TRUE(found_block_comment);
    EXPECT_TRUE(found_code_block);

    // Get CodeBlock children
    int class_cb_idx = -1;
    for (size_t i = 0; i < tokens.count; ++i) {
        const char *t = tokens[i].type;
        if (t && std::string(t) == "CodeBlock") {
            class_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(class_cb_idx, -1);

    auto class_cb = tokens[class_cb_idx];
    bool found_line_comment = false;
    bool found_printf_var = false;
    bool found_double_str = false;
    bool found_return_keyword = false;
    bool found_number = false;

    for (size_t i = 0; i < class_cb.children; ++i) {
        const char *t = class_cb[i].type;
        std::string type = t ? t : "";
        if (type == "LineComment") found_line_comment = true;
        if (type == "Variable") found_printf_var = true;
        if (type == "DoubleString") found_double_str = true;
        if (type == "Keyword") found_return_keyword = true;
        if (type == "Number") found_number = true;
    }

    EXPECT_TRUE(found_line_comment);
    EXPECT_TRUE(found_printf_var);
    EXPECT_TRUE(found_double_str);
    EXPECT_TRUE(found_return_keyword);
    EXPECT_TRUE(found_number);

    // DoubleString should have nested StringEscape
    int double_str_idx = -1;
    for (size_t i = 0; i < class_cb.children; ++i) {
        const char *t = class_cb[i].type;
        if (t && std::string(t) == "DoubleString") {
            double_str_idx = i;
            break;
        }
    }
    ASSERT_NE(double_str_idx, -1);

    auto double_str = class_cb[double_str_idx];
    ASSERT_GE(double_str.children, 1);
    const char *dst = double_str[0].type;
    EXPECT_STREQ(dst ? dst : "", "StringEscape");

    // Comprehensive Token Coverage Check
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("Preprocessor"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("DataType"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Boolean"));
}

TEST(parse_C, sign_merge_top_level_and_incremental) {
    const char *text = "int x = -1; int y = 10-10; int z = !3;";
    auto tokens = TextParser(text, &c_definition);

    // Unary "-1" merges into a single Number; subtraction and "!3" do not merge.
    bool found_neg1 = false;
    bool found_neg10 = false;
    bool found_not3 = false;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Number") == 0) {
            if (item.value == "-1") found_neg1 = true;
            if (item.value == "-10") found_neg10 = true;
        }
        if (item.type && strcmp(item.type, "Operator") == 0 && item.value == "!") found_not3 = true;
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };
    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }
    EXPECT_TRUE(found_neg1);
    EXPECT_TRUE(found_not3);
    EXPECT_FALSE(found_neg10);

    // Incremental parser must produce identical sign-merge results.
    textparser_t handle = nullptr;
    ASSERT_EQ(textparser_openmem(text, strlen(text), TEXTPARSER_ENCODING_LATIN1, &handle), 0);
    ASSERT_NE(handle, nullptr);
    ASSERT_EQ(textparser_parse(handle, &c_definition), 0);

    bool inc_neg1 = false;
    std::function<void(const textparser_token_item *)> iscan = [&](const textparser_token_item *item) {
        if (item == nullptr) return;
        char *txt = textparser_get_token_text(handle, item);
        if (txt) {
            if (strcmp(txt, "-1") == 0) inc_neg1 = true;
            textparser_free_token_text(txt);
        }
        iscan(textparser_get_token_child(item));
        iscan(textparser_get_token_next(item));
    };
    iscan(textparser_get_first_token(handle));
    EXPECT_TRUE(inc_neg1);
    textparser_close(handle);
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
    // When including textparser.hpp in C++, textparser_defer and textparser_parser_state_defer must be defined.
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
    // 1. (int)(x) should be disambiguated as TypeCast
    {
        auto tokens = TextParser("(int)(x);", &c_definition);
        tokens.post_process();

        bool found_cast = false;
        for (size_t i = 0; i < tokens.count; ++i) {
            if (tokens[i].type && strcmp(tokens[i].type, "TypeCast") == 0) {
                found_cast = true;
            }
        }
        EXPECT_TRUE(found_cast);
    }

    // 2. (uint32_t)(*ptr) should be disambiguated as TypeCast
    {
        auto tokens = TextParser("(uint32_t)(*ptr);", &c_definition);
        tokens.post_process();

        bool found_cast = false;
        for (size_t i = 0; i < tokens.count; ++i) {
            if (tokens[i].type && strcmp(tokens[i].type, "TypeCast") == 0) {
                found_cast = true;
            }
        }
        EXPECT_TRUE(found_cast);
    }

    // 3. (my_callback)(x) should remain Parenthesis (function call / expression)
    {
        auto tokens = TextParser("(my_callback)(x);", &c_definition);
        tokens.post_process();

        bool found_cast = false;
        for (size_t i = 0; i < tokens.count; ++i) {
            if (tokens[i].type && strcmp(tokens[i].type, "TypeCast") == 0) {
                found_cast = true;
            }
        }
        EXPECT_FALSE(found_cast);
    }
}

TEST(parse_C, declaration_disambiguates_parameter_type_identifiers) {
    auto tokens = TextParser(R"(
void adv_regex_free(adv_regex_context *ctx, void **regex, enum textparser_encoding encoding);
result_type transform(input_context value, const options_context **options, int count);
void caller(void) {
    consume(left * right);
}
)", &c_definition);
    tokens.post_process();

    std::set<std::string> type_names;
    std::set<std::string> variables;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "TypeName") == 0) {
            type_names.insert(item.value);
        } else if (item.type && strcmp(item.type, "Variable") == 0) {
            variables.insert(item.value);
        }
        for (size_t i = 0; i < item.children; ++i) scan(item[i]);
    };
    for (size_t i = 0; i < tokens.count; ++i) scan(tokens[i]);

    EXPECT_TRUE(type_names.contains("adv_regex_context"));
    EXPECT_TRUE(type_names.contains("input_context"));
    EXPECT_TRUE(type_names.contains("options_context"));
    EXPECT_TRUE(variables.contains("ctx"));
    EXPECT_TRUE(variables.contains("regex"));
    EXPECT_TRUE(variables.contains("encoding"));
    EXPECT_TRUE(variables.contains("value"));
    EXPECT_TRUE(variables.contains("options"));
    EXPECT_TRUE(variables.contains("count"));
    EXPECT_FALSE(type_names.contains("left"));
    EXPECT_FALSE(type_names.contains("right"));
}

TEST(parse_C, token_colors_distinct) {
    // Verify that key token types have distinct and modern colors defined
    auto find_token = [](const char *name) -> const textparser_token * {
        for (size_t i = 0; c_definition.tokens[i].name != nullptr; ++i) {
            if (strcmp(c_definition.tokens[i].name, name) == 0) {
                return &c_definition.tokens[i];
            }
        }
        return nullptr;
    };

    const textparser_token *tok_comment = find_token("LineComment");
    const textparser_token *tok_preproc = find_token("Preprocessor");
    const textparser_token *tok_keyword = find_token("Keyword");
    const textparser_token *tok_type = find_token("DataType");
    const textparser_token *tok_var = find_token("Variable");
    const textparser_token *tok_op = find_token("Operator");
    const textparser_token *tok_str = find_token("DoubleString");
    const textparser_token *tok_num = find_token("Number");
    const textparser_token *tok_bool = find_token("Boolean");
    const textparser_token *tok_cast = find_token("TypeCast");

    ASSERT_NE(tok_comment, nullptr);
    ASSERT_NE(tok_preproc, nullptr);
    ASSERT_NE(tok_keyword, nullptr);
    ASSERT_NE(tok_type, nullptr);
    ASSERT_NE(tok_var, nullptr);
    ASSERT_NE(tok_op, nullptr);
    ASSERT_NE(tok_str, nullptr);
    ASSERT_NE(tok_num, nullptr);
    ASSERT_NE(tok_bool, nullptr);
    ASSERT_NE(tok_cast, nullptr);

    EXPECT_EQ(tok_comment->text_color, 0x6a9955);
    EXPECT_EQ(tok_preproc->text_color, 0xd7ba7d);
    EXPECT_EQ(tok_keyword->text_color, 0xc586c0);
    EXPECT_EQ(tok_type->text_color, 0x4ec9b0);
    EXPECT_EQ(tok_var->text_color, 0x9cdcfe);
    EXPECT_EQ(tok_op->text_color, 0xd4d4d4);
    EXPECT_EQ(tok_str->text_color, 0xce9178);
    EXPECT_EQ(tok_num->text_color, 0xb5cea8);
    EXPECT_EQ(tok_bool->text_color, 0x569cd6);
    EXPECT_EQ(tok_cast->text_color, 0x4ec9b0);

    // Verify all key token types have different colors
    std::set<uint32_t> colors = {
        tok_comment->text_color,
        tok_preproc->text_color,
        tok_keyword->text_color,
        tok_type->text_color,
        tok_var->text_color,
        tok_op->text_color,
        tok_str->text_color,
        tok_num->text_color,
        tok_bool->text_color
    };
    EXPECT_EQ(colors.size(), 9u);
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
    EXPECT_TRUE(found.contains("Attribute"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("DataType"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));

    // Check specific tokens
    bool found_nullptr = false;
    bool found_constexpr = false;
    bool found_typeof = false;
    bool found_typeof_unqual = false;
    bool found_static_assert = false;
    bool found_thread_local = false;
    bool found_alignas = false;
    bool found_alignof = false;
    bool found_nullptr_t = false;
    bool found_bitint = false;

    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Keyword") == 0) {
            if (item.value == "nullptr") found_nullptr = true;
            if (item.value == "constexpr") found_constexpr = true;
            if (item.value == "typeof") found_typeof = true;
            if (item.value == "typeof_unqual") found_typeof_unqual = true;
            if (item.value == "static_assert") found_static_assert = true;
            if (item.value == "thread_local") found_thread_local = true;
            if (item.value == "alignas") found_alignas = true;
            if (item.value == "alignof") found_alignof = true;
        }
        if (item.type && strcmp(item.type, "DataType") == 0) {
            if (item.value == "nullptr_t") found_nullptr_t = true;
            if (item.value == "_BitInt") found_bitint = true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };

    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }

    EXPECT_TRUE(found_nullptr);
    EXPECT_TRUE(found_constexpr);
    EXPECT_TRUE(found_typeof);
    EXPECT_TRUE(found_typeof_unqual);
    EXPECT_TRUE(found_static_assert);
    EXPECT_TRUE(found_thread_local);
    EXPECT_TRUE(found_alignas);
    EXPECT_TRUE(found_alignof);
    EXPECT_TRUE(found_nullptr_t);
    EXPECT_TRUE(found_bitint);
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

TEST(parse_C, function_detection) {
    auto tokens = TextParser(R"(
int compute_sum(int a, int b) {
    return a + b;
}

void process_data(void) {
    int result = compute_sum(10, 20);
    printf("Result: %d\n", result);
    custom_log   (result);
}
)", &c_definition);

    std::set<std::string> functions;
    std::set<std::string> variables;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Function") == 0) {
            functions.insert(item.value);
        } else if (item.type && strcmp(item.type, "Variable") == 0) {
            variables.insert(item.value);
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };

    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }

    EXPECT_TRUE(functions.contains("compute_sum"));
    EXPECT_TRUE(functions.contains("process_data"));
    EXPECT_TRUE(functions.contains("printf"));
    EXPECT_TRUE(functions.contains("custom_log"));

    EXPECT_TRUE(variables.contains("a"));
    EXPECT_TRUE(variables.contains("b"));
    EXPECT_TRUE(variables.contains("result"));
    EXPECT_FALSE(variables.contains("compute_sum"));
    EXPECT_FALSE(variables.contains("printf"));
}

TEST(parse_C, function_vs_keyword_and_variable) {
    auto tokens = TextParser(R"(
if (condition) {
    while (running()) {
        for (int i = 0; i < limit; ++i) {
            switch (get_val(i)) {
                case 1:
                    break;
            }
        }
    }
}
size_t sz = sizeof(int);
)", &c_definition);

    std::set<std::string> keywords;
    std::set<std::string> functions;
    std::set<std::string> variables;

    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Keyword") == 0) {
            keywords.insert(item.value);
        } else if (item.type && strcmp(item.type, "Function") == 0) {
            functions.insert(item.value);
        } else if (item.type && strcmp(item.type, "Variable") == 0) {
            variables.insert(item.value);
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };

    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }

    EXPECT_TRUE(keywords.contains("if"));
    EXPECT_TRUE(keywords.contains("while"));
    EXPECT_TRUE(keywords.contains("for"));
    EXPECT_TRUE(keywords.contains("switch"));
    EXPECT_TRUE(keywords.contains("sizeof"));

    EXPECT_FALSE(functions.contains("if"));
    EXPECT_FALSE(functions.contains("while"));
    EXPECT_FALSE(functions.contains("for"));
    EXPECT_FALSE(functions.contains("switch"));
    EXPECT_FALSE(functions.contains("sizeof"));

    EXPECT_TRUE(functions.contains("running"));
    EXPECT_TRUE(functions.contains("get_val"));

    EXPECT_TRUE(variables.contains("condition"));
    EXPECT_TRUE(variables.contains("limit"));
    EXPECT_TRUE(variables.contains("i"));
    EXPECT_TRUE(variables.contains("sz"));
}

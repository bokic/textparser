#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <csharp_definition.json.h>


TEST(parse_CSharp, basic_csharp_program) {
    auto tokens = TextParser(R"(
using System;
using System.Threading.Tasks;

namespace HelloNamespace {
    public class HelloWorld {
        public async Task<string> GetMessageAsync(int delayMs) {
            await Task.Delay(delayMs);
            string name = "World\n\t\"User\"";
            string defaultName = null;
            string finalName = name ?? defaultName;
            bool isActive = true;
            double score = 42.5;
            return $"Hello {finalName}!";
        }
    }
}
)", &csharp_definition);

    // Root should find: Keywords (using, namespace), Variable (System, Threading, Tasks, HelloNamespace), CodeBlock, etc.
    bool found_using_keyword = false;
    bool found_namespace_keyword = false;
    bool found_code_block = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        if (type == "Keyword") {
            if (std::string(tokens[i].type) == "Keyword") {
                found_using_keyword = true;
            }
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    // Since 'using' is a keyword and 'namespace' is a keyword
    EXPECT_TRUE(found_using_keyword);
    EXPECT_TRUE(found_code_block);

    // Get namespace CodeBlock
    int namespace_cb_idx = -1;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (std::string(tokens[i].type) == "CodeBlock") {
            namespace_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(namespace_cb_idx, -1);

    auto namespace_cb = tokens[namespace_cb_idx];
    bool found_class_keyword = false;
    bool found_class_code_block = false;

    for (size_t i = 0; i < namespace_cb.children; ++i) {
        std::string type = namespace_cb[i].type;
        if (type == "Keyword") {
            found_class_keyword = true;
        }
        if (type == "CodeBlock") {
            found_class_code_block = true;
        }
    }

    EXPECT_TRUE(found_class_keyword);
    EXPECT_TRUE(found_class_code_block);

    // Inside class CodeBlock
    int class_cb_idx = -1;
    for (size_t i = 0; i < namespace_cb.children; ++i) {
        if (std::string(namespace_cb[i].type) == "CodeBlock") {
            class_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(class_cb_idx, -1);

    auto class_cb = namespace_cb[class_cb_idx];
    bool found_async_keyword = false;
    bool found_method_code_block = false;

    for (size_t i = 0; i < class_cb.children; ++i) {
        std::string type = class_cb[i].type;
        if (type == "Keyword") {
            found_async_keyword = true;
        }
        if (type == "CodeBlock") {
            found_method_code_block = true;
        }
    }

    EXPECT_TRUE(found_async_keyword);
    EXPECT_TRUE(found_method_code_block);

    // Inside method CodeBlock
    int method_cb_idx = -1;
    for (size_t i = 0; i < class_cb.children; ++i) {
        if (std::string(class_cb[i].type) == "CodeBlock") {
            method_cb_idx = i;
            break;
        }
    }
    ASSERT_NE(method_cb_idx, -1);

    auto method_cb = class_cb[method_cb_idx];
    bool found_await_keyword = false;
    bool found_double_str = false;
    bool found_null_coalescing = false;
    bool found_boolean = false;
    bool found_number = false;

    for (size_t i = 0; i < method_cb.children; ++i) {
        std::string type = method_cb[i].type;
        if (type == "Keyword") {
            found_await_keyword = true;
        }
        if (type == "DoubleString") {
            found_double_str = true;
        }
        if (type == "Operator") {
            found_null_coalescing = true;
        }
        if (type == "Boolean") {
            found_boolean = true;
        }
        if (type == "Number") {
            found_number = true;
        }
    }

    EXPECT_TRUE(found_await_keyword);
    EXPECT_TRUE(found_double_str);
    EXPECT_TRUE(found_null_coalescing);
    EXPECT_TRUE(found_boolean);
    EXPECT_TRUE(found_number);

    // Verify nested string escape inside DoubleString
    int double_str_idx = -1;
    for (size_t i = 0; i < method_cb.children; ++i) {
        if (std::string(method_cb[i].type) == "DoubleString") {
            double_str_idx = i;
            break;
        }
    }
    ASSERT_NE(double_str_idx, -1);

    auto double_str = method_cb[double_str_idx];
    ASSERT_GE(double_str.children, 1);
    EXPECT_STREQ(double_str[0].type, "StringEscape");
}

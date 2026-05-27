#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <csharp_definition.json.h>


TEST(parse_CSharp, basic_csharp_program) {
    std::string source = R"(
// C# HelloWorld program
using System;
using System.Threading.Tasks;

/* A multi-line block comment
   to test BlockComment token */
namespace HelloNamespace {
    public class HelloWorld {
        public async Task<string> GetMessageAsync(int delayMs) {
            await Task.Delay(delayMs);
            char singleChar = '\n'; // SingleString with StringEscape
            char basicChar = 'a'; // SingleString
            string name = "World\n\t\"User\"";
            string defaultName = null;
            string finalName = name ?? defaultName;
            bool isActive = true;
            double score = 42.5;
            int hexNum = 0x2A; // hex number
            int binNum = 0b1010; // bin number
            int largeNum = 1_000_000; // digit separator
            Func<int, int> doubleVal = x => x * 2; // lambda operator =>
            global::System.Console.WriteLine("Done"); // namespace alias qualifier ::
            return $"Hello {finalName}!";
        }
    }
}
)";
    auto tokens = TextParser(source.c_str(), &csharp_definition);

    // Root should find: LineComment, BlockComment, Keywords (using, namespace), Variable (System, Threading, Tasks, HelloNamespace), CodeBlock, etc.
    bool found_line_comment = false;
    bool found_block_comment = false;
    bool found_using_keyword = false;
    bool found_namespace_keyword = false;
    bool found_code_block = false;
    bool found_variable_root = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        if (type == "LineComment") {
            found_line_comment = true;
        }
        if (type == "BlockComment") {
            found_block_comment = true;
        }
        if (type == "Keyword") {
            std::string val = source.substr(tokens[i].position, tokens[i].length);
            if (val == "using") {
                found_using_keyword = true;
            }
            if (val == "namespace") {
                found_namespace_keyword = true;
            }
        }
        if (type == "Variable") {
            found_variable_root = true;
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    // Assert all root level token types are correctly parsed and detected
    EXPECT_TRUE(found_line_comment);
    EXPECT_TRUE(found_block_comment);
    EXPECT_TRUE(found_using_keyword);
    EXPECT_TRUE(found_namespace_keyword);
    EXPECT_TRUE(found_variable_root);
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
    bool found_single_str = false;
    bool found_null_coalescing = false;
    bool found_lambda_op = false;
    bool found_alias_op = false;
    bool found_boolean = false;
    bool found_number = false;
    bool found_hex_number = false;
    bool found_bin_number = false;
    bool found_large_number = false;

    for (size_t i = 0; i < method_cb.children; ++i) {
        std::string type = method_cb[i].type;
        std::string value = source.substr(method_cb[i].position, method_cb[i].length);
        if (type == "Keyword") {
            found_await_keyword = true;
        }
        if (type == "DoubleString") {
            found_double_str = true;
        }
        if (type == "SingleString") {
            found_single_str = true;
        }
        if (type == "Operator") {
            if (value == "??") {
                found_null_coalescing = true;
            }
            if (value == "=>") {
                found_lambda_op = true;
            }
            if (value == "::") {
                found_alias_op = true;
            }
        }
        if (type == "Boolean") {
            found_boolean = true;
        }
        if (type == "Number") {
            found_number = true;
            if (value == "0x2A") {
                found_hex_number = true;
            }
            if (value == "0b1010") {
                found_bin_number = true;
            }
            if (value == "1_000_000") {
                found_large_number = true;
            }
        }
    }

    EXPECT_TRUE(found_await_keyword);
    EXPECT_TRUE(found_double_str);
    EXPECT_TRUE(found_single_str);
    EXPECT_TRUE(found_null_coalescing);
    EXPECT_TRUE(found_lambda_op);
    EXPECT_TRUE(found_alias_op);
    EXPECT_TRUE(found_boolean);
    EXPECT_TRUE(found_number);
    EXPECT_TRUE(found_hex_number);
    EXPECT_TRUE(found_bin_number);
    EXPECT_TRUE(found_large_number);

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

    // Verify nested string escape inside SingleString
    int single_str_idx = -1;
    for (size_t i = 0; i < method_cb.children; ++i) {
        if (std::string(method_cb[i].type) == "SingleString") {
            if (method_cb[i].children > 0) {
                single_str_idx = i;
                break;
            }
        }
    }
    ASSERT_NE(single_str_idx, -1);

    auto single_str = method_cb[single_str_idx];
    ASSERT_GE(single_str.children, 1);
    EXPECT_STREQ(single_str[0].type, "StringEscape");
}

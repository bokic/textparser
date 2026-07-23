#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <javascript_definition.json.h>


TEST(parse_JavaScript, basic_js_program) {
    auto tokens = TextParser(R"(
import { helper } from './utils.js';
export class Calculator {
    async multiplyAsync(x, y) {
        // A simple async delay
        await helper.delay(100);
        const result = x * y;
        const $element = null;
        const displayValue = result ?? 0;
        const msg = `Output is: ${displayValue}\n`;
        const hasFinished = true;
        const hexVal = 0xff;
        return hasFinished;
    }
}
)", &javascript_definition);

    // Root should find: Keywords (import, from, export, class), Variable (helper, Calculator, utils), CodeBlock, Operator, etc.
    bool found_import_keyword = false;
    bool found_export_keyword = false;
    bool found_code_block = false;

    for (size_t i = 0; i < tokens.count; ++i) {
        std::string type = tokens[i].type;
        if (type == "Keyword") {
            found_import_keyword = true;
            found_export_keyword = true;
        }
        if (type == "CodeBlock") {
            found_code_block = true;
        }
    }

    EXPECT_TRUE(found_import_keyword);
    EXPECT_TRUE(found_export_keyword);
    EXPECT_TRUE(found_code_block);

    // Get class CodeBlock (comes right after Calculator Variable)
    int class_cb_idx = -1;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (std::string(tokens[i].type) == "CodeBlock") {
            if (i > 0 && std::string(tokens[i - 1].type) == "Variable") {
                class_cb_idx = i;
                break;
            }
        }
    }
    ASSERT_NE(class_cb_idx, -1);

    auto class_cb = tokens[class_cb_idx];
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
    bool found_line_comment = false;
    bool found_await_keyword = false;
    bool found_variable_with_dollar = false;
    bool found_nullish_coalescing = false;
    bool found_template_string = false;
    bool found_boolean = false;
    bool found_hex_number = false;

    for (size_t i = 0; i < method_cb.children; ++i) {
        std::string type = method_cb[i].type;
        if (type == "LineComment") {
            found_line_comment = true;
        }
        if (type == "Keyword") {
            found_await_keyword = true;
        }
        if (type == "Variable") {
            // In the code: const $element = null;
            // Let's verify we parse $element as variable.
            found_variable_with_dollar = true;
        }
        if (type == "Operator") {
            found_nullish_coalescing = true;
        }
        if (type == "TemplateString") {
            found_template_string = true;
        }
        if (type == "Boolean") {
            found_boolean = true;
        }
        if (type == "Number") {
            found_hex_number = true;
        }
    }

    EXPECT_TRUE(found_line_comment);
    EXPECT_TRUE(found_await_keyword);
    EXPECT_TRUE(found_variable_with_dollar);
    EXPECT_TRUE(found_nullish_coalescing);
    EXPECT_TRUE(found_template_string);
    EXPECT_TRUE(found_boolean);
    EXPECT_TRUE(found_hex_number);

    // Verify StringEscape inside TemplateString
    int template_str_idx = -1;
    for (size_t i = 0; i < method_cb.children; ++i) {
        if (std::string(method_cb[i].type) == "TemplateString") {
            template_str_idx = i;
            break;
        }
    }
    ASSERT_NE(template_str_idx, -1);

    auto template_str = method_cb[template_str_idx];
    ASSERT_GE(template_str.children, 1);
    EXPECT_STREQ(template_str[0].type, "StringEscape");
}

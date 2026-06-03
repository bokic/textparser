#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <iostream>

#include <ada_definition.json.h>
#include <c_definition.json.h>
#include <cfml_definition.json.h>
#include <cpp_definition.json.h>
#include <csharp_definition.json.h>
#include <css_definition.json.h>
#include <fortran_definition.json.h>
#include <go_definition.json.h>
#include <java_definition.json.h>
#include <javascript_definition.json.h>
#include <matlab_definition.json.h>
#include <pascal_definition.json.h>
#include <perl_definition.json.h>
#include <php_definition.json.h>
#include <python_definition.json.h>
#include <r_definition.json.h>
#include <scratch_definition.json.h>
#include <swift_definition.json.h>
#include <typescript_definition.json.h>
#include <vb_definition.json.h>

static void verify_subtraction(const textparser_language_definition *definition, const char *lang_name) {
    // For embedded/templating languages (CFML, PHP) or CSS, we wrap subtraction in their valid block/tag syntax.
    std::string test_str = "10-10";
    if (strcmp(lang_name, "CFML") == 0) {
        test_str = "<cfset 10-10 />";
    } else if (strcmp(lang_name, "PHP") == 0) {
        test_str = "<?php 10-10; ?>";
    } else if (strcmp(lang_name, "CSS") == 0) {
        test_str = ".class { margin: 10px - 10px; }";
    }

    auto tokens = TextParser(test_str.c_str(), definition);
    
    // Let's scan to find Operator('-') or CSS's Value('-')
    bool found_subtraction = false;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && (strcmp(item.type, "Operator") == 0 || strcmp(item.type, "Value") == 0) && item.value == "-") {
            found_subtraction = true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };
    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }
    
    if (!found_subtraction) {
        std::cout << "DEBUG: " << lang_name << " subtraction tokens:" << std::endl;
        std::function<void(const TokenParserItem&, int)> dump = [&](const TokenParserItem &item, int indent) {
            if (item.type) {
                std::cout << std::string(indent * 2, ' ') << item.type << ": '" << item.value << "'" << std::endl;
            }
            for (size_t i = 0; i < item.children; ++i) {
                dump(item[i], indent + 1);
            }
        };
        for (size_t i = 0; i < tokens.count; ++i) {
            dump(tokens[i], 0);
        }
    }
    
    EXPECT_TRUE(found_subtraction) << "Failed subtraction for language: " << lang_name;
}

static void verify_negative_number(const textparser_language_definition *definition, const char *lang_name) {
    std::string test_str = "x = -1";
    if (strcmp(lang_name, "CFML") == 0) {
        test_str = "<cfset x = -1 />";
    } else if (strcmp(lang_name, "PHP") == 0) {
        test_str = "<?php $x = -1; ?>";
    } else if (strcmp(lang_name, "CSS") == 0) {
        test_str = ".class { z-index: -1; }";
    } else if (strcmp(lang_name, "Scratch") == 0) {
        test_str = "move (-1) steps"; // Scratch variables / block syntax
    } else if (strcmp(lang_name, "Perl") == 0) {
        test_str = "$x = -1;";
    }

    auto tokens = TextParser(test_str.c_str(), definition);
    
    // Find the token with value "-1" and check if it is of type "Number"
    bool found_neg_one = false;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Number") == 0 && item.value == "-1") {
            found_neg_one = true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };
    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }
    
    if (!found_neg_one) {
        std::cout << "DEBUG: " << lang_name << " negative number tokens:" << std::endl;
        std::function<void(const TokenParserItem&, int)> dump = [&](const TokenParserItem &item, int indent) {
            if (item.type) {
                std::cout << std::string(indent * 2, ' ') << item.type << ": '" << item.value << "'" << std::endl;
            }
            for (size_t i = 0; i < item.children; ++i) {
                dump(item[i], indent + 1);
            }
        };
        for (size_t i = 0; i < tokens.count; ++i) {
            dump(tokens[i], 0);
        }
    }
    
    EXPECT_TRUE(found_neg_one) << "Failed negative number for language: " << lang_name;
}

TEST(parse_Subtraction, all_languages) {
    verify_subtraction(&ada_definition, "Ada");
    verify_subtraction(&c_definition, "C");
    verify_subtraction(&cfml_definition, "CFML");
    verify_subtraction(&cpp_definition, "C++");
    verify_subtraction(&csharp_definition, "C#");
    verify_subtraction(&css_definition, "CSS");
    verify_subtraction(&fortran_definition, "Fortran");
    verify_subtraction(&go_definition, "Go");
    verify_subtraction(&java_definition, "Java");
    verify_subtraction(&javascript_definition, "JavaScript");
    verify_subtraction(&matlab_definition, "Matlab");
    verify_subtraction(&pascal_definition, "Pascal");
    verify_subtraction(&perl_definition, "Perl");
    verify_subtraction(&php_definition, "PHP");
    verify_subtraction(&python_definition, "Python");
    verify_subtraction(&r_definition, "R");
    verify_subtraction(&scratch_definition, "Scratch");
    verify_subtraction(&swift_definition, "Swift");
    verify_subtraction(&typescript_definition, "TypeScript");
    verify_subtraction(&vb_definition, "VB");
}

TEST(parse_NegativeNumbers, all_languages) {
    // Only verify languages that natively support negative signs in their Number token regex.
    // Ada, Fortran, and Pascal do not have a leading sign in their Number regexes.
    verify_negative_number(&c_definition, "C");
    verify_negative_number(&cfml_definition, "CFML");
    verify_negative_number(&cpp_definition, "C++");
    verify_negative_number(&csharp_definition, "C#");
    verify_negative_number(&css_definition, "CSS");
    verify_negative_number(&go_definition, "Go");
    verify_negative_number(&java_definition, "Java");
    verify_negative_number(&javascript_definition, "JavaScript");
    verify_negative_number(&matlab_definition, "Matlab");
    verify_negative_number(&perl_definition, "Perl");
    verify_negative_number(&php_definition, "PHP");
    verify_negative_number(&python_definition, "Python");
    verify_negative_number(&r_definition, "R");
    verify_negative_number(&scratch_definition, "Scratch");
    verify_negative_number(&swift_definition, "Swift");
    verify_negative_number(&typescript_definition, "TypeScript");
    verify_negative_number(&vb_definition, "VB");
}

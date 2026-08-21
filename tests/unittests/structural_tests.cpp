#define TEXTPARSER_ALLOW_C_HEADER_IN_CPP
#include <textparser.hpp>
#include <textparser-json.h>
#include "tokenparser.hpp"
#include <gtest/gtest.h>

#include <string>
#include <cstring>
#include <vector>

// 1. Dynamic JSON Grammar Test: C-like Declaration vs Assignment
TEST(structural_tests, c_declaration_vs_assignment_speculative_backtracking) {
    const char *grammar_json = R"json({
        "name": "minilang",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["mini"],
        "defaultTextEncoding": "latin1",
        "startTokens": ["Declaration", "Assignment", "ExpressionStatement"],
        "otherTextInside": true,
        "tokens": {
            "TypeSpecifier": {
                "type": "SimpleToken",
                "startRegex": "int\\b|float\\b|char\\b|void\\b"
            },
            "Identifier": {
                "type": "SimpleToken",
                "startRegex": "[a-zA-Z_][a-zA-Z0-9_]*"
            },
            "Equals": {
                "type": "SimpleToken",
                "startRegex": "="
            },
            "Number": {
                "type": "SimpleToken",
                "startRegex": "[0-9]+"
            },
            "Semicolon": {
                "type": "SimpleToken",
                "startRegex": ";"
            },
            "Declaration": {
                "type": "Sequence",
                "nestedTokens": ["TypeSpecifier", "Identifier", "Equals", "Number", "Semicolon"]
            },
            "Assignment": {
                "type": "Sequence",
                "nestedTokens": ["Identifier", "Equals", "Number", "Semicolon"]
            },
            "ExpressionStatement": {
                "type": "Sequence",
                "nestedTokens": ["Identifier", "Semicolon"]
            }
        }
    })json";

    textparser_language_definition *lang = nullptr;
    int err = textparser_json_load_language_definition_from_string(grammar_json, &lang);
    ASSERT_EQ(err, 0);
    ASSERT_NE(lang, nullptr);

    // Test 1: Declaration match
    {
        const char *code = "int x = 42;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "Declaration");
        EXPECT_EQ(tokens[0].children, 5u);
        EXPECT_STREQ(tokens[0][0].type, "TypeSpecifier");
        EXPECT_STREQ(tokens[0][1].type, "Identifier");
        EXPECT_STREQ(tokens[0][2].type, "Equals");
        EXPECT_STREQ(tokens[0][3].type, "Number");
        EXPECT_STREQ(tokens[0][4].type, "Semicolon");
    }

    // Test 2: Assignment match (Declaration fails on TypeSpecifier and backtracks to Assignment)
    {
        const char *code = "x = 100;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "Assignment");
        EXPECT_EQ(tokens[0].children, 4u);
        EXPECT_STREQ(tokens[0][0].type, "Identifier");
        EXPECT_STREQ(tokens[0][1].type, "Equals");
        EXPECT_STREQ(tokens[0][2].type, "Number");
        EXPECT_STREQ(tokens[0][3].type, "Semicolon");
    }

    // Test 3: ExpressionStatement match (Declaration and Assignment fail, ExpressionStatement succeeds)
    {
        const char *code = "foo;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "ExpressionStatement");
        EXPECT_EQ(tokens[0].children, 2u);
        EXPECT_STREQ(tokens[0][0].type, "Identifier");
        EXPECT_STREQ(tokens[0][1].type, "Semicolon");
    }

    // Test 4: Multiple statements in sequence
    {
        const char *code = "int a = 1; b = 2; c;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 3u);
        EXPECT_STREQ(tokens[0].type, "Declaration");
        EXPECT_STREQ(tokens[1].type, "Assignment");
        EXPECT_STREQ(tokens[2].type, "ExpressionStatement");
    }

    textparser_free_language_definition(lang);
}

// 2. Rust-style variable declaration: let [mut] x: type = val;
TEST(structural_tests, rust_style_structural_statements) {
    const char *grammar_json = R"json({
        "name": "rust_mini",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["rs"],
        "defaultTextEncoding": "latin1",
        "startTokens": ["LetMutDecl", "LetDecl", "AssignStmt"],
        "otherTextInside": true,
        "tokens": {
            "KwLet": { "type": "SimpleToken", "startRegex": "let\\b" },
            "KwMut": { "type": "SimpleToken", "startRegex": "mut\\b" },
            "Ident": { "type": "SimpleToken", "startRegex": "[a-zA-Z_][a-zA-Z0-9_]*" },
            "Colon": { "type": "SimpleToken", "startRegex": ":" },
            "TypeIdent": { "type": "SimpleToken", "startRegex": "i32\\b|u32\\b|f64\\b|bool\\b|String\\b" },
            "Equals": { "type": "SimpleToken", "startRegex": "=" },
            "Number": { "type": "SimpleToken", "startRegex": "[0-9]+" },
            "Semicolon": { "type": "SimpleToken", "startRegex": ";" },
            "LetMutDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwLet", "KwMut", "Ident", "Colon", "TypeIdent", "Equals", "Number", "Semicolon"]
            },
            "LetDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwLet", "Ident", "Colon", "TypeIdent", "Equals", "Number", "Semicolon"]
            },
            "AssignStmt": {
                "type": "Sequence",
                "nestedTokens": ["Ident", "Equals", "Number", "Semicolon"]
            }
        }
    })json";

    textparser_language_definition *lang = nullptr;
    int err = textparser_json_load_language_definition_from_string(grammar_json, &lang);
    ASSERT_EQ(err, 0);

    // LetMutDecl
    {
        const char *code = "let mut counter: i32 = 10;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "LetMutDecl");
        EXPECT_EQ(tokens[0].children, 8u);
    }

    // LetDecl (speculatively attempts LetMutDecl, fails on 'mut', backtracks, matches LetDecl)
    {
        const char *code = "let total: u32 = 500;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "LetDecl");
        EXPECT_EQ(tokens[0].children, 7u);
    }

    // AssignStmt
    {
        const char *code = "counter = 20;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "AssignStmt");
        EXPECT_EQ(tokens[0].children, 4u);
    }

    textparser_free_language_definition(lang);
}

// 3. Go-style structural statements: var x int = 10 vs x := 10
TEST(structural_tests, go_style_structural_statements) {
    const char *grammar_json = R"json({
        "name": "go_mini",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["go"],
        "defaultTextEncoding": "latin1",
        "startTokens": ["VarDecl", "ShortDecl"],
        "otherTextInside": true,
        "tokens": {
            "KwVar": { "type": "SimpleToken", "startRegex": "var\\b" },
            "Ident": { "type": "SimpleToken", "startRegex": "[a-zA-Z_][a-zA-Z0-9_]*" },
            "TypeIdent": { "type": "SimpleToken", "startRegex": "int\\b|string\\b|bool\\b" },
            "ColonEquals": { "type": "SimpleToken", "startRegex": ":=" },
            "Equals": { "type": "SimpleToken", "startRegex": "=" },
            "Number": { "type": "SimpleToken", "startRegex": "[0-9]+" },
            "VarDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwVar", "Ident", "TypeIdent", "Equals", "Number"]
            },
            "ShortDecl": {
                "type": "Sequence",
                "nestedTokens": ["Ident", "ColonEquals", "Number"]
            }
        }
    })json";

    textparser_language_definition *lang = nullptr;
    int err = textparser_json_load_language_definition_from_string(grammar_json, &lang);
    ASSERT_EQ(err, 0);

    // var x int = 10
    {
        const char *code = "var x int = 10";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "VarDecl");
        EXPECT_EQ(tokens[0].children, 5u);
    }

    // x := 42
    {
        const char *code = "x := 42";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "ShortDecl");
        EXPECT_EQ(tokens[0].children, 3u);
    }

    textparser_free_language_definition(lang);
}

// 4. TypeScript-style structural statements: const x: number = 42;
TEST(structural_tests, typescript_style_structural_statements) {
    const char *grammar_json = R"json({
        "name": "ts_mini",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["ts"],
        "defaultTextEncoding": "latin1",
        "startTokens": ["ConstTypedDecl", "ConstDecl", "AssignStmt"],
        "otherTextInside": true,
        "tokens": {
            "KwConst": { "type": "SimpleToken", "startRegex": "const\\b" },
            "Ident": { "type": "SimpleToken", "startRegex": "[a-zA-Z_][a-zA-Z0-9_]*" },
            "Colon": { "type": "SimpleToken", "startRegex": ":" },
            "TypeIdent": { "type": "SimpleToken", "startRegex": "number\\b|string\\b|boolean\\b" },
            "Equals": { "type": "SimpleToken", "startRegex": "=" },
            "Number": { "type": "SimpleToken", "startRegex": "[0-9]+" },
            "Semicolon": { "type": "SimpleToken", "startRegex": ";" },
            "ConstTypedDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwConst", "Ident", "Colon", "TypeIdent", "Equals", "Number", "Semicolon"]
            },
            "ConstDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwConst", "Ident", "Equals", "Number", "Semicolon"]
            },
            "AssignStmt": {
                "type": "Sequence",
                "nestedTokens": ["Ident", "Equals", "Number", "Semicolon"]
            }
        }
    })json";

    textparser_language_definition *lang = nullptr;
    int err = textparser_json_load_language_definition_from_string(grammar_json, &lang);
    ASSERT_EQ(err, 0);

    // const x: number = 42;
    {
        const char *code = "const x: number = 42;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "ConstTypedDecl");
        EXPECT_EQ(tokens[0].children, 7u);
    }

    // const y = 100; (speculatively tries ConstTypedDecl, fails on missing ':', backtracks to ConstDecl)
    {
        const char *code = "const y = 100;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "ConstDecl");
        EXPECT_EQ(tokens[0].children, 5u);
    }

    textparser_free_language_definition(lang);
}

// 5. Zig-style structural statements: const x: u32 = 10;
TEST(structural_tests, zig_style_structural_statements) {
    const char *grammar_json = R"json({
        "name": "zig_mini",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["zig"],
        "defaultTextEncoding": "latin1",
        "startTokens": ["ConstDecl", "VarDecl"],
        "otherTextInside": true,
        "tokens": {
            "KwConst": { "type": "SimpleToken", "startRegex": "const\\b" },
            "KwVar": { "type": "SimpleToken", "startRegex": "var\\b" },
            "Ident": { "type": "SimpleToken", "startRegex": "[a-zA-Z_][a-zA-Z0-9_]*" },
            "Colon": { "type": "SimpleToken", "startRegex": ":" },
            "TypeIdent": { "type": "SimpleToken", "startRegex": "u8\\b|u16\\b|u32\\b|u64\\b|i32\\b" },
            "Equals": { "type": "SimpleToken", "startRegex": "=" },
            "Number": { "type": "SimpleToken", "startRegex": "[0-9]+" },
            "Semicolon": { "type": "SimpleToken", "startRegex": ";" },
            "ConstDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwConst", "Ident", "Colon", "TypeIdent", "Equals", "Number", "Semicolon"]
            },
            "VarDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwVar", "Ident", "Colon", "TypeIdent", "Equals", "Number", "Semicolon"]
            }
        }
    })json";

    textparser_language_definition *lang = nullptr;
    int err = textparser_json_load_language_definition_from_string(grammar_json, &lang);
    ASSERT_EQ(err, 0);

    {
        const char *code = "const max_size: u32 = 1024;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "ConstDecl");
        EXPECT_EQ(tokens[0].children, 7u);
    }

    {
        const char *code = "var buffer_idx: u8 = 0;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "VarDecl");
        EXPECT_EQ(tokens[0].children, 7u);
    }

    textparser_free_language_definition(lang);
}

// 6. CFML Script variable declaration: var x = 10;
TEST(structural_tests, cfml_script_structural_statements) {
    const char *grammar_json = R"json({
        "name": "cfml_script_mini",
        "version": 1.0,
        "caseSensitivity": false,
        "defaultFileExtensions": ["cfc", "cfm"],
        "defaultTextEncoding": "latin1",
        "startTokens": ["VarTypedDecl", "VarDecl", "AssignStmt"],
        "otherTextInside": true,
        "tokens": {
            "KwVar": { "type": "SimpleToken", "startRegex": "var\\b" },
            "TypeIdent": { "type": "SimpleToken", "startRegex": "numeric\\b|string\\b|boolean\\b|array\\b|struct\\b" },
            "Ident": { "type": "SimpleToken", "startRegex": "[a-zA-Z_][a-zA-Z0-9_]*" },
            "Equals": { "type": "SimpleToken", "startRegex": "=" },
            "Number": { "type": "SimpleToken", "startRegex": "[0-9]+" },
            "Semicolon": { "type": "SimpleToken", "startRegex": ";" },
            "VarTypedDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwVar", "TypeIdent", "Ident", "Equals", "Number", "Semicolon"]
            },
            "VarDecl": {
                "type": "Sequence",
                "nestedTokens": ["KwVar", "Ident", "Equals", "Number", "Semicolon"]
            },
            "AssignStmt": {
                "type": "Sequence",
                "nestedTokens": ["Ident", "Equals", "Number", "Semicolon"]
            }
        }
    })json";

    textparser_language_definition *lang = nullptr;
    int err = textparser_json_load_language_definition_from_string(grammar_json, &lang);
    ASSERT_EQ(err, 0);

    // var numeric x = 10;
    {
        const char *code = "var numeric x = 10;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "VarTypedDecl");
        EXPECT_EQ(tokens[0].children, 6u);
    }

    // var y = 20; (speculatively tries VarTypedDecl, fails on non-type 'y', backtracks to VarDecl)
    {
        const char *code = "var y = 20;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "VarDecl");
        EXPECT_EQ(tokens[0].children, 5u);
    }

    // z = 30;
    {
        const char *code = "z = 30;";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "AssignStmt");
        EXPECT_EQ(tokens[0].children, 4u);
    }

    textparser_free_language_definition(lang);
}

// 7. Edge Cases: Nested Sequence, Whitespace, Incomplete sequence at EOF
TEST(structural_tests, sequence_edge_cases) {
    const char *grammar_json = R"json({
        "name": "nested_seq",
        "version": 1.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["test"],
        "defaultTextEncoding": "latin1",
        "startTokens": ["OuterSeq"],
        "otherTextInside": true,
        "tokens": {
            "A": { "type": "SimpleToken", "startRegex": "A" },
            "B": { "type": "SimpleToken", "startRegex": "B" },
            "C": { "type": "SimpleToken", "startRegex": "C" },
            "D": { "type": "SimpleToken", "startRegex": "D" },
            "InnerSeq": {
                "type": "Sequence",
                "nestedTokens": ["B", "C"]
            },
            "OuterSeq": {
                "type": "Sequence",
                "nestedTokens": ["A", "InnerSeq", "D"]
            }
        }
    })json";

    textparser_language_definition *lang = nullptr;
    int err = textparser_json_load_language_definition_from_string(grammar_json, &lang);
    ASSERT_EQ(err, 0);

    // Full nested match with internal whitespace
    {
        const char *code = "A   B   C   D";
        auto tokens = TextParser(code, lang);
        ASSERT_EQ(tokens.count, 1u);
        EXPECT_STREQ(tokens[0].type, "OuterSeq");
        EXPECT_EQ(tokens[0].children, 3u);
        EXPECT_STREQ(tokens[0][0].type, "A");
        EXPECT_STREQ(tokens[0][1].type, "InnerSeq");
        EXPECT_EQ(tokens[0][1].children, 2u);
        EXPECT_STREQ(tokens[0][1][0].type, "B");
        EXPECT_STREQ(tokens[0][1][1].type, "C");
        EXPECT_STREQ(tokens[0][2].type, "D");
    }

    // Incomplete nested sequence at EOF (A B without C D) -> fails OuterSeq gracefully
    {
        const char *code = "A B";
        auto tokens = TextParser(code, lang);
        EXPECT_EQ(tokens.count, 0u);
    }

    textparser_free_language_definition(lang);
}

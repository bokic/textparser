#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>
#include <iostream>

#include <rust_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Rust, basic_program) {
    auto tokens = TextParser(R"(
#[derive(Debug)]
pub struct Helper<'a> {
    pub name: &'a str,
}

/* A standard Rust block comment */
fn main() {
    // Print message
    let flag = true;
    let c = 'a';
    let msg = "Hello \n \"World\"!";
    if flag {
        let mut x = 42 + 2;
        let arr = [0x2a, 0];
        x = arr[0];
    }
    println!("Done!");
}
)", &rust_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("Attribute"));
    EXPECT_TRUE(found.contains("Macro"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("CharLiteral"));
    EXPECT_TRUE(found.contains("Lifetime"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Parenthesis"));
    EXPECT_TRUE(found.contains("ArrayIndex"));
}

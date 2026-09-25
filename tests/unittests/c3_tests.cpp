#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <iostream>

#include <c3_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_C3, basic_program) {
    auto tokens = TextParser(R"(
module math::utils;

import std::io;

/* A standard C3 block comment */
<* Doc comment for demo *>
fn int main() {
    // Print logic
    @require(true)
    flag := true;
    c := 'a';
    msg := "Hello \n \"World\"!";
    if flag {
        x := 42 + 2;
        arr: int[2];
        arr[0] = 0x2a;
    }
    defer {
        io::printn("Done!");
    }
    $assert(true);
    return 0;
}
)", &c3_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("DocComment"));
    EXPECT_TRUE(found.contains("Annotation"));
    EXPECT_TRUE(found.contains("CtAssert"));
    EXPECT_TRUE(found.contains("ModuleKeyword"));
    EXPECT_TRUE(found.contains("ImportKeyword"));
    EXPECT_TRUE(found.contains("FnKeyword"));
    EXPECT_TRUE(found.contains("IfKeyword"));
    EXPECT_TRUE(found.contains("TrueKeyword"));
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("CharLiteral"));
    EXPECT_TRUE(found.contains("Integer"));
    EXPECT_TRUE(found.contains("HexNumber"));
    EXPECT_TRUE(found.contains("ColonAssign"));
    EXPECT_TRUE(found.contains("Plus"));
}

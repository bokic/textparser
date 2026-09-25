#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <iostream>

#include <jai_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_JAI, basic_program) {
    auto tokens = TextParser(R"(
#import "Basic";

/* A standard JAI block comment */
main :: () {
    // Print message
    flag := true;
    msg := "Hello \n \"World\"!";
    if flag {
        x := 42 + 2;
        arr: [2] int;
        arr[0] = 0x2a;
    }
    defer {
        print("Done!\n");
    }
}
)", &jai_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("ImportDirective"));
    EXPECT_TRUE(found.contains("IfKeyword"));
    EXPECT_TRUE(found.contains("DeferKeyword"));
    EXPECT_TRUE(found.contains("IntKeyword"));
    EXPECT_TRUE(found.contains("TrueKeyword"));
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("DecNumber"));
    EXPECT_TRUE(found.contains("HexNumber"));
    EXPECT_TRUE(found.contains("ColonColon"));
    EXPECT_TRUE(found.contains("ColonEqual"));
    EXPECT_TRUE(found.contains("Plus"));
}

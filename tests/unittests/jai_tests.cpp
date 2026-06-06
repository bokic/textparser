#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>
#include <iostream>

#include <jai_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
        std::cout << "FOUND TOKEN: " << item.type << " (len: " << item.length << ", val: '" << item.value << "')" << std::endl;
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
    EXPECT_TRUE(found.contains("Directive"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Parenthesis"));
    EXPECT_TRUE(found.contains("ArrayIndex"));
}

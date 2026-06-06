#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>
#include <iostream>

#include <zig_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
        std::cout << "FOUND TOKEN: " << item.type << " (len: " << item.length << ", val: '" << item.value << "')" << std::endl;
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Zig, basic_program) {
    auto tokens = TextParser(R"(
const std = @import("std");

pub fn main() !void {
    // Print message
    const stdout = std.io.getStdOut().writer();
    var flag: bool = true;
    var c: u8 = 'a';
    var u = undefined;
    if (flag) {
        const x: u32 = 42 + 2;
        var arr = [_]u32{ 0x2a, 0 };
        _ = arr;
    }
    defer {
        stdout.print("Done!\n", .{}) catch {};
    }
    const multiline =
        \\Line 1
        \\Line 2
    ;
    _ = multiline;
}
)", &zig_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("MultiLineString"));
    EXPECT_TRUE(found.contains("Directive"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Parenthesis"));
    EXPECT_TRUE(found.contains("ArrayIndex"));
}

#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <iostream>

#include <zig_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
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

    EXPECT_TRUE(found.contains("ConstKeyword"));
    EXPECT_TRUE(found.contains("VarKeyword"));
    EXPECT_TRUE(found.contains("PubKeyword"));
    EXPECT_TRUE(found.contains("FnKeyword"));
    EXPECT_TRUE(found.contains("IfKeyword"));
    EXPECT_TRUE(found.contains("DeferKeyword"));
    EXPECT_TRUE(found.contains("CatchKeyword"));
    EXPECT_TRUE(found.contains("BuiltinIdentifier"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("CharLiteral"));
    EXPECT_TRUE(found.contains("MultiLineString"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Identifier"));
}

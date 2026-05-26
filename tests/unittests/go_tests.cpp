#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <go_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Go, basic_go_program) {
    auto tokens = TextParser(R"(
// Package comment
package main

import (
    "fmt"
    "os"
)

/*
 * Greeter greets the user.
 */
type Greeter struct {
    name string
    count int
}

func NewGreeter(name string) *Greeter {
    return &Greeter{name: name, count: 0}
}

func (g *Greeter) Greet() string {
    var msg string
    msg = "Hello, " + g.name + "!"
    g.count++
    return msg
}

func main() {
    g := NewGreeter("World")
    result := g.Greet()
    fmt.Println(result)

    raw := `line one
line two`
    _ = raw
    if true && result != "" {
        fmt.Println("done")
    }
}
)", &go_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("BacktickString"));
}

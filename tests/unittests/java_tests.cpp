#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <java_definition.json.h>

TEST(parse_Java, basic_class) {
    const char *source = R"(
package com.example;

import java.util.*;

/* This is a multi-line
   BlockComment */
@Deprecated
public class Hello {
    // A standard main method
    public static void main(String[] args) {
        boolean flag = true;
        flag = false;
        char c = '\n';
        int num = 42;
        double val = 3.14f;
        var text = null;
        int hex = 0x2f;
        int bin = 0b1010;
        long large = 1_234_567L;
        Runnable r = () -> {};
        java.util.function.Consumer<String> printer = System.out::println;
        System.out.println("Hello \n \"World\"!");
    }
}
)";

    auto tokens = TextParser(source, &java_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (tokens[i].type) {
            found.insert(tokens[i].type);
        }
    }

    EXPECT_TRUE(found.contains("PackageKeyword"));
    EXPECT_TRUE(found.contains("ImportKeyword"));
    EXPECT_TRUE(found.contains("PublicKeyword"));
    EXPECT_TRUE(found.contains("ClassKeyword"));
    EXPECT_TRUE(found.contains("StaticKeyword"));
    EXPECT_TRUE(found.contains("VarKeyword"));
    EXPECT_TRUE(found.contains("NullKeyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("CharacterLiteral"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("At"));
    EXPECT_TRUE(found.contains("Arrow"));
    EXPECT_TRUE(found.contains("DoubleColon"));
    EXPECT_TRUE(found.contains("Identifier"));
}

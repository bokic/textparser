#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <typescript_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_TypeScript, basic_typescript_program) {
    auto tokens = TextParser(R"(
// TypeScript interface and class
interface User {
    readonly id: number;
    name: string;
    email?: string;
}

/* Generic class with constraint */
class AdminUser<T extends User> implements IAdmin {
    private users: T[] = [];

    public async addUser(user: T): Promise<void> {
        this.users.push(user);
    }

    public getFirst(): T | undefined {
        return this.users[0];
    }
}

type Status = 'active' | 'inactive';
const count: number = 42;
let isReady: boolean = false;
let message: string = "Hello\nWorld";
let greeting = `Hello ${name}!`;
)", &typescript_definition);

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
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("TemplateString"));
    EXPECT_TRUE(found.contains("StringEscape"));
}

TEST(parse_TypeScript, scientific_notation) {
    auto tokens = TextParser("let a = 1e-9; let b = 2e+5; let c = 1.5E-3;", &typescript_definition);

    bool found_neg = false;
    bool found_pos = false;
    bool found_upper = false;
    std::function<void(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, "Number") == 0) {
            if (item.value == "1e-9") found_neg = true;
            if (item.value == "2e+5") found_pos = true;
            if (item.value == "1.5E-3") found_upper = true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            scan(item[i]);
        }
    };
    for (size_t i = 0; i < tokens.count; ++i) {
        scan(tokens[i]);
    }
    EXPECT_TRUE(found_neg);
    EXPECT_TRUE(found_pos);
    EXPECT_TRUE(found_upper);
}

TEST(parse_TypeScript, regex_vs_division_disambiguation) {
    // 1. Regex literal assignment
    {
        auto tokens = TextParser("const pattern: RegExp = /^[a-z]+$/i;", &typescript_definition);
        bool found_regex = false;
        for (size_t i = 0; i < tokens.count; ++i) {
            if (tokens[i].type && strcmp(tokens[i].type, "Regex") == 0) {
                found_regex = true;
                EXPECT_EQ(tokens[i].value, "/^[a-z]+$/i");
            }
        }
        EXPECT_TRUE(found_regex);
    }

    // 2. Division expression
    {
        auto tokens = TextParser("const result: number = x / y / z;", &typescript_definition);
        bool found_regex = false;
        int div_count = 0;
        for (size_t i = 0; i < tokens.count; ++i) {
            if (tokens[i].type && strcmp(tokens[i].type, "Regex") == 0) found_regex = true;
            if (tokens[i].type && strcmp(tokens[i].type, "Operator") == 0 && tokens[i].value == "/") div_count++;
        }
        EXPECT_FALSE(found_regex);
        EXPECT_EQ(div_count, 2);
    }
}


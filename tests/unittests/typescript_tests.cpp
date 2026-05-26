#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
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
let message: string = "Hello World";
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
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("TemplateString"));
}

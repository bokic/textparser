#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <sql_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_SQL, basic_sql_program) {
    auto tokens = TextParser(R"(
-- SQL main table definition
CREATE TABLE `users` (
    id INT PRIMARY KEY,
    [name] VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE,
    is_active BOOLEAN DEFAULT TRUE
);

/* Main query block */
select u.id, u.[name], u.email
from `users` as u
where u.id > 100 AND u.is_active = true AND u.[name] = 'John' AND u.email != "john@example.com";
)", &sql_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("CreateKeyword"));
    EXPECT_TRUE(found.contains("TableKeyword"));
    EXPECT_TRUE(found.contains("SelectKeyword"));
    EXPECT_TRUE(found.contains("TrueKeyword"));
    EXPECT_TRUE(found.contains("IntKeyword"));
    EXPECT_TRUE(found.contains("BacktickIdentifier"));
    EXPECT_TRUE(found.contains("BracketIdentifier"));
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("Greater"));
    EXPECT_TRUE(found.contains("NotEqual"));
}


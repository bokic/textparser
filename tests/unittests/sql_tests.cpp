#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
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
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("DataType"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("BacktickIdentifier"));
    EXPECT_TRUE(found.contains("BracketIdentifier"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("Number"));
}


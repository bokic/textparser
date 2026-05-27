#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <python_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Python, basic_python_program) {
    auto tokens = TextParser(R"(
# Python class with typing
from dataclasses import dataclass
from typing import Optional


@dataclass
class Greeter:
    name: str
    count: int = 0
    enabled: bool = True

    def greet(self) -> str:
        """Return a greeting message."""
        self.count += 1
        return f"Hello, {self.name}!\n"


def main() -> None:
    g = Greeter(name='World')
    result = g.greet()
    print(result)
    flag = False
    doc = '''this is a triple single quote string'''


if __name__ == '__main__':
    main()

x = "hello"
)", &python_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("TripleSingleString"));
    EXPECT_TRUE(found.contains("TripleDoubleString"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("FString"));
    EXPECT_TRUE(found.contains("FStringInterpolation"));
    EXPECT_TRUE(found.contains("StringEscape"));
}

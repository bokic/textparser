#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <bash_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Bash, basic_bash_program) {
    auto tokens = TextParser(R"(
#!/bin/bash
# A simple bash script
function run_build() {
    local target=$1
    local verbose=true
    local count=100
    if [ $? -eq 0 ]; then
        echo -e "Building target: \"${target}\"\n"
        echo 'Build has started...\t'
    else
        echo "Build failed with exit code \$?"
    fi
}
)", &bash_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
}

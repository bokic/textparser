#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <scratch_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Scratch, basic_scratch_program) {
    auto tokens = TextParser(R"(
# Scratch cat animation
when green flag clicked
set size to (100)
show
forever
    move (10) steps
    if touching (mouse-pointer) then
        say Hello for 2 seconds
    else
        move (-10) steps
    end
    wait 0.5 seconds
end

when this sprite clicked
broadcast message1
)", &scratch_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Parenthesis"));
    EXPECT_TRUE(found.contains("Variable"));
}

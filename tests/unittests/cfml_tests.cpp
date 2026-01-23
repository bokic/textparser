#include "tokenparser.hpp"
#include <textparser.h>

#include <gtest/gtest.h>

#include <cfml_definition.json.h>


TEST(parse_CFML, basic_cfset) {
    auto tokens = TextParser(R"(<cfset a = 1234 />)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "StartTag");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   18);
    EXPECT_EQ   (tokens[0].children, 1);

    EXPECT_STREQ(tokens[0][0].type, "Expression");
    EXPECT_EQ   (tokens[0][0].position, 7);
    EXPECT_EQ   (tokens[0][0].length,   9);
    EXPECT_EQ   (tokens[0][0].children, 3);

    EXPECT_STREQ(tokens[0][0][0].type, "Variable");
    EXPECT_EQ   (tokens[0][0][0].position, 7);
    EXPECT_EQ   (tokens[0][0][0].length,   1);
    EXPECT_EQ   (tokens[0][0][0].children, 0);

    EXPECT_STREQ(tokens[0][0][1].type, "Operator");
    EXPECT_EQ   (tokens[0][0][1].position, 9);
    EXPECT_EQ   (tokens[0][0][1].length,   1);
    EXPECT_EQ   (tokens[0][0][1].children, 0);

    EXPECT_STREQ(tokens[0][0][2].type, "Number");
    EXPECT_EQ   (tokens[0][0][2].position, 11);
    EXPECT_EQ   (tokens[0][0][2].length,   4);
    EXPECT_EQ   (tokens[0][0][2].children, 0);
}

#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>

#include <json_definition.json.h>


TEST(parse_JSON, empty_array) {
    auto tokens = TextParser(R"([])", &json_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "Array");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   2);
    EXPECT_EQ   (tokens[0].children, 0);
}

TEST(parse_JSON, empty_object) {
    auto tokens = TextParser(R"({})", &json_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "Object");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   2);
    EXPECT_EQ   (tokens[0].children, 0);
}

TEST(parse_JSON, simple_array) {
    auto tokens = TextParser(R"([1,2,3])", &json_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].type, "Array");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   7);
    EXPECT_EQ   (tokens[0].children, 5);

    EXPECT_STREQ(tokens[0][0].type, "Number");
    EXPECT_EQ   (tokens[0][0].position, 1);
    EXPECT_EQ   (tokens[0][0].length,   1);
    EXPECT_EQ   (tokens[0][0].children, 0);

    EXPECT_STREQ(tokens[0][1].type, "ValueSeparator");
    EXPECT_EQ   (tokens[0][1].position, 2);
    EXPECT_EQ   (tokens[0][1].length,   1);
    EXPECT_EQ   (tokens[0][1].children, 0);

    EXPECT_STREQ(tokens[0][2].type, "Number");
    EXPECT_EQ   (tokens[0][2].position, 3);
    EXPECT_EQ   (tokens[0][2].length,   1);
    EXPECT_EQ   (tokens[0][2].children, 0);

    EXPECT_STREQ(tokens[0][3].type, "ValueSeparator");
    EXPECT_EQ   (tokens[0][3].position, 4);
    EXPECT_EQ   (tokens[0][3].length,   1);
    EXPECT_EQ   (tokens[0][3].children, 0);

    EXPECT_STREQ(tokens[0][4].type, "Number");
    EXPECT_EQ   (tokens[0][4].position, 5);
    EXPECT_EQ   (tokens[0][4].length,   1);
    EXPECT_EQ   (tokens[0][4].children, 0);
}

TEST(parse_JSON, simple_object) {
    auto tokens = TextParser(R"({"key": "value"})", &json_definition);
    EXPECT_EQ(tokens.count, 1);
}

#include <gtest/gtest.h>
#include <textparser.h>

#include <json_definition.json.h>
#include <cfml_definition.json.h>


static int parse(const textparser_language_definition &definition, const char *code)
{
    textparser_defer(mem);
    textparser_openmem(code, strlen(code), definition.default_text_encoding, &mem);
    return textparser_parse(mem, &definition);
}

static int parse_json(const char *code)
{
    return parse(json_definition, code);
}

static int parse_cfml(const char *code)
{
    return parse(cfml_definition, code);
}

TEST(parse_JSON, basic) {
    EXPECT_EQ(parse_json(R"()"), 0);
    EXPECT_NE(parse_json(R"([)"), 0);
    EXPECT_EQ(parse_json(R"(])"), 0);
    EXPECT_NE(parse_json(R"({)"), 0);
    EXPECT_EQ(parse_json(R"(})"), 0);
    EXPECT_EQ(parse_json(R"({})"), 0);
    EXPECT_EQ(parse_json(R"([1])"), 0);
}

TEST(parse_CFML, basic) {
    EXPECT_EQ(parse_cfml(R"(<cfset a = "#a#" />)"), 0);
}

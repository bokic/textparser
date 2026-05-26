#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <css_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_CSS, basic_css_program) {
    auto tokens = TextParser(R"(
/* Main Stylesheet */
@media (max-width: 768px) {
    body {
        margin: 0;
        padding: 10px 20px;
    }
}

#app-container .card > a:hover {
    background-color: #ffffff;
    font-family: 'Helvetica Neue', Arial, sans-serif;
    width: calc(100% - 2rem);
    display: flex !important;
}
)", &css_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("AtRule"));
    EXPECT_TRUE(found.contains("IdName"));
    EXPECT_TRUE(found.contains("ClassName"));
    EXPECT_TRUE(found.contains("PseudoClass"));
    EXPECT_TRUE(found.contains("TagName"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Declaration"));
    EXPECT_TRUE(found.contains("HexColor"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("FunctionCall"));
    EXPECT_TRUE(found.contains("Important"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Value"));
}

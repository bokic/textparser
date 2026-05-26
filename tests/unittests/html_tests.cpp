#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <html_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_HTML, basic_html_program) {
    auto tokens = TextParser(R"(
<!DOCTYPE html>
<!-- HTML Main Document -->
<html lang="en">
<head>
    <meta charset="utf-8" />
    <title>Hello HTML</title>
</head>
<body>
    <div class='container' id="app">
        <h1>Welcome to AntiGravity Code!</h1>
        <img src="logo.png" alt='AntiGravity Logo' />
    </div>
</body>
</html>
)", &html_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Doctype"));
    EXPECT_TRUE(found.contains("Comment"));
    EXPECT_TRUE(found.contains("Tag"));
    EXPECT_TRUE(found.contains("ClosingTag"));
    EXPECT_TRUE(found.contains("AttributeName"));
    EXPECT_TRUE(found.contains("Equal"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("SingleString"));
}

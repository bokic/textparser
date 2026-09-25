#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <matlab_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_MATLAB, basic_matlab_program) {
    auto tokens = TextParser(R"(
% MATLAB function example
function result = compute_stats(data)
    % Compute mean and standard deviation
    n = length(data);
    if n == 0
        result = struct('mean', NaN, 'std', NaN);
        return
    end

    total = sum(data);
    mean_val = total / n;

    % Block comment example
    sq_diff = sum((data - mean_val).^2);
    std_val = sqrt(sq_diff / (n - 1));

    result.mean = mean_val;
    result.std = std_val;
end
)", &matlab_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("KwFunction"));
    EXPECT_TRUE(found.contains("KwIf"));
    EXPECT_TRUE(found.contains("KwReturn"));
    EXPECT_TRUE(found.contains("KwEnd"));
    EXPECT_TRUE(found.contains("DecNumber"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DotPower"));
    EXPECT_TRUE(found.contains("Eq"));
    EXPECT_TRUE(found.contains("Ident"));
}

TEST(parse_MATLAB, unary_vs_binary_subtraction) {
    {
        auto tokens = TextParser("x-1", &matlab_definition);
        ASSERT_EQ(tokens.count, 3);
        EXPECT_STREQ(tokens[0].type, "Ident");
        EXPECT_STREQ(tokens[0].value.c_str(), "x");
        EXPECT_STREQ(tokens[1].type, "Minus");
        EXPECT_STREQ(tokens[1].value.c_str(), "-");
        EXPECT_STREQ(tokens[2].type, "DecNumber");
        EXPECT_STREQ(tokens[2].value.c_str(), "1");
    }
    {
        auto tokens = TextParser("10-10", &matlab_definition);
        ASSERT_EQ(tokens.count, 3);
        EXPECT_STREQ(tokens[0].type, "DecNumber");
        EXPECT_STREQ(tokens[0].value.c_str(), "10");
        EXPECT_STREQ(tokens[1].type, "Minus");
        EXPECT_STREQ(tokens[1].value.c_str(), "-");
        EXPECT_STREQ(tokens[2].type, "DecNumber");
        EXPECT_STREQ(tokens[2].value.c_str(), "10");
    }
    {
        auto tokens = TextParser("x = -1", &matlab_definition);
        ASSERT_EQ(tokens.count, 4);
        EXPECT_STREQ(tokens[0].type, "Ident");
        EXPECT_STREQ(tokens[0].value.c_str(), "x");
        EXPECT_STREQ(tokens[1].type, "Assign");
        EXPECT_STREQ(tokens[1].value.c_str(), "=");
        EXPECT_STREQ(tokens[2].type, "Minus");
        EXPECT_STREQ(tokens[2].value.c_str(), "-");
        EXPECT_STREQ(tokens[3].type, "DecNumber");
        EXPECT_STREQ(tokens[3].value.c_str(), "1");
    }
}


#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
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

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
}

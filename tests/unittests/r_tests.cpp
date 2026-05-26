#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <r_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_R, basic_r_program) {
    auto tokens = TextParser(R"(
# R S3 class example
#' @title Compute statistics
compute_stats <- function(data, na.rm = FALSE) {
    n <- length(data)
    if (n == 0) {
        return(NULL)
    }

    mean_val <- mean(data, na.rm = na.rm)
    std_val <- sd(data, na.rm = na.rm)

    result <- list(
        mean = mean_val,
        sd = std_val,
        n = n
    )
    class(result) <- "summary_stats"
    return(result)
}

# Generic method
print.summary_stats <- function(x, ...) {
    cat("Mean:", x$mean, "\n")
    cat("Std Dev:", x$sd, "\n")
    invisible(x)
}

data <- c(1, 2, 3, 4, 5)
stats <- compute_stats(data)
print(stats)

label <- 'result'
)", &r_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("SingleString"));
}

#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <r.h>
#include <set>
#include <string>
#include <vector>

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
    class(result) <- "summary_stats\n"
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

# Backtick identifier and escaped string
`custom identifier` <- "escaped\tvalue"

# Custom infix operator
a %custom_op% b
)", &r_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("KwFunction"));
    EXPECT_TRUE(found.contains("KwIf"));
    EXPECT_TRUE(found.contains("KwFalse"));
    EXPECT_TRUE(found.contains("KwNull"));
    EXPECT_TRUE(found.contains("DecNumber"));
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("BacktickIdentifier"));
    EXPECT_TRUE(found.contains("LeftAssign"));
    EXPECT_TRUE(found.contains("SpecialInfix"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("SingleString"));
}

struct RValidationFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/r_definition.json", &json_definition),
                      TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json_definition, nullptr);
        }
    }

    void TearDown() override {
        if (json_definition != nullptr) {
            textparser_free_language_definition(json_definition);
            json_definition = nullptr;
        }
    }

    const textparser_language_definition *get_definition() const {
        return GetParam() ? json_definition : &r_definition;
    }

    std::vector<std::string> validate(const char *r_content) {
        std::vector<std::string> messages;
        textparser_t handle = nullptr;
        int res = textparser_openmem(r_content, strlen(r_content), TEXTPARSER_ENCODING_UTF_8, &handle);
        if (res != 0) return messages;

        res = textparser_parse(handle, get_definition());
        if (res != 0) {
            textparser_close(handle);
            return messages;
        }

        res = textparser_r_register_validators(handle);
        if (res != 0) {
            textparser_close(handle);
            return messages;
        }

        textparser_match_result match = {};
        res = textparser_execute_language_grammar(handle, get_definition(), &match);
        if (res != 0) {
            textparser_close(handle);
            return messages;
        }

        textparser_validation *val = textparser_validate_r(handle);
        if (val) {
            for (int i = 0; i < val->len; i++) {
                if (val->items[i]->text) {
                    messages.push_back(val->items[i]->text);
                }
            }
            textparser_validation_clear(val);
        }
        textparser_close(handle);
        return messages;
    }

    bool has_error_code(const std::vector<std::string> &errors, const std::string &code) {
        for (const auto &err : errors) {
            if (err.find(code) != std::string::npos) return true;
        }
        return false;
    }
};

INSTANTIATE_TEST_SUITE_P(RDefinitionSources, RValidationFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(RValidationFixture, valid_r_code_produces_no_diagnostics) {
    const char *source = R"(
add <- function(a, b = 0) {
    return(a + b)
}

total <- 0
for (i in 1:10) {
    if (i == 5) next
    if (i == 9) break
    total <- total + i
}

mult <- \(x, y) x * y
val <- is.na(x)
)";
    auto diags = validate(source);
    EXPECT_TRUE(diags.empty());
}

TEST_P(RValidationFixture, duplicate_parameter_names_rejected_R2001) {
    const char *source = "f <- function(x, y, x) { x + y }";
    auto diags = validate(source);
    EXPECT_TRUE(has_error_code(diags, "R2001"));
}

TEST_P(RValidationFixture, duplicate_parameter_in_lambda_rejected_R2001) {
    const char *source = "f <- \\(a, b, a) a + b";
    auto diags = validate(source);
    EXPECT_TRUE(has_error_code(diags, "R2001"));
}

TEST_P(RValidationFixture, repeated_dots_parameter_rejected_R2002) {
    const char *source = "f <- function(x, ..., ...) 1";
    auto diags = validate(source);
    EXPECT_TRUE(has_error_code(diags, "R2002"));
}

TEST_P(RValidationFixture, break_outside_loop_rejected_R2003) {
    const char *source = "break";
    auto diags = validate(source);
    EXPECT_TRUE(has_error_code(diags, "R2003"));

    const char *func_source = "f <- function() { break }";
    auto diags2 = validate(func_source);
    EXPECT_TRUE(has_error_code(diags2, "R2003"));
}

TEST_P(RValidationFixture, next_outside_loop_rejected_R2003) {
    const char *source = "next";
    auto diags = validate(source);
    EXPECT_TRUE(has_error_code(diags, "R2003"));
}

TEST_P(RValidationFixture, break_inside_loop_accepted) {
    const char *source1 = "while (TRUE) { break }";
    EXPECT_TRUE(validate(source1).empty());

    const char *source2 = "repeat { break }";
    EXPECT_TRUE(validate(source2).empty());

    const char *source3 = "for (x in vec) { next }";
    EXPECT_TRUE(validate(source3).empty());
}

TEST_P(RValidationFixture, comparing_with_na_or_nan_warns_R2004) {
    const char *source_na = "res <- (x == NA)";
    auto diags_na = validate(source_na);
    EXPECT_TRUE(has_error_code(diags_na, "R2004"));

    const char *source_nan = "res <- (y != NaN)";
    auto diags_nan = validate(source_nan);
    EXPECT_TRUE(has_error_code(diags_nan, "R2004"));
}

TEST_P(RValidationFixture, invalid_left_hand_side_assignment_R2005) {
    const char *source_num = "123 <- 5";
    auto diags_num = validate(source_num);
    EXPECT_TRUE(has_error_code(diags_num, "R2005"));

    const char *source_str = "\"name\" <- 10";
    auto diags_str = validate(source_str);
    EXPECT_TRUE(has_error_code(diags_str, "R2005"));
}

TEST_P(RValidationFixture, invalid_right_hand_side_right_assignment_R2006) {
    const char *source = "5 -> 123";
    auto diags = validate(source);
    EXPECT_TRUE(has_error_code(diags, "R2006"));
}


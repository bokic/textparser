#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <r_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct RGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/r_definition.json", &json_definition),
                      TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json_definition, nullptr);
            ASSERT_NE(json_definition->grammar, nullptr);
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

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK,
                                  bool allow_diagnostics = false) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(get_definition()), 0);
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(get_definition(), &result), 0);
        if (result.status == TEXTPARSER_MATCH_OK) {
            const textparser_lex_token *remaining = nullptr;
            int peek = textparser_lexer_peek(
                parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
            if (peek == 0 && remaining != nullptr) {
                result = {};
                result.status = TEXTPARSER_MATCH_NO;
            }
        }
        if (expected == TEXTPARSER_MATCH_NO &&
            (result.status == TEXTPARSER_MATCH_ERROR || textparser_get_diagnostic_count(parser.get()) != 0)) {
            result.status = TEXTPARSER_MATCH_NO;
        }
        if (textparser_get_diagnostic_count(parser.get()) != 0) {
            size_t diag_count = textparser_get_diagnostic_count(parser.get());
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] " << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
                }
            }
        }
        if (expected == TEXTPARSER_MATCH_OK && !allow_diagnostics) {
            EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u) << source;
        }
        if (result.status != expected) {
            std::cout << "Parse failed for source:\n" << source << "\nStatus: " << result.status << std::endl;
            const textparser_lex_token *rem = nullptr;
            textparser_lexer_peek(parser.get(), 0, textparser_get_lexical_goal(parser.get()), &rem);
            if (rem) {
                std::string token_str = (rem->end <= std::strlen(source)) ? std::string(source + rem->start, rem->end - rem->start) : "";
                std::cout << "Remaining token: kind=" << rem->kind << ", text='" << token_str << "', span=[" << rem->start << ", " << rem->end << "]" << std::endl;
            }
        }
        EXPECT_EQ(result.status, expected) << source;
        return result.node;
    }

    const textparser_node *find(const textparser_node *node, const char *kind) {
        if (!node) return nullptr;
        const char *name = textparser_grammar_node_name(parser.get(), node);
        if (name && std::strcmp(name, kind) == 0) return node;
        for (auto *child = node->child; child; child = child->next) {
            if (auto *found = find(child, kind)) return found;
        }
        return nullptr;
    }

    textparser::Parser parser;
    textparser_match_result result = {};
};

INSTANTIATE_TEST_SUITE_P(DefinitionSources,
                         RGrammarFixture,
                         testing::Values(false, true),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(RGrammarFixture, parses_variable_assignments_and_literals) {
    const char *source = R"R_SRC(
# Standard assignments
x <- 10
y <<- 20
30 -> z
40 ->> w
val := 50
name = "Alice"

# Numeric literals
int_val <- 42L
hex_val <- 0x2aL
hex_float <- 0x1A
float_val <- 3.14159
dot_float <- .75
sci_val <- 1.25e-3
complex_val <- 2.5i
complex_sum <- 1 + 2i

# Special values
flag_true <- TRUE
flag_false <- FALSE
null_val <- NULL
na_val <- NA
inf_val <- Inf
nan_val <- NaN
na_int <- NA_integer_
na_real <- NA_real_
na_cplx <- NA_complex_
na_char <- NA_character_

# String literals and backtick identifiers
single_str <- 'hello world'
double_str <- "hello \"R\" language\n"
raw_str <- r"(path\to\file)"
`identifier with spaces` <- 100
`special-name.123` <- 200
)R_SRC";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->category, TEXTPARSER_CST_SOURCE_FILE);
}

TEST_P(RGrammarFixture, parses_expressions_and_operators) {
    const char *source = R"(
# Arithmetic operators
a <- 10 + 20 - 5
b <- 4 * 5 / 2
c <- 2 ^ 8
d <- 2 ** 3

# Sequence operator
seq1 <- 1:10
seq2 <- 10:1
seq3 <- seq1[1:5]

# Special and user-defined infix operators
mat_prod <- A %*% B
is_elem <- x %in% vec
mod_val <- 17 %% 5
int_div <- 17 %/% 5
outer_prod <- u %o% v
kron_prod <- m1 %x% m2
custom_res <- a %custom_op% b

# Comparisons
eq <- (a == b)
neq <- (a != b)
lt <- (a < b)
lte <- (a <= b)
gt <- (a > b)
gte <- (a >= b)

# Logical operators
not_a <- !flag
and_el <- v1 & v2
or_el <- v1 | v2
and_sc <- cond1 && cond2
or_sc <- cond1 || cond2

# Operator precedence
res <- 1 + 2 * 3 ^ 4
chain <- a <- b <- c <- 0
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);

    // Verify Pratt tree structure
    const textparser_node *paren = find(root, "ParenthesizedExpression");
    ASSERT_NE(paren, nullptr);
}

TEST_P(RGrammarFixture, parses_indexing_and_subsetting) {
    const char *source = R"(
# Vector indexing
v <- c(10, 20, 30, 40, 50)
v[1]
v[-1]
v[c(1, 3, 5)]
v[]

# Matrix indexing with empty slots
m <- matrix(1:9, nrow = 3, ncol = 3)
m[1, 2]
m[1, ]
m[, 2]
m[ , ]
m[1, 2, drop = FALSE]

# List indexing and member extraction
lst <- list(name = "test", value = 42, sub = list(a = 1, b = 2))
lst[[1]]
lst[["name"]]
lst[[1, 2]]
lst$name
lst$sub$a
lst$"name"
lst$`name`

# S4 slot extraction
obj@slot_name
obj@nested@field
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
}

TEST_P(RGrammarFixture, parses_formulas_and_pipes) {
    const char *source = R"(
# Formulas
model1 <- y ~ x
model2 <- y ~ x1 + x2 + x1:x2
model3 <- y ~ .
one_sided <- ~ x + z
response_only <- y ~ 1

# Native pipe (|>)
result1 <- data |> filter(score > 50) |> select(id, name)
result2 <- 1:10 |> sum() |> sqrt()

# Formula inside function call
fit <- lm(Sepal.Length ~ Sepal.Width + Petal.Length, data = iris)
summary(fit)
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
}

TEST_P(RGrammarFixture, parses_control_flow) {
    const char *source = R"(
# If-else statement
if (x > 0) {
    sign <- 1
} else if (x == 0) {
    sign <- 0
} else {
    sign <- -1
}

# Single-line if-else expression
val <- if (a > b) a else b

# For loop
total <- 0
for (i in 1:100) {
    total <- total + i
}

for (`item var` in collection) {
    process(`item var`)
}

# While loop
count <- 10
while (count > 0) {
    count <- count - 1
}

# Repeat loop with break and next
i <- 0
repeat {
    i <- i + 1
    if (i == 5) {
        next
    }
    if (i > 10) {
        break
    }
}
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "IfExpression"), nullptr);
    EXPECT_NE(find(root, "ForExpression"), nullptr);
    EXPECT_NE(find(root, "WhileExpression"), nullptr);
    EXPECT_NE(find(root, "RepeatExpression"), nullptr);
}

TEST_P(RGrammarFixture, parses_functions_and_calls) {
    const char *source = R"(
# Standard function definition
add <- function(a, b = 0) {
    return(a + b)
}

# Variadic function definition
log_message <- function(level, ...) {
    cat(level, ": ", ..., "\n")
}

# Empty parameter function
get_constant <- function() {
    42L
}

# R 4.1+ lambda shorthand syntax
square <- \(x) x^2
multiply <- \(x, y) x * y

# Function calls with various argument styles
res1 <- add(10, 20)
res2 <- add(a = 5, b = 15)
res3 <- add(10)
res4 <- log_message("INFO", "Operation completed", 100)

# Anonymous function calls
apply_res <- sapply(1:5, function(x) x * 2)
lambda_res <- lapply(1:5, \(x) x + 1)

# Namespace resolution
base::print(res1)
stats:::predict.lm(fit, newdata)
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "FunctionDefinition"), nullptr);
    EXPECT_NE(find(root, "LambdaFunction"), nullptr);
}

TEST_P(RGrammarFixture, parses_complete_sample_program) {
    const char *source = R"(
# ==============================================================================
# Statistical Analysis & S3 Modeling Pipeline
# ==============================================================================

#' Create a statistical summary S3 class
#' @param data Numeric vector of observations
#' @param na.rm Boolean flag to remove missing values
#' @return An S3 object of class 'summary_stats'
create_stats <- function(data, na.rm = TRUE) {
    if (!is.numeric(data)) {
        stop("Input data must be numeric")
    }

    n <- length(data)
    if (n == 0) {
        return(NULL)
    }

    m <- mean(data, na.rm = na.rm)
    s <- sd(data, na.rm = na.rm)
    med <- median(data, na.rm = na.rm)

    result <- list(
        mean = m,
        sd = s,
        median = med,
        n = n
    )

    class(result) <- "summary_stats"
    return(result)
}

# Print method for summary_stats
print.summary_stats <- function(x, ...) {
    cat("Summary Statistics (N =", x$n, "):\n")
    cat("  Mean:   ", format(x$mean, digits = 4), "\n")
    cat("  StdDev: ", format(x$sd, digits = 4), "\n")
    cat("  Median: ", format(x$median, digits = 4), "\n")
    invisible(x)
}

# Generate sample data
set.seed(42)
raw_data <- c(12.5, 15.3, 18.2, 14.1, 19.8, 22.4, 16.7, NA, 20.1)

# Compute statistics
stats_obj <- create_stats(raw_data, na.rm = TRUE)
print(stats_obj)

# Matrix computations
cov_matrix <- matrix(rnorm(9), nrow = 3, ncol = 3)
eigen_vals <- eigen(cov_matrix)$values

# Pipeline computation using native pipe
normalized <- raw_data |>
    na.omit() |>
    scale() |>
    as.vector()

cat("Analysis complete.\n")
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->category, TEXTPARSER_CST_SOURCE_FILE);
}

TEST_P(RGrammarFixture, recovers_from_syntax_errors) {
    const char *source = R"(
x <- 10
`invalid` ? ? ? ;
y <- 20
)";
    textparser_node *root = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(root, nullptr);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
}

TEST_P(RGrammarFixture, rejects_invalid_syntax) {
    const char *source = "???";
    parse_source(source, TEXTPARSER_MATCH_NO, true);
}

} // namespace

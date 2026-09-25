#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <set>
#include <string>
#include <vector>

#include <sql_definition.json.h>
#include <sql.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_SQL, basic_sql_program) {
    auto tokens = TextParser(R"(
-- SQL main table definition
CREATE TABLE `users` (
    id INT PRIMARY KEY,
    [name] VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE,
    is_active BOOLEAN DEFAULT TRUE
);

/* Main query block */
select u.id, u.[name], u.email
from `users` as u
where u.id > 100 AND u.is_active = true AND u.[name] = 'John' AND u.email != "john@example.com";
)", &sql_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("BlockComment"));
    EXPECT_TRUE(found.contains("CreateKeyword"));
    EXPECT_TRUE(found.contains("TableKeyword"));
    EXPECT_TRUE(found.contains("SelectKeyword"));
    EXPECT_TRUE(found.contains("TrueKeyword"));
    EXPECT_TRUE(found.contains("IntKeyword"));
    EXPECT_TRUE(found.contains("BacktickIdentifier"));
    EXPECT_TRUE(found.contains("BracketIdentifier"));
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("Greater"));
    EXPECT_TRUE(found.contains("NotEqual"));
}

struct SQLValidationFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/sql_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &sql_definition;
    }

    std::vector<std::string> validate(const char *source) {
        textparser_t handle = nullptr;
        EXPECT_EQ(textparser_openmem(source, (int)strlen(source), TEXTPARSER_ENCODING_UTF_8, &handle), 0);
        EXPECT_EQ(textparser_parse(handle, get_definition()), 0);
        EXPECT_EQ(textparser_sql_register_validators(handle), 0);
        textparser_match_result match{};
        EXPECT_EQ(textparser_execute_language_grammar(handle, get_definition(), &match), 0);

        std::vector<std::string> messages;
        textparser_validation *val = textparser_validate_sql(handle);
        if (val != nullptr) {
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, SQLValidationFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(SQLValidationFixture, valid_query_produces_no_diagnostics) {
    auto errors = validate("SELECT u.id, COUNT(*) FROM users u JOIN orders o ON u.id = o.user_id WHERE u.age >= 18 GROUP BY u.id HAVING COUNT(*) > 5;");
    EXPECT_TRUE(errors.empty());
}

TEST_P(SQLValidationFixture, aggregate_in_where_rejected_SQL2001) {
    auto err_count = validate("SELECT * FROM a WHERE COUNT(*) > 1;");
    EXPECT_TRUE(has_error_code(err_count, "SQL2001"));

    auto err_sum = validate("SELECT * FROM a WHERE SUM(x) = 100;");
    EXPECT_TRUE(has_error_code(err_sum, "SQL2001"));

    auto err_avg = validate("SELECT * FROM a WHERE AVG(price) <= 50.0;");
    EXPECT_TRUE(has_error_code(err_avg, "SQL2001"));

    auto err_min = validate("SELECT * FROM a WHERE MIN(val) >= 0;");
    EXPECT_TRUE(has_error_code(err_min, "SQL2001"));

    auto err_max = validate("SELECT * FROM a WHERE MAX(score) < 10;");
    EXPECT_TRUE(has_error_code(err_max, "SQL2001"));
}

TEST_P(SQLValidationFixture, aggregate_in_subquery_within_where_is_allowed) {
    auto errors = validate("SELECT * FROM users WHERE age > (SELECT AVG(age) FROM users);");
    EXPECT_TRUE(errors.empty());
}

TEST_P(SQLValidationFixture, nested_aggregate_functions_rejected_SQL2002) {
    auto err1 = validate("SELECT COUNT(SUM(x)) FROM a;");
    EXPECT_TRUE(has_error_code(err1, "SQL2002"));

    auto err2 = validate("SELECT AVG(COUNT(*)) FROM a;");
    EXPECT_TRUE(has_error_code(err2, "SQL2002"));

    auto err3 = validate("SELECT MIN(MAX(val)) FROM a;");
    EXPECT_TRUE(has_error_code(err3, "SQL2002"));
}

TEST_P(SQLValidationFixture, separate_aggregates_in_expression_allowed) {
    auto errors = validate("SELECT SUM(x) + COUNT(y) FROM a;");
    EXPECT_TRUE(errors.empty());
}

TEST_P(SQLValidationFixture, insert_arity_mismatch_rejected_SQL2010) {
    // 3 columns, 2 values
    auto err1 = validate("INSERT INTO users (id, name, age) VALUES (1, 'Alice');");
    EXPECT_TRUE(has_error_code(err1, "SQL2010"));

    // 1 column, 2 values
    auto err2 = validate("INSERT INTO users (id) VALUES (1, 'Alice');");
    EXPECT_TRUE(has_error_code(err2, "SQL2010"));

    // Multi-row mismatch with column list
    auto err3 = validate("INSERT INTO users (id, name) VALUES (1, 'Alice'), (2);");
    EXPECT_TRUE(has_error_code(err3, "SQL2010"));

    // Multi-row mismatch without column list (row 2 has different count than row 1)
    auto err4 = validate("INSERT INTO users VALUES (1, 'Alice'), (2);");
    EXPECT_TRUE(has_error_code(err4, "SQL2010"));

    // Correct arity
    auto clean = validate("INSERT INTO users (id, name) VALUES (1, 'Alice'), (2, 'Bob');");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, duplicate_column_in_insert_list_rejected_SQL2011) {
    auto err = validate("INSERT INTO users (id, name, id) VALUES (1, 'Alice', 2);");
    EXPECT_TRUE(has_error_code(err, "SQL2011"));

    auto clean = validate("INSERT INTO users (id, name) VALUES (1, 'Alice');");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, natural_join_with_on_or_using_rejected_SQL2016) {
    auto err_on = validate("SELECT * FROM a NATURAL JOIN b ON a.id = b.id;");
    EXPECT_TRUE(has_error_code(err_on, "SQL2016"));

    auto err_using = validate("SELECT * FROM a NATURAL JOIN b USING (id);");
    EXPECT_TRUE(has_error_code(err_using, "SQL2016"));

    auto clean = validate("SELECT * FROM a NATURAL JOIN b;");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, cross_join_with_on_rejected_SQL2017) {
    auto err = validate("SELECT * FROM a CROSS JOIN b ON a.id = b.id;");
    EXPECT_TRUE(has_error_code(err, "SQL2017"));

    auto clean = validate("SELECT * FROM a CROSS JOIN b;");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, duplicate_column_in_using_rejected_SQL2018) {
    auto err = validate("SELECT * FROM a JOIN b USING (id, name, id);");
    EXPECT_TRUE(has_error_code(err, "SQL2018"));

    auto clean = validate("SELECT * FROM a JOIN b USING (id, name);");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, duplicate_table_alias_in_from_clause_rejected_SQL2019) {
    auto err_join = validate("SELECT * FROM users u JOIN orders u ON 1=1;");
    EXPECT_TRUE(has_error_code(err_join, "SQL2019"));

    auto err_comma = validate("SELECT * FROM users a, orders a;");
    EXPECT_TRUE(has_error_code(err_comma, "SQL2019"));

    auto clean = validate("SELECT * FROM users u JOIN orders o ON u.id = o.user_id;");
    EXPECT_TRUE(clean.empty());

    // Subquery with own alias scope should not clash with outer alias
    auto clean_subquery = validate("SELECT * FROM users u JOIN (SELECT * FROM orders u) o ON u.id = o.user_id;");
    EXPECT_TRUE(clean_subquery.empty());
}

TEST_P(SQLValidationFixture, duplicate_column_in_table_definition_rejected_SQL2025) {
    auto err = validate("CREATE TABLE t (id INT, name VARCHAR(50), id INT);");
    EXPECT_TRUE(has_error_code(err, "SQL2025"));

    auto clean = validate("CREATE TABLE t (id INT, name VARCHAR(50), age INT);");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, multiple_primary_keys_rejected_SQL2026) {
    // Two column-level primary keys
    auto err_col = validate("CREATE TABLE t (id INT PRIMARY KEY, code VARCHAR(36) PRIMARY KEY);");
    EXPECT_TRUE(has_error_code(err_col, "SQL2026"));

    // Column-level and table-level primary key
    auto err_mixed = validate("CREATE TABLE t (id INT PRIMARY KEY, code VARCHAR(36), PRIMARY KEY (id));");
    EXPECT_TRUE(has_error_code(err_mixed, "SQL2026"));

    // Two table-level primary keys
    auto err_table = validate("CREATE TABLE t (id INT, code VARCHAR(36), PRIMARY KEY (id), PRIMARY KEY (code));");
    EXPECT_TRUE(has_error_code(err_table, "SQL2026"));

    // Valid single primary key
    auto clean1 = validate("CREATE TABLE t (id INT PRIMARY KEY, code VARCHAR(36));");
    EXPECT_TRUE(clean1.empty());

    // Valid composite table primary key
    auto clean2 = validate("CREATE TABLE t (id INT, code VARCHAR(36), PRIMARY KEY (id, code));");
    EXPECT_TRUE(clean2.empty());
}

TEST_P(SQLValidationFixture, multiple_auto_increment_columns_rejected_SQL2027) {
    auto err = validate("CREATE TABLE t (id INT AUTO_INCREMENT, seq INT AUTO_INCREMENT);");
    EXPECT_TRUE(has_error_code(err, "SQL2027"));

    auto clean = validate("CREATE TABLE t (id INT AUTO_INCREMENT, seq INT);");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, duplicate_constraint_name_rejected_SQL2028) {
    auto err = validate(R"(
CREATE TABLE t (
    id INT,
    c INT,
    CONSTRAINT pk PRIMARY KEY (id),
    CONSTRAINT pk UNIQUE (c)
);)");
    EXPECT_TRUE(has_error_code(err, "SQL2028"));

    auto clean = validate(R"(
CREATE TABLE t (
    id INT,
    c INT,
    CONSTRAINT pk PRIMARY KEY (id),
    CONSTRAINT uq_c UNIQUE (c)
);)");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, set_operation_column_count_mismatch_rejected_SQL2020) {
    auto err_union = validate("SELECT a, b FROM t1 UNION SELECT c FROM t2;");
    EXPECT_TRUE(has_error_code(err_union, "SQL2020"));

    auto err_intersect = validate("SELECT a, b FROM t1 INTERSECT SELECT c FROM t2;");
    EXPECT_TRUE(has_error_code(err_intersect, "SQL2020"));

    auto err_except = validate("SELECT a, b FROM t1 EXCEPT SELECT c FROM t2;");
    EXPECT_TRUE(has_error_code(err_except, "SQL2020"));

    auto clean = validate("SELECT a, b FROM t1 UNION SELECT c, d FROM t2;");
    EXPECT_TRUE(clean.empty());

    auto clean_with_order = validate("SELECT a FROM t1 UNION SELECT b FROM t2 ORDER BY a;");
    EXPECT_TRUE(clean_with_order.empty());

    auto clean_wildcard = validate("SELECT * FROM t1 UNION SELECT a FROM t2;");
    EXPECT_TRUE(clean_wildcard.empty());
}

TEST_P(SQLValidationFixture, update_without_where_rejected_SQL2013) {
    auto err = validate("UPDATE users SET name = 'Bob';");
    EXPECT_TRUE(has_error_code(err, "SQL2013"));

    // Subquery having WHERE doesn't satisfy outer UPDATE
    auto err_sub = validate("UPDATE users SET name = (SELECT name FROM src WHERE id = 1);");
    EXPECT_TRUE(has_error_code(err_sub, "SQL2013"));

    auto clean = validate("UPDATE users SET name = 'Bob' WHERE id = 1;");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, delete_without_where_rejected_SQL2014) {
    auto err = validate("DELETE FROM users;");
    EXPECT_TRUE(has_error_code(err, "SQL2014"));

    auto clean = validate("DELETE FROM users WHERE id = 1;");
    EXPECT_TRUE(clean.empty());
}

TEST_P(SQLValidationFixture, mixed_parameter_styles_rejected_SQL2030) {
    auto err_q_named = validate("SELECT * FROM users WHERE id = ? AND name = :name;");
    EXPECT_TRUE(has_error_code(err_q_named, "SQL2030"));

    auto err_pg_named = validate("SELECT * FROM users WHERE id = $1 AND name = @name;");
    EXPECT_TRUE(has_error_code(err_pg_named, "SQL2030"));

    auto clean_positional_q = validate("SELECT * FROM users WHERE id = ? AND name = ?;");
    EXPECT_TRUE(clean_positional_q.empty());

    auto clean_positional_pg = validate("SELECT * FROM users WHERE id = $1 AND name = $2;");
    EXPECT_TRUE(clean_positional_pg.empty());

    auto clean_named_colon = validate("SELECT * FROM users WHERE id = :id AND name = :name;");
    EXPECT_TRUE(clean_named_colon.empty());

    auto clean_named_at = validate("SELECT * FROM users WHERE id = @id AND name = @name;");
    EXPECT_TRUE(clean_named_at.empty());
}

TEST_P(SQLValidationFixture, constant_or_tautological_predicate_rejected_SQL2015) {
    auto err_self_num = validate("SELECT * FROM users WHERE 1 = 1;");
    EXPECT_TRUE(has_error_code(err_self_num, "SQL2015"));

    auto err_self_col = validate("SELECT * FROM users WHERE id = id;");
    EXPECT_TRUE(has_error_code(err_self_col, "SQL2015"));

    auto err_const_cmp = validate("SELECT * FROM users WHERE 1 = 0;");
    EXPECT_TRUE(has_error_code(err_const_cmp, "SQL2015"));

    auto err_const_literal = validate("SELECT * FROM users WHERE 1;");
    EXPECT_TRUE(has_error_code(err_const_literal, "SQL2015"));

    auto err_bool_literal = validate("SELECT * FROM users WHERE true;");
    EXPECT_TRUE(has_error_code(err_bool_literal, "SQL2015"));

    auto err_join_tautology = validate("SELECT * FROM a JOIN b ON a.id = a.id;");
    EXPECT_TRUE(has_error_code(err_join_tautology, "SQL2015"));

    auto clean_normal = validate("SELECT * FROM users WHERE id = 10 AND name = 'Alice';");
    EXPECT_TRUE(clean_normal.empty());

    auto clean_join = validate("SELECT * FROM a JOIN b ON a.id = b.id;");
    EXPECT_TRUE(clean_join.empty());
}




#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <sql_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct SQLGrammarFixture : testing::TestWithParam<bool> {
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

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK) {
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, SQLGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(SQLGrammarFixture, parses_simple_and_complex_select_queries) {
    const char *simple = "SELECT id, name FROM users;";
    auto *node = parse_source(simple);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SelectStatement"), nullptr);
    EXPECT_NE(find(node, "FromClause"), nullptr);

    const char *complex =
        "SELECT DISTINCT u.id, u.name, COUNT(*) AS total\n"
        "FROM users AS u\n"
        "INNER JOIN orders AS o ON u.id = o.user_id\n"
        "LEFT OUTER JOIN profiles ON u.id = profiles.user_id\n"
        "WHERE u.age >= 18 AND u.is_active = TRUE\n"
        "GROUP BY u.id, u.name\n"
        "HAVING COUNT(*) > 5\n"
        "ORDER BY total DESC, u.id ASC\n"
        "LIMIT 50 OFFSET 100;";
    node = parse_source(complex);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SelectStatement"), nullptr);
    EXPECT_NE(find(node, "JoinClause"), nullptr);
    EXPECT_NE(find(node, "WhereClause"), nullptr);
    EXPECT_NE(find(node, "GroupByClause"), nullptr);
    EXPECT_NE(find(node, "HavingClause"), nullptr);
    EXPECT_NE(find(node, "OrderByClause"), nullptr);
    EXPECT_NE(find(node, "LimitClause"), nullptr);
}

TEST_P(SQLGrammarFixture, parses_cte_with_queries_and_set_operations) {
    const char *cte =
        "WITH regional_sales AS (\n"
        "    SELECT region, SUM(amount) AS total_sales\n"
        "    FROM orders\n"
        "    GROUP BY region\n"
        ")\n"
        "SELECT region FROM regional_sales WHERE total_sales > 1000;";
    auto *node = parse_source(cte);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "WithClause"), nullptr);
    EXPECT_NE(find(node, "CommonTableExpression"), nullptr);

    const char *setOp =
        "SELECT id FROM active_users\n"
        "UNION ALL\n"
        "SELECT id FROM archived_users;";
    node = parse_source(setOp);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SetOperation"), nullptr);
}

TEST_P(SQLGrammarFixture, parses_insert_update_delete_statements) {
    const char *insertValues =
        "INSERT INTO users (id, name, email)\n"
        "VALUES (1, 'Alice', 'alice@example.com'),\n"
        "       (2, 'Bob', 'bob@example.com');";
    auto *node = parse_source(insertValues);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InsertStatement"), nullptr);
    EXPECT_NE(find(node, "ValuesClause"), nullptr);

    const char *insertSelect =
        "INSERT INTO audit_log (user_id, action)\n"
        "SELECT id, 'registered' FROM users WHERE created_at > 1000;";
    node = parse_source(insertSelect);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InsertStatement"), nullptr);

    const char *update =
        "UPDATE users\n"
        "SET name = 'Charlie', status = 1\n"
        "WHERE id = 42;";
    node = parse_source(update);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "UpdateStatement"), nullptr);
    EXPECT_NE(find(node, "Assignment"), nullptr);

    const char *del = "DELETE FROM users WHERE is_active = FALSE;";
    node = parse_source(del);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DeleteStatement"), nullptr);
}

TEST_P(SQLGrammarFixture, parses_create_alter_drop_table_ddl) {
    const char *createTable =
        "CREATE TABLE IF NOT EXISTS users (\n"
        "    id INT PRIMARY KEY,\n"
        "    name VARCHAR(100) NOT NULL,\n"
        "    email VARCHAR(255) UNIQUE,\n"
        "    balance DECIMAL(10, 2) DEFAULT 0.0,\n"
        "    department_id INT REFERENCES departments (id),\n"
        "    CONSTRAINT fk_user_dept FOREIGN KEY (department_id) REFERENCES departments (id)\n"
        ");";
    auto *node = parse_source(createTable);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CreateTableStatement"), nullptr);
    EXPECT_NE(find(node, "ColumnDefinition"), nullptr);
    EXPECT_NE(find(node, "TableConstraint"), nullptr);

    const char *alterTable =
        "ALTER TABLE users\n"
        "ADD COLUMN phone VARCHAR(20),\n"
        "DROP COLUMN legacy_status;";
    node = parse_source(alterTable);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AlterTableStatement"), nullptr);
    EXPECT_NE(find(node, "AlterAction"), nullptr);

    const char *dropTable = "DROP TABLE IF EXISTS users, temp_users CASCADE;";
    node = parse_source(dropTable);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DropTableStatement"), nullptr);
}

TEST_P(SQLGrammarFixture, parses_view_index_transaction_and_truncate) {
    const char *view =
        "CREATE VIEW active_users AS\n"
        "SELECT id, name FROM users WHERE is_active = TRUE;";
    auto *node = parse_source(view);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CreateViewStatement"), nullptr);

    const char *dropView = "DROP VIEW IF EXISTS active_users;";
    node = parse_source(dropView);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DropViewStatement"), nullptr);

    const char *index = "CREATE UNIQUE INDEX idx_user_email ON users (email);";
    node = parse_source(index);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CreateIndexStatement"), nullptr);

    const char *dropIndex = "DROP INDEX IF EXISTS idx_user_email;";
    node = parse_source(dropIndex);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DropIndexStatement"), nullptr);

    const char *tx = "BEGIN TRANSACTION; COMMIT; ROLLBACK;";
    node = parse_source(tx);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TransactionStatement"), nullptr);

    const char *trunc = "TRUNCATE TABLE session_logs;";
    node = parse_source(trunc);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TruncateStatement"), nullptr);
}

TEST_P(SQLGrammarFixture, parses_expressions_predicates_and_case) {
    const char *predicates =
        "SELECT id FROM items\n"
        "WHERE price BETWEEN 10 AND 50\n"
        "  AND category IN ('electronics', 'books')\n"
        "  AND description LIKE '%sale%'\n"
        "  AND deleted_at IS NULL\n"
        "  AND status NOT IN (3, 4)\n"
        "  AND tag NOT LIKE '%obsolete%';";
    auto *node = parse_source(predicates);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "BetweenPredicate"), nullptr);
    EXPECT_NE(find(node, "InPredicate"), nullptr);
    EXPECT_NE(find(node, "LikePredicate"), nullptr);
    EXPECT_NE(find(node, "IsPredicate"), nullptr);

    const char *caseExpr =
        "SELECT\n"
        "    id,\n"
        "    CASE\n"
        "        WHEN score >= 90 THEN 'A'\n"
        "        WHEN score >= 80 THEN 'B'\n"
        "        ELSE 'F'\n"
        "    END AS grade,\n"
        "    CAST(id AS VARCHAR(10)) AS str_id\n"
        "FROM students;";
    node = parse_source(caseExpr);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "CaseExpression"), nullptr);
    EXPECT_NE(find(node, "WhenClause"), nullptr);
    EXPECT_NE(find(node, "CastExpression"), nullptr);

    const char *existsExpr =
        "SELECT name FROM departments AS d\n"
        "WHERE EXISTS (SELECT 1 FROM employees WHERE department_id = d.id);";
    node = parse_source(existsExpr);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ExistsExpression"), nullptr);
}

TEST_P(SQLGrammarFixture, recovers_across_statements_at_semicolon) {
    const char *source =
        "SELECT FROM WHERE bad syntax here;\n"
        "SELECT id, name FROM users;";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SelectStatement"), nullptr);
}

} // namespace

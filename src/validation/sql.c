#include "sql.h"
#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SQL_VALIDATION_MAX_DEPTH 128

void textparser_validation_clear(textparser_validation *validation) {
    validation_clear_internal(validation);
}

EXPORT_SQL textparser_validation *textparser_validate_sql(textparser_t handle) {
    if (handle == nullptr) return nullptr;
    size_t diag_count = textparser_get_diagnostic_count(handle);
    if (diag_count == 0) return nullptr;

    textparser_validation *validation = nullptr;
    for (size_t i = 0; i < diag_count; i++) {
        textparser_diagnostic diag = {0};
        if (textparser_get_diagnostic(handle, i, &diag) == 0) {
            enum textparser_validation_item_type type = TEXTPARSER_VALIDATION_ITEM_TYPE_INFO;
            if (diag.severity == TEXTPARSER_SEVERITY_ERROR)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR;
            else if (diag.severity == TEXTPARSER_SEVERITY_WARNING)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING;

            char *msg = dynamic_printf("%s: %s", diag.code ? diag.code : "SQL", diag.message ? diag.message : "");
            textparser_validation_item_add(type, &validation, msg, diag.start_pos, diag.length);
        }
    }
    return validation;
}

static inline bool kind(const textparser_node *node, const char *name) {
    return node != nullptr && node->cst_kind != nullptr && strcmp(node->cst_kind, name) == 0;
}

static char *node_text(textparser_t handle, const textparser_node *node, size_t *length) {
    if (!node || node->source_end < node->source_start) return nullptr;
    textparser_token_item prefix = {.len = node->source_start};
    textparser_token_item slice = {.prev = &prefix, .len = node->source_end - node->source_start};
    *length = slice.len;
    uint16_t *wide16 = textparser_get_token_text16(handle, &slice);
    uint32_t *wide32 = wide16 ? nullptr : textparser_get_token_text32(handle, &slice);
    if (wide16 || wide32) {
        *length = 0;
        for (size_t i = 0; i < slice.len; ++i) {
            uint32_t cp = wide16 ? wide16[i] : wide32[i];
            if (wide16 && cp >= 0xd800 && cp <= 0xdbff && i + 1 < slice.len &&
                wide16[i + 1] >= 0xdc00 && wide16[i + 1] <= 0xdfff) {
                cp = 0x10000 + ((cp - 0xd800) << 10) + wide16[++i] - 0xdc00;
            }
            *length += cp <= 0x7f ? 1 : cp <= 0x7ff ? 2 : cp <= 0xffff ? 3 : 4;
        }
    }
    textparser_free_token_text(wide16);
    textparser_free_token_text(wide32);
    return textparser_get_token_text(handle, &slice);
}

static void report(textparser_t handle, const textparser_node *node,
                   const char *code, const char *message) {
    textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, code, message,
        node->source_start, node->source_end - node->source_start);
}

static void report_warning(textparser_t handle, const textparser_node *node,
                           const char *code, const char *message) {
    textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_WARNING, code, message,
        node->source_start, node->source_end - node->source_start);
}

static bool is_aggregate_function(const char *name) {
    if (!name) return false;
    return strcasecmp(name, "COUNT") == 0 ||
           strcasecmp(name, "SUM") == 0 ||
           strcasecmp(name, "AVG") == 0 ||
           strcasecmp(name, "MIN") == 0 ||
           strcasecmp(name, "MAX") == 0;
}

static const textparser_node *find_first_identifier_node(const textparser_node *node) {
    if (!node) return nullptr;
    if (kind(node, "Identifier") ||
        kind(node, "BacktickIdentifier") ||
        kind(node, "BracketIdentifier")) {
        return node;
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        const textparser_node *found = find_first_identifier_node(child);
        if (found) return found;
    }
    return nullptr;
}

static char *get_callee_name(textparser_t handle, const textparser_node *func_call) {
    if (!func_call || !func_call->child) return nullptr;
    const textparser_node *col_ref = func_call->child;
    if (!kind(col_ref, "ColumnRef")) return nullptr;

    const textparser_node *id_node = find_first_identifier_node(col_ref);
    if (!id_node) return nullptr;

    size_t len = 0;
    return node_text(handle, id_node, &len);
}

static bool has_child_kind(const textparser_node *node, const char *target_kind) {
    if (!node) return false;
    for (const textparser_node *child = node->child; child; child = child->next) {
        if (kind(child, target_kind)) return true;
        if (has_child_kind(child, target_kind)) return true;
    }
    return false;
}

static const textparser_node *find_child_kind(const textparser_node *node, const char *target_kind) {
    if (!node) return nullptr;
    for (const textparser_node *child = node->child; child; child = child->next) {
        if (kind(child, target_kind)) return child;
        const textparser_node *found = find_child_kind(child, target_kind);
        if (found) return found;
    }
    return nullptr;
}

static void collect_identifiers(textparser_t handle, const textparser_node *node,
                                char ***names, size_t *count, size_t *cap) {
    if (!node) return;
    if (kind(node, "Identifier") || kind(node, "BacktickIdentifier") || kind(node, "BracketIdentifier")) {
        size_t len = 0;
        char *txt = node_text(handle, node, &len);
        if (txt) {
            if (*count >= *cap) {
                *cap = (*cap == 0) ? 8 : (*cap * 2);
                *names = realloc(*names, *cap * sizeof(char *));
            }
            (*names)[(*count)++] = txt;
        }
        return;
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        collect_identifiers(handle, child, names, count, cap);
    }
}

static void check_column_name_list_duplicates(textparser_t handle, const textparser_node *col_list,
                                              const char *code, const char *clause_name) {
    char **names = nullptr;
    size_t count = 0, cap = 0;
    collect_identifiers(handle, col_list, &names, &count, &cap);
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (strcasecmp(names[i], names[j]) == 0) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Duplicate column '%s' in %s", names[i], clause_name);
                report(handle, col_list, code, msg);
                goto done;
            }
        }
    }
done:
    for (size_t i = 0; i < count; i++) {
        textparser_free_token_text(names[i]);
    }
    free(names);
}

static size_t count_columns_in_list(const textparser_node *col_list) {
    if (!col_list) return 0;
    size_t count = 0;
    for (const textparser_node *item = col_list->child; item; item = item->next) {
        if (kind(item, "LParen") || kind(item, "RParen")) continue;
        if (kind(item, "Repeat")) {
            for (const textparser_node *seq = item->child; seq; seq = seq->next) {
                count++;
            }
        } else {
            count++;
        }
    }
    return count;
}

static size_t count_expressions_in_row(const textparser_node *row_node) {
    const textparser_node *expr_list = find_child_kind(row_node, "ExpressionList");
    if (!expr_list) return 0;
    size_t count = 0;
    for (const textparser_node *item = expr_list->child; item; item = item->next) {
        if (kind(item, "Repeat")) {
            for (const textparser_node *seq = item->child; seq; seq = seq->next) {
                count++;
            }
        } else {
            count++;
        }
    }
    return count;
}

static void collect_rows(const textparser_node *node, const textparser_node **rows, size_t *count, size_t max_rows) {
    if (!node || *count >= max_rows) return;
    if (kind(node, "Row")) {
        rows[(*count)++] = node;
        return;
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        collect_rows(child, rows, count, max_rows);
    }
}

/* Phase 2: Table alias extraction */
static char *get_table_factor_alias(textparser_t handle, const textparser_node *table_factor) {
    if (!table_factor || !table_factor->child) return nullptr;
    const textparser_node *first = table_factor->child;
    const textparser_node *second = first ? first->next : nullptr;
    if (!second) return nullptr;

    const textparser_node *id_node = find_first_identifier_node(second);
    if (id_node) {
        size_t len = 0;
        return node_text(handle, id_node, &len);
    }
    return nullptr;
}

static void collect_from_clause_aliases(textparser_t handle, const textparser_node *node,
                                        char ***aliases, size_t *count, size_t *cap) {
    if (!node) return;
    if (kind(node, "SelectStatement") || kind(node, "SelectStatementCore")) {
        return;
    }
    if (kind(node, "TableFactor")) {
        char *alias = get_table_factor_alias(handle, node);
        if (alias) {
            if (*count >= *cap) {
                *cap = (*cap == 0) ? 8 : (*cap * 2);
                *aliases = realloc(*aliases, *cap * sizeof(char *));
            }
            (*aliases)[(*count)++] = alias;
        }
        return;
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        collect_from_clause_aliases(handle, child, aliases, count, cap);
    }
}

/* Phase 2: Set operation column count helpers */
static const textparser_node *find_direct_child_kind(const textparser_node *node, const char *target_kind) {
    if (!node) return nullptr;
    for (const textparser_node *child = node->child; child; child = child->next) {
        if (kind(child, target_kind)) return child;
    }
    return nullptr;
}

static bool select_item_is_wildcard(const textparser_node *item) {
    if (!item) return false;
    for (const textparser_node *c = item->child; c; c = c->next) {
        if (kind(c, "Asterisk")) return true;
        if (kind(c, "ColumnRef")) {
            for (const textparser_node *cc = c->child; cc; cc = cc->next) {
                if (kind(cc, "Asterisk")) return true;
            }
        }
    }
    return false;
}

static bool select_list_has_wildcard(const textparser_node *node) {
    if (!node) return false;
    if (kind(node, "SelectItem") && select_item_is_wildcard(node)) return true;
    for (const textparser_node *child = node->child; child; child = child->next) {
        if (select_list_has_wildcard(child)) return true;
    }
    return false;
}

static size_t count_select_items(const textparser_node *node) {
    if (!node) return 0;
    size_t count = 0;
    if (kind(node, "SelectItem")) count++;
    for (const textparser_node *child = node->child; child; child = child->next) {
        count += count_select_items(child);
    }
    return count;
}

/* Phase 2: DDL table constraint and column extraction */
static void collect_column_definitions(const textparser_node *node,
                                       const textparser_node **cols, size_t *count, size_t max_cols) {
    if (!node || *count >= max_cols) return;
    if (kind(node, "ColumnDefinition")) {
        cols[(*count)++] = node;
        return;
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        collect_column_definitions(child, cols, count, max_cols);
    }
}

static void collect_table_constraints(const textparser_node *node,
                                      const textparser_node **constraints, size_t *count, size_t max_constraints) {
    if (!node || *count >= max_constraints) return;
    if (kind(node, "TableConstraint")) {
        constraints[(*count)++] = node;
        return;
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        collect_table_constraints(child, constraints, count, max_constraints);
    }
}

static char *get_column_definition_name(textparser_t handle, const textparser_node *col_def) {
    if (!col_def || !col_def->child) return nullptr;
    const textparser_node *id_node = find_first_identifier_node(col_def->child);
    if (!id_node) id_node = find_first_identifier_node(col_def);
    if (id_node) {
        size_t len = 0;
        return node_text(handle, id_node, &len);
    }
    return nullptr;
}

static char *get_table_constraint_name(textparser_t handle, const textparser_node *constraint_node) {
    if (!constraint_node) return nullptr;
    for (const textparser_node *item = constraint_node->child; item; item = item->next) {
        if (has_child_kind(item, "ConstraintKeyword")) {
            for (const textparser_node *c = item->child; c; c = c->next) {
                if (kind(c, "ConstraintKeyword")) {
                    const textparser_node *name_node = find_first_identifier_node(c->next);
                    if (name_node) {
                        size_t len = 0;
                        return node_text(handle, name_node, &len);
                    }
                }
            }
        }
    }
    return nullptr;
}

static void check_create_table(textparser_t handle, const textparser_node *create_table_node) {
    const textparser_node *elem_list = find_child_kind(create_table_node, "TableElementList");
    if (!elem_list) return;

    const textparser_node *col_defs[128];
    size_t col_count = 0;
    collect_column_definitions(elem_list, col_defs, &col_count, 128);

    const textparser_node *constraints[128];
    size_t constraint_count = 0;
    collect_table_constraints(elem_list, constraints, &constraint_count, 128);

    // 1. Check duplicate column names (SQL2025)
    char *col_names[128] = {0};
    for (size_t i = 0; i < col_count; i++) {
        col_names[i] = get_column_definition_name(handle, col_defs[i]);
    }
    for (size_t i = 0; i < col_count; i++) {
        if (!col_names[i]) continue;
        for (size_t j = i + 1; j < col_count; j++) {
            if (col_names[j] && strcasecmp(col_names[i], col_names[j]) == 0) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Duplicate column '%s' in table definition", col_names[i]);
                report(handle, col_defs[j], "SQL2025", msg);
                goto done_dup_cols;
            }
        }
    }
done_dup_cols:

    // 2. Check multiple PRIMARY KEY constraints (SQL2026)
    size_t pk_count = 0;
    const textparser_node *second_pk_node = nullptr;
    for (size_t i = 0; i < col_count; i++) {
        if (has_child_kind(col_defs[i], "PrimaryKeyword")) {
            pk_count++;
            if (pk_count == 2) second_pk_node = col_defs[i];
        }
    }
    for (size_t i = 0; i < constraint_count; i++) {
        if (has_child_kind(constraints[i], "PrimaryKeyword")) {
            pk_count++;
            if (pk_count == 2) second_pk_node = constraints[i];
        }
    }
    if (pk_count > 1 && second_pk_node) {
        report(handle, second_pk_node, "SQL2026", "Multiple PRIMARY KEY constraints are not allowed");
    }

    // 3. Check multiple AUTO_INCREMENT columns (SQL2027)
    size_t auto_inc_count = 0;
    const textparser_node *second_auto_inc = nullptr;
    for (size_t i = 0; i < col_count; i++) {
        if (has_child_kind(col_defs[i], "AutoIncrementKeyword")) {
            auto_inc_count++;
            if (auto_inc_count == 2) second_auto_inc = col_defs[i];
        }
    }
    if (auto_inc_count > 1 && second_auto_inc) {
        report(handle, second_auto_inc, "SQL2027", "Table can have only one AUTO_INCREMENT column");
    }

    // 4. Check duplicate constraint names (SQL2028)
    char *c_names[128] = {0};
    for (size_t i = 0; i < constraint_count; i++) {
        c_names[i] = get_table_constraint_name(handle, constraints[i]);
    }
    for (size_t i = 0; i < constraint_count; i++) {
        if (!c_names[i]) continue;
        for (size_t j = i + 1; j < constraint_count; j++) {
            if (c_names[j] && strcasecmp(c_names[i], c_names[j]) == 0) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Duplicate constraint name '%s' in table definition", c_names[i]);
                report(handle, constraints[j], "SQL2028", msg);
                goto done_dup_constraints;
            }
        }
    }
done_dup_constraints:

    for (size_t i = 0; i < col_count; i++) {
        if (col_names[i]) textparser_free_token_text(col_names[i]);
    }
    for (size_t i = 0; i < constraint_count; i++) {
        if (c_names[i]) textparser_free_token_text(c_names[i]);
    }
}

/* Phase 3: Predicate and parameter safety helpers */
static bool is_comparison_operator(const textparser_node *node) {
    if (!node) return false;
    return kind(node, "Equal") ||
           kind(node, "NotEqual") ||
           kind(node, "Less") ||
           kind(node, "LessEqual") ||
           kind(node, "Greater") ||
           kind(node, "GreaterEqual");
}

static bool is_constant_expression(const textparser_node *node) {
    if (!node) return false;
    if (kind(node, "Number") ||
        kind(node, "SingleString") ||
        kind(node, "DoubleString") ||
        kind(node, "TrueKeyword") ||
        kind(node, "FalseKeyword") ||
        kind(node, "NullKeyword")) {
        return true;
    }
    if ((kind(node, "Plus") || kind(node, "Minus")) && node->child && !node->child->next) {
        return is_constant_expression(node->child);
    }
    if (kind(node, "ParenthesizedExpression") && node->child) {
        for (const textparser_node *c = node->child; c; c = c->next) {
            if (kind(c, "LParen") || kind(c, "RParen")) continue;
            return is_constant_expression(c);
        }
    }
    return false;
}

static void collect_parameter_styles(textparser_t handle, const textparser_node *node,
                                     bool *has_positional, bool *has_named,
                                     const textparser_node **offending_node) {
    if (!node) return;
    if (kind(node, "Variable")) {
        size_t len = 0;
        char *text = node_text(handle, node, &len);
        if (text && len > 0) {
            if (text[0] == '?' || text[0] == '$') {
                *has_positional = true;
                if (*has_named && !*offending_node) {
                    *offending_node = node;
                }
            } else if (text[0] == ':' || text[0] == '@') {
                *has_named = true;
                if (*has_positional && !*offending_node) {
                    *offending_node = node;
                }
            }
            textparser_free_token_text(text);
        }
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        collect_parameter_styles(handle, child, has_positional, has_named, offending_node);
    }
}

static void check_parameter_consistency(textparser_t handle, const textparser_node *stmt) {
    bool has_positional = false;
    bool has_named = false;
    const textparser_node *offending = nullptr;
    collect_parameter_styles(handle, stmt, &has_positional, &has_named, &offending);
    if (has_positional && has_named && offending) {
        report_warning(handle, offending, "SQL2030",
                       "Cannot mix positional (? or $N) and named (:param or @param) parameters in statement");
    }
}

typedef struct {
    bool in_where;
    bool in_predicate;
    const char *enclosing_aggregate;
} sql_validation_context;

static void trim_in_place(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

static void check_sql_node(textparser_t handle, const textparser_node *node,
                           sql_validation_context ctx, unsigned depth) {
    if (!node || depth > SQL_VALIDATION_MAX_DEPTH) return;

    if (kind(node, "Statement")) {
        check_parameter_consistency(handle, node);
    }

    if (kind(node, "UpdateStatement")) {
        if (!find_direct_child_kind(node, "WhereClause")) {
            report_warning(handle, node, "SQL2013",
                           "UPDATE statement without WHERE clause will modify all rows");
        }
    }

    if (kind(node, "DeleteStatement")) {
        if (!find_direct_child_kind(node, "WhereClause")) {
            report_warning(handle, node, "SQL2014",
                           "DELETE statement without WHERE clause will delete all rows (use TRUNCATE if intentional)");
        }
    }

    if (kind(node, "WhereClause")) {
        ctx.in_where = true;
        ctx.in_predicate = true;
        for (const textparser_node *c = node->child; c; c = c->next) {
            if (kind(c, "WhereKeyword")) continue;
            if (is_constant_expression(c)) {
                size_t len = 0;
                char *txt = node_text(handle, c, &len);
                trim_in_place(txt);
                char msg[256];
                snprintf(msg, sizeof(msg), "WHERE clause specifies constant literal '%s'", txt ? txt : "");
                report_warning(handle, c, "SQL2015", msg);
                if (txt) textparser_free_token_text(txt);
            }
            break;
        }
    } else if (kind(node, "HavingClause")) {
        ctx.in_where = false;
        ctx.in_predicate = true;
    } else if (kind(node, "JoinClause") && has_child_kind(node, "OnKeyword")) {
        ctx.in_predicate = true;
    } else if (kind(node, "SelectStatement") ||
               kind(node, "SelectStatementCore") ||
               kind(node, "ExistsExpression")) {
        ctx.in_where = false;
        ctx.in_predicate = false;
    }

    if (ctx.in_predicate && is_comparison_operator(node)) {
        const textparser_node *left = node->child;
        const textparser_node *right = left ? left->next : nullptr;
        if (left && right) {
            size_t l1 = 0, l2 = 0;
            char *left_txt = node_text(handle, left, &l1);
            char *right_txt = node_text(handle, right, &l2);
            trim_in_place(left_txt);
            trim_in_place(right_txt);
            if (left_txt && right_txt) {
                if (strcasecmp(left_txt, right_txt) == 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Self-comparison '%s' against itself in predicate is tautological or redundant",
                             left_txt);
                    report_warning(handle, node, "SQL2015", msg);
                } else if (is_constant_expression(left) && is_constant_expression(right)) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Comparison between constant literals '%s' and '%s' produces a constant predicate",
                             left_txt, right_txt);
                    report_warning(handle, node, "SQL2015", msg);
                }
            }
            if (left_txt) textparser_free_token_text(left_txt);
            if (right_txt) textparser_free_token_text(right_txt);
        }
    }

    if (kind(node, "FunctionCall")) {
        char *name = get_callee_name(handle, node);
        if (name) {
            if (is_aggregate_function(name)) {
                if (ctx.in_where) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                        "Aggregate function '%s' is not allowed in WHERE clause; use HAVING instead", name);
                    report(handle, node, "SQL2001", msg);
                }
                if (ctx.enclosing_aggregate) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                        "Nested aggregate functions are not allowed: '%s' inside '%s'",
                        name, ctx.enclosing_aggregate);
                    report(handle, node, "SQL2002", msg);
                }
            }
        }

        {
            sql_validation_context child_ctx = ctx;
            if (name && is_aggregate_function(name)) {
                child_ctx.enclosing_aggregate = name;
            }

            if (node->child) {
                check_sql_node(handle, node->child, ctx, depth + 1);
                for (const textparser_node *arg = node->child->next; arg; arg = arg->next) {
                    check_sql_node(handle, arg, child_ctx, depth + 1);
                }
            }
        }

        if (name) textparser_free_token_text(name);
        return;
    }

    if (kind(node, "FromClause")) {
        char **aliases = nullptr;
        size_t count = 0, cap = 0;
        collect_from_clause_aliases(handle, node, &aliases, &count, &cap);
        for (size_t i = 0; i < count; i++) {
            for (size_t j = i + 1; j < count; j++) {
                if (strcasecmp(aliases[i], aliases[j]) == 0) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Duplicate table alias '%s' in FROM clause", aliases[i]);
                    report(handle, node, "SQL2019", msg);
                    goto done_from_alias;
                }
            }
        }
    done_from_alias:
        for (size_t i = 0; i < count; i++) textparser_free_token_text(aliases[i]);
        free(aliases);
    }

    if (kind(node, "SelectStatementCore")) {
        const textparser_node *set_op = find_direct_child_kind(node, "SetOperation");
        if (set_op) {
            const textparser_node *right_select = find_direct_child_kind(set_op, "SelectStatementCore");
            const textparser_node *left_list = find_direct_child_kind(node, "SelectList");
            const textparser_node *right_list = right_select ? find_direct_child_kind(right_select, "SelectList") : nullptr;
            if (left_list && right_list &&
                !select_list_has_wildcard(left_list) &&
                !select_list_has_wildcard(right_list)) {
                size_t left_count = count_select_items(left_list);
                size_t right_count = count_select_items(right_list);
                if (left_count != right_count) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Each query in set operation must have the same number of columns (%zu vs %zu)",
                             left_count, right_count);
                    report(handle, set_op, "SQL2020", msg);
                }
            }
        }
    }

    if (kind(node, "CreateTableStatement")) {
        check_create_table(handle, node);
    }

    if (kind(node, "JoinClause")) {
        bool has_natural = has_child_kind(node, "NaturalKeyword");
        bool has_cross = has_child_kind(node, "CrossKeyword");
        bool has_on = has_child_kind(node, "OnKeyword");
        bool has_using = has_child_kind(node, "UsingKeyword");

        if (has_natural && (has_on || has_using)) {
            report(handle, node, "SQL2016", "NATURAL JOIN cannot specify an ON or USING clause");
        }
        if (has_cross && has_on) {
            report(handle, node, "SQL2017", "CROSS JOIN cannot specify an ON clause");
        }
        if (has_using) {
            const textparser_node *col_list = find_child_kind(node, "ColumnNameList");
            if (col_list) {
                check_column_name_list_duplicates(handle, col_list, "SQL2018", "USING clause");
            }
        }
    }

    if (kind(node, "InsertStatement")) {
        const textparser_node *col_list = find_child_kind(node, "ColumnNameList");
        size_t col_count = 0;
        if (col_list) {
            col_count = count_columns_in_list(col_list);
            check_column_name_list_duplicates(handle, col_list, "SQL2011", "INSERT column list");
        }

        const textparser_node *values_clause = find_child_kind(node, "ValuesClause");
        if (values_clause) {
            const textparser_node *rows[256];
            size_t row_count = 0;
            collect_rows(values_clause, rows, &row_count, 256);

            for (size_t r = 0; r < row_count; r++) {
                size_t val_count = count_expressions_in_row(rows[r]);
                if (col_count > 0 && val_count != col_count) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                        "INSERT statement has %zu columns but %zu values were supplied",
                        col_count, val_count);
                    report(handle, rows[r], "SQL2010", msg);
                } else if (col_count == 0 && r > 0) {
                    size_t first_row_vals = count_expressions_in_row(rows[0]);
                    if (val_count != first_row_vals) {
                        char msg[256];
                        snprintf(msg, sizeof(msg),
                            "VALUES row %zu has %zu values, but row 1 has %zu values",
                            r + 1, val_count, first_row_vals);
                        report(handle, rows[r], "SQL2010", msg);
                    }
                }
            }
        }
    }

    for (const textparser_node *child = node->child; child; child = child->next) {
        check_sql_node(handle, child, ctx, depth + 1);
    }
}

static textparser_action sql_source_complete(textparser_t handle,
                                             const textparser_event *event,
                                             void *user_data) {
    (void)user_data;
    if (event && event->node) {
        sql_validation_context ctx = {0};
        for (const textparser_node *item = event->node; item; item = item->next) {
            check_sql_node(handle, item, ctx, 0);
        }
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_SQL int textparser_sql_register_validators(textparser_t handle) {
    if (!handle) return -1;
    return textparser_register_handler(handle, "sql.legality", sql_source_complete, nullptr);
}

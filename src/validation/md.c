#include "md.h"
#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MD_MAX_SEEN_HEADINGS 512

typedef struct {
    int last_heading_level;
    int h1_count;
    char *seen_headings[MD_MAX_SEEN_HEADINGS];
    size_t seen_count;
} md_context;

void textparser_validation_clear(textparser_validation *validation) {
    validation_clear_internal(validation);
}

static inline bool is_node(textparser_t handle, const textparser_node *node, const char *name) {
    if (!node) return false;
    const char *n = textparser_grammar_node_name(handle, node);
    return n != NULL && strcmp(n, name) == 0;
}

static char *node_text(textparser_t handle, const textparser_node *node, size_t *length) {
    if (!node || node->source_end < node->source_start) return NULL;
    textparser_token_item prefix = {.len = node->source_start};
    textparser_token_item slice = {.prev = &prefix, .len = node->source_end - node->source_start};
    *length = slice.len;
    uint16_t *wide16 = textparser_get_token_text16(handle, &slice);
    uint32_t *wide32 = wide16 ? NULL : textparser_get_token_text32(handle, &slice);
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

static void validate_heading(textparser_t handle, const textparser_node *node, md_context *ctx) {
    size_t len = 0;
    char *text = node_text(handle, node, &len);
    if (!text || len == 0) {
        if (text) textparser_free_token_text(text);
        return;
    }

    size_t idx = 0;
    while (idx < len && (text[idx] == ' ' || text[idx] == '\t')) {
        idx++;
    }

    int level = 0;
    while (idx < len && text[idx] == '#') {
        level++;
        idx++;
    }

    if (level < 1 || level > 6) {
        textparser_free_token_text(text);
        return;
    }

    // MD2001: Heading level increment
    if (ctx->last_heading_level == 0) {
        if (level > 1) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "First heading in document has level %d; expected level 1", level);
            report_warning(handle, node, "MD2001", msg);
        }
    } else if (level > ctx->last_heading_level + 1) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Heading level %d follows heading level %d; expected at most level %d",
                 level, ctx->last_heading_level, ctx->last_heading_level + 1);
        report_warning(handle, node, "MD2001", msg);
    }
    ctx->last_heading_level = level;

    // MD2002: Multiple top-level H1 headings
    if (level == 1) {
        ctx->h1_count++;
        if (ctx->h1_count > 1) {
            report_warning(handle, node, "MD2002",
                           "Multiple top-level headings (H1) in document");
        }
    }

    // Extract heading title
    while (idx < len && (text[idx] == ' ' || text[idx] == '\t')) {
        idx++;
    }
    size_t end_idx = len;
    while (end_idx > idx && (text[end_idx - 1] == ' ' || text[end_idx - 1] == '\t' ||
                             text[end_idx - 1] == '\r' || text[end_idx - 1] == '\n')) {
        end_idx--;
    }

    size_t title_len = (end_idx > idx) ? (end_idx - idx) : 0;
    if (title_len > 0) {
        char title_buf[256];
        if (title_len >= sizeof(title_buf)) title_len = sizeof(title_buf) - 1;
        memcpy(title_buf, text + idx, title_len);
        title_buf[title_len] = '\0';

        // MD2006: Duplicate heading title
        for (size_t i = 0; i < ctx->seen_count; i++) {
            if (ctx->seen_headings[i] && strcasecmp(ctx->seen_headings[i], title_buf) == 0) {
                char msg[512];
                snprintf(msg, sizeof(msg), "Duplicate heading '%s' in document", title_buf);
                report_warning(handle, node, "MD2006", msg);
                break;
            }
        }
        if (ctx->seen_count < MD_MAX_SEEN_HEADINGS) {
            ctx->seen_headings[ctx->seen_count++] = strdup(title_buf);
        }
    }

    textparser_free_token_text(text);
}

static void validate_link_or_image(textparser_t handle, const textparser_node *node, bool is_image) {
    const textparser_node *link_end = NULL;
    for (const textparser_node *child = node->child; child; child = child->next) {
        if (is_node(handle, child, "Link_End")) {
            link_end = child;
            break;
        }
    }
    if (!link_end) return;

    size_t len = 0;
    char *text = node_text(handle, link_end, &len);
    if (!text || len == 0) {
        if (text) textparser_free_token_text(text);
        return;
    }

    // Link_End matches: \](?:\([^)\r\n]*\)|\[[^\]\r\n]*\])?
    if (text[0] == ']') {
        if (len == 1) {
            // Missing URL target (e.g. "[text]")
            if (!is_image) {
                report_warning(handle, node, "MD2004", "Link has no target or reference URL");
            }
        } else if (text[1] == '(') {
            // Direct destination: ](...)
            const char *close_paren = strchr(text + 2, ')');
            size_t dest_len = close_paren ? (size_t)(close_paren - (text + 2)) : (len - 2);

            bool empty = true;
            for (size_t i = 0; i < dest_len; i++) {
                if (!isspace((unsigned char)text[2 + i])) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                if (is_image) {
                    report(handle, link_end, "MD2003", "Empty image destination");
                } else {
                    report(handle, link_end, "MD2003", "Empty link destination");
                }
            }
        } else if (text[1] == '[') {
            // Reference link: ][...]
            const char *close_bracket = strchr(text + 2, ']');
            size_t ref_len = close_bracket ? (size_t)(close_bracket - (text + 2)) : (len - 2);

            bool empty = true;
            for (size_t i = 0; i < ref_len; i++) {
                if (!isspace((unsigned char)text[2 + i])) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                report(handle, link_end, "MD2003", "Empty reference link label");
            }
        }
    }

    textparser_free_token_text(text);
}

static int count_delimiter_columns(const char *text, size_t len) {
    if (!text || len == 0) return 0;
    int count = 0;
    size_t i = 0;
    while (i < len) {
        while (i < len && (text[i] == ' ' || text[i] == '\t' || text[i] == '|')) {
            i++;
        }
        if (i >= len) break;
        if (text[i] == ':') i++;
        if (i < len && text[i] == '-') {
            while (i < len && text[i] == '-') i++;
            if (i < len && text[i] == ':') i++;
            count++;
        } else {
            while (i < len && text[i] != '|') i++;
        }
    }
    return count;
}

static int count_table_row_columns(const char *text, size_t len) {
    if (!text || len == 0) return 0;
    size_t start = 0;
    while (start < len && (text[start] == ' ' || text[start] == '\t' ||
                           text[start] == '\r' || text[start] == '\n')) {
        start++;
    }
    if (start < len && text[start] == '|') start++;

    size_t end = len;
    while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' ||
                           text[end - 1] == '\r' || text[end - 1] == '\n')) {
        end--;
    }
    if (end > start && text[end - 1] == '|') end--;

    if (start >= end) return 0;

    int cols = 1;
    for (size_t i = start; i < end; i++) {
        if (text[i] == '|') cols++;
    }
    return cols;
}

static const textparser_node *find_table_delimiter(textparser_t handle, const textparser_node *node) {
    if (!node) return NULL;
    if (is_node(handle, node, "TableDelimiter")) return node;
    for (const textparser_node *c = node->child; c; c = c->next) {
        const textparser_node *found = find_table_delimiter(handle, c);
        if (found) return found;
    }
    return NULL;
}

static void find_table_rows(textparser_t handle, const textparser_node *node,
                            const textparser_node **rows, size_t *count, size_t max_rows) {
    if (!node || *count >= max_rows) return;
    if (is_node(handle, node, "TableRow")) {
        rows[(*count)++] = node;
        return;
    }
    for (const textparser_node *c = node->child; c; c = c->next) {
        find_table_rows(handle, c, rows, count, max_rows);
    }
}

static void validate_table(textparser_t handle, const textparser_node *table_node) {
    const textparser_node *delim_node = find_table_delimiter(handle, table_node);
    if (!delim_node) return;

    size_t delim_len = 0;
    char *delim_text = node_text(handle, delim_node, &delim_len);
    if (!delim_text) return;

    int expected_cols = count_delimiter_columns(delim_text, delim_len);
    textparser_free_token_text(delim_text);

    if (expected_cols <= 0) return;

    const textparser_node *rows[256];
    size_t row_count = 0;
    find_table_rows(handle, table_node, rows, &row_count, 256);

    for (size_t i = 0; i < row_count; i++) {
        size_t row_len = 0;
        char *row_text = node_text(handle, rows[i], &row_len);
        if (row_text) {
            int row_cols = count_table_row_columns(row_text, row_len);
            textparser_free_token_text(row_text);
            if (row_cols > 0 && row_cols != expected_cols) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Table row has %d columns; expected %d columns matching delimiter",
                         row_cols, expected_cols);
                report_warning(handle, rows[i], "MD2005", msg);
            }
        }
    }
}

static void validate_fenced_code_block(textparser_t handle, const textparser_node *node) {
    size_t len = 0;
    char *text = node_text(handle, node, &len);
    if (!text || len < 3) {
        if (text) textparser_free_token_text(text);
        return;
    }

    char open_delim = text[0]; // '`' or '~'
    if (open_delim != '`' && open_delim != '~') {
        textparser_free_token_text(text);
        return;
    }

    if (len == 3) {
        report_warning(handle, node, "MD2007", "Unclosed code fence");
        textparser_free_token_text(text);
        return;
    }

    // Check closing delimiter
    size_t end = len;
    while (end > 0 && (text[end - 1] == ' ' || text[end - 1] == '\t' ||
                       text[end - 1] == '\r' || text[end - 1] == '\n')) {
        end--;
    }

    if (end >= 3) {
        char close_delim = text[end - 1];
        if (close_delim == '`' || close_delim == '~') {
            if (close_delim != open_delim) {
                report_warning(handle, node, "MD2007",
                               "Mismatched code fence delimiters");
            }
        }
    }

    textparser_free_token_text(text);
}

static void check_md_node(textparser_t handle, const textparser_node *node, md_context *ctx) {
    if (!node) return;

    if (is_node(handle, node, "Heading")) {
        validate_heading(handle, node, ctx);
    } else if (is_node(handle, node, "Link")) {
        validate_link_or_image(handle, node, false);
    } else if (is_node(handle, node, "Image")) {
        validate_link_or_image(handle, node, true);
    } else if (is_node(handle, node, "Table")) {
        validate_table(handle, node);
    } else if (is_node(handle, node, "FencedCodeBlock")) {
        validate_fenced_code_block(handle, node);
    }

    for (const textparser_node *child = node->child; child; child = child->next) {
        check_md_node(handle, child, ctx);
    }
}

static void validate_md_ast(textparser_t handle, const textparser_node *root) {
    md_context ctx = {0};
    check_md_node(handle, root, &ctx);
    for (size_t i = 0; i < ctx.seen_count; i++) {
        if (ctx.seen_headings[i]) {
            free(ctx.seen_headings[i]);
            ctx.seen_headings[i] = NULL;
        }
    }
}

static textparser_action md_source_complete(textparser_t handle,
                                            const textparser_event *event,
                                            void *user_data) {
    (void)user_data;
    if (event && event->node) {
        for (const textparser_node *item = event->node; item; item = item->next) {
            validate_md_ast(handle, item);
        }
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_MD int textparser_md_register_validators(textparser_t handle) {
    if (!handle) return -1;
    return textparser_register_handler(handle, "md.legality", md_source_complete, NULL);
}

EXPORT_MD textparser_validation *textparser_validate_md(textparser_t handle) {
    if (handle == NULL) return NULL;

    size_t diag_count = textparser_get_diagnostic_count(handle);
    if (diag_count == 0) {
        // Run validation directly if not triggered by event handler
        const textparser_node *root = textparser_get_first_token(handle);
        if (root) {
            validate_md_ast(handle, root);
            diag_count = textparser_get_diagnostic_count(handle);
        }
    }

    if (diag_count == 0) return NULL;

    textparser_validation *validation = NULL;
    for (size_t i = 0; i < diag_count; i++) {
        textparser_diagnostic diag = {0};
        if (textparser_get_diagnostic(handle, i, &diag) == 0) {
            enum textparser_validation_item_type type = TEXTPARSER_VALIDATION_ITEM_TYPE_INFO;
            if (diag.severity == TEXTPARSER_SEVERITY_ERROR)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR;
            else if (diag.severity == TEXTPARSER_SEVERITY_WARNING)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING;

            char *msg = dynamic_printf("%s: %s",
                                       diag.code ? diag.code : "MD",
                                       diag.message ? diag.message : "");
            textparser_validation_item_add(type, &validation, msg, diag.start_pos, diag.length);
        }
    }
    return validation;
}

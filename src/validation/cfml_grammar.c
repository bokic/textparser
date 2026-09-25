#include "cfml.h"
#include "cfml_common.h"
#include "cfml_functions.h"
#include "cfml_tags.h"
#include "validation.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The v2 CFML grammar owns syntax (tag pairs, statements, expressions). This
 * validator ports the legacy token-tree checks that the grammar cannot express:
 * built-in function name/arity and the cfprocessingdirective position rule. */
#define CFML_VALIDATION_DEPTH 128

static bool kind(const textparser_node *node, const char *name) {
    return node != nullptr && node->cst_kind != nullptr &&
        strcmp(node->cst_kind, name) == 0;
}

/* Detached view of the explicit grammar span (see php_grammar.c). */
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

static const cfml_function_info *find_function_info(const char *name, size_t length) {
    for (int i = 0; i < cfml_function_count; ++i) {
        if (strncasecmp(name, cfml_functions[i].name, length) == 0 &&
            cfml_functions[i].name[length] == '\0')
            return &cfml_functions[i];
    }
    return nullptr;
}

/* Count Argument nodes through the Repeat/Sequence scaffolding only. */
static int argument_count(const textparser_node *node) {
    int count = 0;
    for (const textparser_node *item = node; item; item = item->next) {
        if (kind(item, "Argument")) ++count;
        else if (kind(item, "Repeat") || kind(item, "Sequence"))
            count += argument_count(item->child);
    }
    return count;
}

static int call_argument_count(const textparser_node *postfix) {
    for (const textparser_node *item = postfix->child; item; item = item->next)
        if (kind(item, "ArgumentList")) return argument_count(item->child);
    return 0;
}

/* A direct call is `Variable ( ... )`; member/index callees start with another
 * PostfixExpressionSuffix and resolve at runtime. */
static void check_call(textparser_t handle, const textparser_node *postfix) {
    const textparser_node *callee = postfix->child;
    if (!kind(callee, "Variable") || !kind(callee->next, "LParen")) return;
    size_t length = 0;
    char *text = node_text(handle, callee, &length);
    if (!text) return;
    if (length == 0 || text[0] == '$') { textparser_free_token_text(text); return; }
    const cfml_function_info *info = find_function_info(text, length);
    if (!info) {
        char message[512];
        snprintf(message, sizeof(message), "Unknown CFML function: [%s]", text);
        report(handle, callee, "CF2001", message);
    } else {
        int minimum = 0, maximum = 0;
        for (const cfml_function_parameter_info *param = info->parameters;
             param && param->name; ++param) {
            ++maximum;
            if (param->required) ++minimum;
        }
        int count = call_argument_count(postfix);
        char message[512];
        if (count < minimum) {
            snprintf(message, sizeof(message),
                "Function [%s] requires at least %d arguments, but %d were provided",
                text, minimum, count);
            report(handle, callee, "CF2002", message);
        } else if (count > maximum) {
            snprintf(message, sizeof(message),
                "Function [%s] takes at most %d arguments, but %d were provided",
                text, maximum, count);
            report(handle, callee, "CF2002", message);
        }
    }
    textparser_free_token_text(text);
}

static const char *start_tag_names[] = {
    "StartTag_Start", "SetStartTag_Start", "OutputStartTag_Start", "QueryStartTag_Start",
    "LoopStartTag_Start", "MailStartTag_Start", "SavecontentStartTag_Start",
    "ScriptStartTag_Start", "IfStartTag_Start", "ElseIfStartTag_Start", "ElseStartTag_Start",
    nullptr
};

static bool is_start_tag(const textparser_node *node) {
    if (!node || !node->cst_kind) return false;
    for (size_t i = 0; start_tag_names[i]; ++i)
        if (strcmp(node->cst_kind, start_tag_names[i]) == 0) return true;
    return false;
}

static const cfml_tag_info *find_tag_info(const char *name, size_t length) {
    for (int i = 0; i < cfml_tag_count; ++i) {
        if (strncasecmp(name, cfml_tags[i].name, length) == 0 &&
            cfml_tags[i].name[length] == '\0')
            return &cfml_tags[i];
    }
    return nullptr;
}

/* Only `cf*` tags are in the bundled table; HTML and namespaced tags are free. */
static void check_unknown_tag(textparser_t handle, const textparser_node *node) {
    size_t length = 0;
    char *text = node_text(handle, node, &length);
    if (!text) return;
    if (length >= 3 && (text[1] == 'c' || text[1] == 'C') &&
        (text[2] == 'f' || text[2] == 'F')) {
        const char *name = text + 1;
        size_t name_len = length - 1;
        if (find_tag_info(name, name_len) == nullptr) {
            char message[512];
            snprintf(message, sizeof(message), "Unknown CFML tag: [%.*s]", (int)name_len, name);
            report(handle, node, "CF2004", message);
        }
    }
    textparser_free_token_text(text);
}

/* Extract the tag name from `<cfname ...>` / `</cfname>` into a fresh buffer. */
static char *tag_name_text(textparser_t handle, const textparser_node *node, size_t *name_len) {
    size_t total = 0;
    char *text = node_text(handle, node, &total);
    if (!text || total < 3 || text[0] != '<') {
        textparser_free_token_text(text);
        return nullptr;
    }
    size_t start = 1;
    if (text[start] == '/') start++;
    size_t end = start;
    while (end < total && (isalnum((unsigned char)text[end]) || text[end] == '_' || text[end] == ':'))
        end++;
    *name_len = end - start;
    if (*name_len == 0) {
        textparser_free_token_text(text);
        return nullptr;
    }
    memmove(text, text + start, *name_len);
    text[*name_len] = '\0';
    return text;
}

typedef struct {
    const textparser_node *node;
    char *name;
    const cfml_tag_info *info;
    bool is_start;
} cfml_tag_entry;

typedef struct {
    cfml_tag_entry *items;
    size_t count;
    size_t capacity;
} cfml_tag_list;

static void tag_list_add(cfml_tag_list *list, const textparser_node *node, const char *name,
                         size_t length, bool is_start) {
    if (list->count == list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 32;
        cfml_tag_entry *items = realloc(list->items, capacity * sizeof(*items));
        if (!items) return;
        list->items = items;
        list->capacity = capacity;
    }
    char *copy = malloc(length + 1);
    if (!copy) return;
    memcpy(copy, name, length);
    copy[length] = '\0';
    list->items[list->count++] = (cfml_tag_entry){
        node, copy, find_tag_info(name, length), is_start};
}

static bool node_kind_ends_with(const textparser_node *node, const char *suffix);

static void collect_tags(textparser_t handle, const textparser_node *node, cfml_tag_list *list) {
    for (const textparser_node *item = node; item; item = item->next) {
        bool is_start = node_kind_ends_with(item, "StartTag") ||
            node_kind_ends_with(item, "StartTag_Start");
        bool is_end = node_kind_ends_with(item, "EndTag");
        if (is_start || is_end) {
            size_t length = 0;
            char *name = tag_name_text(handle, item, &length);
            if (name && length) tag_list_add(list, item, name, length, is_start);
            textparser_free_token_text(name);
        }
        if (item->child) collect_tags(handle, item->child, list);
    }
}

static bool tag_names_equal(const cfml_tag_entry *a, const cfml_tag_entry *b) {
    return strcasecmp(a->name, b->name) == 0;
}

static bool node_kind_ends_with(const textparser_node *node, const char *suffix) {
    if (!node || !node->cst_kind) return false;
    size_t name_length = strlen(node->cst_kind);
    size_t suffix_length = strlen(suffix);
    return name_length >= suffix_length &&
        strcmp(node->cst_kind + name_length - suffix_length, suffix) == 0;
}

static int compare_tag_positions(const void *left, const void *right) {
    const cfml_tag_entry *a = left;
    const cfml_tag_entry *b = right;
    if (a->node->source_start < b->node->source_start) return -1;
    if (a->node->source_start > b->node->source_start) return 1;
    return 0;
}

static void check_tag_pairing(textparser_t handle, cfml_tag_list *list) {
    if (list->count > 1) {
        qsort(list->items, list->count, sizeof(*list->items), compare_tag_positions);
        size_t unique_count = 0;
        for (size_t i = 0; i < list->count; ++i) {
            if (unique_count > 0 &&
                list->items[unique_count - 1].node->source_start == list->items[i].node->source_start &&
                list->items[unique_count - 1].is_start == list->items[i].is_start &&
                tag_names_equal(&list->items[unique_count - 1], &list->items[i])) {
                free(list->items[i].name);
                continue;
            }
            if (unique_count != i) list->items[unique_count] = list->items[i];
            ++unique_count;
        }
        list->count = unique_count;
    }
    size_t *open_tags = malloc(list->count * sizeof(*open_tags));
    if (list->count > 0 && !open_tags) return;
    size_t open_count = 0;
    char message[512];

    for (size_t i = 0; i < list->count; ++i) {
        cfml_tag_entry *entry = &list->items[i];
        if (entry->is_start) {
            if (entry->info != nullptr &&
                entry->info->end_tag_type != CFML_END_TAG_FORBIDDEN)
                open_tags[open_count++] = i;
        } else {
            if (entry->info != nullptr && entry->info->end_tag_type == CFML_END_TAG_FORBIDDEN) {
                snprintf(message, sizeof(message), "Ending tag </%s> is forbidden", entry->name);
                report(handle, entry->node, "CF2006", message);
                continue;
            }

            if (entry->info == nullptr) {
                bool found = false;
                for (size_t j = i; j-- > 0 && !found;)
                    if (list->items[j].is_start && tag_names_equal(entry, &list->items[j])) found = true;
                if (!found) {
                    snprintf(message, sizeof(message),
                        "Ending tag </%s> has no matching start tag", entry->name);
                    report(handle, entry->node, "CF2007", message);
                }
                continue;
            }

            size_t match = open_count;
            while (match > 0) {
                --match;
                if (tag_names_equal(entry, &list->items[open_tags[match]])) break;
            }
            if (open_count == 0 || !tag_names_equal(entry, &list->items[open_tags[match]])) {
                snprintf(message, sizeof(message),
                    "Ending tag </%s> has no matching start tag", entry->name);
                report(handle, entry->node, "CF2007", message);
                continue;
            }

            if (match + 1 != open_count) {
                const cfml_tag_entry *expected = &list->items[open_tags[open_count - 1]];
                snprintf(message, sizeof(message),
                    "Ending tag </%s> is out of order; expected </%s> first",
                    entry->name, expected->name);
                report(handle, entry->node, "CF2007", message);
                memmove(&open_tags[match], &open_tags[match + 1],
                    (open_count - match - 1) * sizeof(*open_tags));
                --open_count;
            } else {
                --open_count;
            }
        }
    }

    for (size_t i = 0; i < open_count; ++i) {
        cfml_tag_entry *entry = &list->items[open_tags[i]];
        if (entry->info->end_tag_type == CFML_END_TAG_REQUIRED) {
            snprintf(message, sizeof(message),
                "CFML tag [%s] requires a closing tag </%s>", entry->name, entry->name);
            report(handle, entry->node, "CF2005", message);
        }
    }
    free(open_tags);
}

static void tag_list_free(cfml_tag_list *list) {
    for (size_t i = 0; i < list->count; ++i) free(list->items[i].name);
    free(list->items);
}

static void check_directive_position(textparser_t handle, const textparser_node *node) {
    size_t length = 0;
    char *text = node_text(handle, node, &length);
    if (!text) return;
    /* Text is `<cfprocessingdirective` or `</cfprocessingdirective>`. */
    const char *name = text[1] == '/' ? text + 2 : text + 1;
    if (strncasecmp(name, "cfprocessingdirective", 21) == 0 &&
        (name[21] == '\0' || name[21] == '>' || isspace((unsigned char)name[21])) &&
        (node->source_start > 4096 || node->source_end > 4096)) {
        report(handle, node, "CF2003",
            "cfprocessingdirective should be located within first 4096 bytes of the file");
    }
    textparser_free_token_text(text);
}

static void check_nodes(textparser_t handle, const textparser_node *node, unsigned depth) {
    if (depth > CFML_VALIDATION_DEPTH) return;
    for (const textparser_node *item = node; item; item = item->next) {
        if (kind(item, "PostfixExpressionSuffix")) check_call(handle, item);
        if (is_start_tag(item)) {
            check_directive_position(handle, item);
            check_unknown_tag(handle, item);
        }
        check_nodes(handle, item->child, depth + 1);
    }
}

static textparser_action cfml_source_complete(textparser_t handle, const textparser_event *event,
                                              void *user_data) {
    (void)user_data;
    if (event && event->node) {
        check_nodes(handle, event->node, 0);
        cfml_tag_list tags = {0};
        collect_tags(handle, event->node, &tags);
        check_tag_pairing(handle, &tags);
        tag_list_free(&tags);
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_CFML int textparser_cfml_register_validators(textparser_t handle) {
    if (!handle) return -1;
    return textparser_register_handler(handle, "cfml.legality", cfml_source_complete, nullptr);
}

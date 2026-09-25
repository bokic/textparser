#include "json.h"
#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_VALIDATION_MAX_DEPTH 128

void textparser_validation_clear(textparser_validation *validation) {
    validation_clear_internal(validation);
}

EXPORT_JSON textparser_validation *textparser_validate_json(textparser_t handle) {
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

            char *msg = dynamic_printf("%s: %s", diag.code ? diag.code : "JSON", diag.message ? diag.message : "");
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

static void validate_string_literal(textparser_t handle, const textparser_node *node) {
    size_t len = 0;
    char *text = node_text(handle, node, &len);
    if (!text) return;

    if (len < 2 || text[0] != '"' || text[len - 1] != '"') {
        textparser_free_token_text(text);
        return;
    }

    for (size_t i = 1; i < len - 1; i++) {
        unsigned char c = (unsigned char)text[i];

        if (c < 0x20) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Unescaped control character (0x%02X) in string literal", c);
            report(handle, node, "JSON1002", msg);
            break;
        }

        if (c == '\\') {
            i++;
            if (i >= len - 1) {
                report(handle, node, "JSON1002", "Unterminated escape sequence at end of string");
                break;
            }

            char esc = text[i];
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' ||
                esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                continue;
            } else if (esc == 'u') {
                if (i + 4 >= len - 1) {
                    report(handle, node, "JSON1002", "Invalid \\u escape: expected 4 hexadecimal digits");
                    break;
                }
                char hex[5] = {text[i + 1], text[i + 2], text[i + 3], text[i + 4], '\0'};
                if (!isxdigit((unsigned char)hex[0]) ||
                    !isxdigit((unsigned char)hex[1]) ||
                    !isxdigit((unsigned char)hex[2]) ||
                    !isxdigit((unsigned char)hex[3])) {
                    report(handle, node, "JSON1002", "Invalid hexadecimal digit in \\u escape sequence");
                    break;
                }

                unsigned long cp = strtoul(hex, nullptr, 16);
                i += 4;

                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    if (i + 6 < len - 1 && text[i + 1] == '\\' && text[i + 2] == 'u') {
                        char low_hex[5] = {text[i + 3], text[i + 4], text[i + 5], text[i + 6], '\0'};
                        if (isxdigit((unsigned char)low_hex[0]) &&
                            isxdigit((unsigned char)low_hex[1]) &&
                            isxdigit((unsigned char)low_hex[2]) &&
                            isxdigit((unsigned char)low_hex[3])) {
                            unsigned long low_cp = strtoul(low_hex, nullptr, 16);
                            if (low_cp >= 0xDC00 && low_cp <= 0xDFFF) {
                                i += 6;
                                continue;
                            }
                        }
                    }
                    report(handle, node, "JSON1002",
                           "High surrogate in \\u escape sequence not followed by valid low surrogate");
                    break;
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    report(handle, node, "JSON1002",
                           "Isolated low surrogate in \\u escape sequence without preceding high surrogate");
                    break;
                }
            } else {
                char msg[256];
                snprintf(msg, sizeof(msg), "Invalid escape sequence '\\%c' in string", esc);
                report(handle, node, "JSON1002", msg);
                break;
            }
        }
    }

    textparser_free_token_text(text);
}

static void collect_object_members(const textparser_node *node,
                                   const textparser_node **members,
                                   size_t *count, size_t max_count) {
    if (!node || *count >= max_count) return;
    if (kind(node, "ObjectMember")) {
        members[(*count)++] = node;
        return;
    }
    for (const textparser_node *child = node->child; child; child = child->next) {
        if (kind(child, "Object") || kind(child, "Array")) continue;
        collect_object_members(child, members, count, max_count);
    }
}

static void check_json_node(textparser_t handle, const textparser_node *node, unsigned depth) {
    if (!node) return;

    if (depth > JSON_VALIDATION_MAX_DEPTH) {
        report_warning(handle, node, "JSON1004", "JSON structure exceeds maximum recommended nesting depth (128)");
        return;
    }

    if (kind(node, "String")) {
        validate_string_literal(handle, node);
    }

    if (kind(node, "Object")) {
        const textparser_node *members_node = nullptr;
        for (const textparser_node *c = node->child; c; c = c->next) {
            if (kind(c, "ObjectMembers")) {
                members_node = c;
                break;
            }
        }

        if (members_node) {
            const textparser_node *members[512];
            size_t member_count = 0;
            collect_object_members(members_node, members, &member_count, 512);

            char *keys[512] = {0};
            const textparser_node *key_nodes[512] = {0};
            for (size_t i = 0; i < member_count; i++) {
                for (const textparser_node *kc = members[i]->child; kc; kc = kc->next) {
                    if (kind(kc, "String")) {
                        size_t klen = 0;
                        keys[i] = node_text(handle, kc, &klen);
                        key_nodes[i] = kc;
                        break;
                    }
                }
            }

            for (size_t i = 0; i < member_count; i++) {
                if (!keys[i]) continue;
                for (size_t j = i + 1; j < member_count; j++) {
                    if (keys[j] && strcmp(keys[i], keys[j]) == 0) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Duplicate key %s in object", keys[i]);
                        report(handle, key_nodes[j] ? key_nodes[j] : members[j], "JSON1001", msg);
                        goto done_dup_keys;
                    }
                }
            }
        done_dup_keys:
            for (size_t i = 0; i < member_count; i++) {
                if (keys[i]) textparser_free_token_text(keys[i]);
            }
        }
    }

    for (const textparser_node *child = node->child; child; child = child->next) {
        check_json_node(handle, child, depth + 1);
    }
}

static textparser_action json_source_complete(textparser_t handle,
                                              const textparser_event *event,
                                              void *user_data) {
    (void)user_data;
    if (event && event->node) {
        for (const textparser_node *item = event->node; item; item = item->next) {
            check_json_node(handle, item, 0);
        }
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_JSON int textparser_json_register_validators(textparser_t handle) {
    if (!handle) return -1;
    return textparser_register_handler(handle, "json.legality", json_source_complete, nullptr);
}

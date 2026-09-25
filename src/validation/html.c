#include "html.h"
#include "html_common.h"
#include "html_tags.h"
#include "validation.h"

#include <textparser.h>
#include <html_definition.json.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>



typedef struct {
    char *name;
    size_t position;
    size_t len;
} html_stack_item;

typedef struct {
    /* Legacy v1 token IDs */
    int legacy_Comment;
    int legacy_Doctype;
    int legacy_ClosingTag;
    int legacy_Tag;
    int legacy_AttributeName;
    int legacy_DoubleString;
    int legacy_SingleString;

    /* Schema v2 token IDs */
    int Doctype;
    int Comment;
    int Tag_Start;
    int VoidTag_Start;
    int ScriptTag_Start;
    int StyleTag_Start;
    int ClosingTag;
    int Tag_End;
    int Tag_SelfClose;
    int ScriptTag_End;
    int StyleTag_End;
    int AttributeName;
    int DoubleString;
    int SingleString;
} html_dynamic_token_ids;

void textparser_validation_clear(textparser_validation *validation) {
    validation_clear_internal(validation);
}

static int find_token_id_by_name(textparser_t handle, const char *name) {
    const textparser_language_definition *definition = textparser_get_language(handle);
    if (definition == NULL || definition->tokens == NULL) {
        return TextParser_END;
    }
    int token_idx = 0;
    while (definition->tokens[token_idx].name != NULL) {
        if (strcmp(definition->tokens[token_idx].name, name) == 0) {
            return token_idx;
        }
        token_idx++;
    }
    return TextParser_END;
}

static void resolve_html_token_ids(textparser_t handle, html_dynamic_token_ids *ids) {
    ids->legacy_Comment = find_token_id_by_name(handle, "Comment");
    ids->legacy_Doctype = find_token_id_by_name(handle, "Doctype");
    ids->legacy_ClosingTag = find_token_id_by_name(handle, "ClosingTag");
    ids->legacy_Tag = find_token_id_by_name(handle, "Tag");
    ids->legacy_AttributeName = find_token_id_by_name(handle, "AttributeName");
    ids->legacy_DoubleString = find_token_id_by_name(handle, "DoubleString");
    ids->legacy_SingleString = find_token_id_by_name(handle, "SingleString");

    ids->Doctype = find_token_id_by_name(handle, "Doctype");
    ids->Comment = find_token_id_by_name(handle, "Comment");
    ids->Tag_Start = find_token_id_by_name(handle, "Tag_Start");
    ids->VoidTag_Start = find_token_id_by_name(handle, "VoidTag_Start");
    ids->ScriptTag_Start = find_token_id_by_name(handle, "ScriptTag_Start");
    ids->StyleTag_Start = find_token_id_by_name(handle, "StyleTag_Start");
    ids->ClosingTag = find_token_id_by_name(handle, "ClosingTag");
    ids->Tag_End = find_token_id_by_name(handle, "Tag_End");
    ids->Tag_SelfClose = find_token_id_by_name(handle, "Tag_SelfClose");
    ids->ScriptTag_End = find_token_id_by_name(handle, "ScriptTag_End");
    ids->StyleTag_End = find_token_id_by_name(handle, "StyleTag_End");
    ids->AttributeName = find_token_id_by_name(handle, "AttributeName");
    ids->DoubleString = find_token_id_by_name(handle, "DoubleString");
    ids->SingleString = find_token_id_by_name(handle, "SingleString");
}

static size_t get_token_start(textparser_token_item *token) {
    if ((token->node_flags & TEXTPARSER_NODE_EXPLICIT_SPAN) != 0) {
        return token->source_start;
    }
    return textparser_get_token_position(token);
}

static int compare_strings(const void *a, const void *b) {
    const char *str_a = *(const char *const *)a;
    const char *str_b = *(const char *const *)b;
    return strcmp(str_a, str_b);
}

static int compare_tag_info(const void *key, const void *element) {
    const char *name = (const char *)key;
    const html_tag_info *info = (const html_tag_info *)element;
    return strcmp(name, info->name);
}

static bool is_html_global_attribute(const char *name) {
    return bsearch(&name, html_global_attributes, html_global_attribute_count, sizeof(char *), compare_strings) != NULL;
}

static bool is_html_void_element(const char *name) {
    return bsearch(&name, html_void_elements, html_void_element_count, sizeof(char *), compare_strings) != NULL;
}

static const html_tag_info *find_html_tag_info(const char *name) {
    return bsearch(name, html_tags, html_tag_count, sizeof(html_tag_info), compare_tag_info);
}

static bool is_tag_specific_attribute(const html_tag_info *info, const char *attr_name) {
    if (info->attributes == NULL || info->attribute_count == 0) {
        return false;
    }
    return bsearch(&attr_name, info->attributes, info->attribute_count, sizeof(char *), compare_strings) != NULL;
}

static void get_tag_name(const char *text, textparser_token_item *token, char *buf, size_t max_len) {
    size_t token_pos = get_token_start(token);
    size_t p = token_pos + 1; // skip '<'
    size_t end = token_pos + token->len;
    size_t i = 0;
    while (p < end && i + 1 < max_len) {
        char c = text[p];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '/' || c == '>') {
            break;
        }
        buf[i++] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
        p++;
    }
    buf[i] = '\0';
}

static void get_closing_tag_name(const char *text, textparser_token_item *token, char *buf, size_t max_len) {
    size_t token_pos = get_token_start(token);
    size_t p = token_pos + 2; // skip '</'
    size_t end = token_pos + token->len;
    size_t i = 0;
    while (p < end && i + 1 < max_len) {
        char c = text[p];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '>') {
            break;
        }
        buf[i++] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
        p++;
    }
    buf[i] = '\0';
}

static bool is_tag_self_closing(const char *text, textparser_token_item *token) {
    size_t token_pos = get_token_start(token);
    size_t end = token_pos + token->len;
    if (token->len < 3) return false;
    
    int p = (int)end - 2;
    while (p >= (int)token_pos) {
        char c = text[p];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            p--;
            continue;
        }
        if (c == '/') {
            return true;
        }
        break;
    }
    return false;
}

static bool is_custom_element(const char *name) {
    return strchr(name, '-') != NULL;
}

static bool is_framework_attribute_token(const char *text, textparser_token_item *child, size_t text_size) {
    if (child->len == 0) return false;
    size_t child_pos = get_token_start(child);
    validation_string_defer(name);
    name = (char *)malloc(child->len + 1);
    if (name == NULL) {
        fprintf(stderr, "HTML validation failed: memory allocation error\n");
        exit(1);
    }
    memcpy(name, text + child_pos, child->len);
    name[child->len] = '\0';
    
    // Framework attributes like (click), [class], *ngIf, @click, :href
    // Check characters within the token itself (schema v2) or immediately surrounding (legacy)
    if (name[0] == '[' || name[0] == '(' || name[0] == '*' || name[0] == '@' || name[0] == ':') {
        return true;
    }
    if (name[child->len - 1] == ']' || name[child->len - 1] == ')') {
        return true;
    }
    if (child_pos > 0) {
        char prev = text[child_pos - 1];
        if (prev == '[' || prev == '(' || prev == '*' || prev == '@' || prev == ':') {
            return true;
        }
    }
    if (child_pos + child->len < text_size) {
        char next = text[child_pos + child->len];
        if (next == ']' || next == ')') {
            return true;
        }
    }
    
    // Namespace or event binding like v-on:click or xml:lang or data-xyz
    if (strchr(name, ':') != NULL || strchr(name, '.') != NULL) {
        return true;
    }
    // Starts with data- or aria- or on (event handlers)
    if (strncmp(name, "data-", 5) == 0 || strncmp(name, "aria-", 5) == 0 || strncmp(name, "on", 2) == 0) {
        return true;
    }
    // Any attribute containing a hyphen (like custom element attributes or custom attributes)
    if (strchr(name, '-') != NULL) {
        return true;
    }
    return false;
}

static void check_mandatory_attributes(textparser_validation **ret, const char *tag_name,
                                      size_t tag_pos, size_t tag_len,
                                      bool has_src, bool has_alt, bool has_charset,
                                      bool has_content, bool has_href, bool has_target, bool has_rel) {
    if (strcmp(tag_name, "img") == 0) {
        if (!has_src) {
            char *str = dynamic_printf("HTML tag <img> is missing mandatory attribute [src]");
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, tag_pos, tag_len);
        }
        if (!has_alt) {
            char *str = dynamic_printf("HTML tag <img> is missing mandatory attribute [alt]");
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING, ret, str, tag_pos, tag_len);
        }
    } else if (strcmp(tag_name, "meta") == 0) {
        if (!has_charset && !has_content) {
            char *str = dynamic_printf("HTML tag <meta> is missing mandatory attribute [content]");
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, tag_pos, tag_len);
        }
    } else if (strcmp(tag_name, "base") == 0) {
        if (!has_href && !has_target) {
            char *str = dynamic_printf("HTML tag <base> is missing mandatory attribute [href] or [target]");
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, tag_pos, tag_len);
        }
    } else if (strcmp(tag_name, "link") == 0) {
        if (!has_rel) {
            char *str = dynamic_printf("HTML tag <link> is missing mandatory attribute [rel]");
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, tag_pos, tag_len);
        }
    } else if (strcmp(tag_name, "area") == 0) {
        if (has_href && !has_alt) {
            char *str = dynamic_printf("HTML tag <area> is missing mandatory attribute [alt] when [href] is present");
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, tag_pos, tag_len);
        }
    }
}

static void push_html_stack(html_stack_item **stack, int *stack_top, int *stack_capacity,
                            const char *tag_name, size_t tag_pos, size_t tag_len) {
    if (*stack_top >= *stack_capacity) {
        int new_capacity = (*stack_capacity == 0) ? 32 : (*stack_capacity) * 2;
        html_stack_item *new_stack = realloc(*stack, sizeof(html_stack_item) * new_capacity);
        if (new_stack == NULL) {
            free(*stack);
            fprintf(stderr, "HTML validation failed: memory allocation error\n");
            exit(1);
        }
        *stack = new_stack;
        *stack_capacity = new_capacity;
    }
    (*stack)[*stack_top].name = strdup(tag_name);
    (*stack)[*stack_top].position = tag_pos;
    (*stack)[*stack_top].len = tag_len;
    (*stack_top)++;
}

static void handle_closing_tag(textparser_validation **ret, html_stack_item *stack, int *stack_top,
                               const char *tag_name, size_t token_pos, size_t token_len) {
    bool void_tag = is_html_void_element(tag_name);
    if (void_tag) {
        char *str = dynamic_printf("Ending tag </%s> is forbidden for void element", tag_name);
        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token_pos, token_len);
    } else {
        int found_idx = -1;
        for (int i = *stack_top - 1; i >= 0; i--) {
            if (strcmp(stack[i].name, tag_name) == 0) {
                found_idx = i;
                break;
            }
        }

        if (found_idx != -1) {
            for (int i = *stack_top - 1; i > found_idx; i--) {
                char *str = dynamic_printf("HTML tag [%s] requires a closing tag </%s>", stack[i].name, stack[i].name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, stack[i].position, stack[i].len);
                free(stack[i].name);
            }
            free(stack[found_idx].name);
            *stack_top = found_idx;
        } else {
            char *str = dynamic_printf("Ending tag </%s> has no matching start tag", tag_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token_pos, token_len);
        }
    }
}

static void textparser_validate_html_v2(const html_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle) {
    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    html_stack_item *stack = NULL;
    int stack_capacity = 0;
    int stack_top = 0;

    textparser_token_item *token = textparser_get_first_token(handle);
    while (token != NULL) {
        int tid = token->token_id;
        if (tid != TextParser_END &&
            (tid == ids->Tag_Start || tid == ids->VoidTag_Start ||
             tid == ids->ScriptTag_Start || tid == ids->StyleTag_Start)) {

            textparser_token_item *start_tok = token;
            size_t tag_start = get_token_start(start_tok);
            char tag_name[128];
            get_tag_name(text, start_tok, tag_name, sizeof(tag_name));

            bool known_tag = find_html_tag_info(tag_name) != NULL;
            bool void_tag = is_html_void_element(tag_name);

            if (!known_tag && !is_custom_element(tag_name)) {
                char *str = dynamic_printf("Unknown HTML tag: [%s]", tag_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, tag_start, strlen(tag_name) + 1);
            }

            const html_tag_info *tag_info = find_html_tag_info(tag_name);
            bool has_src = false;
            bool has_alt = false;
            bool has_content = false;
            bool has_charset = false;
            bool has_href = false;
            bool has_target = false;
            bool has_rel = false;
            bool self_closing = false;

            textparser_token_item *child = token->next;
            while (child != NULL) {
                int c_tid = child->token_id;
                if (c_tid == ids->Tag_End || c_tid == ids->ScriptTag_End || c_tid == ids->StyleTag_End) {
                    token = child;
                    break;
                }
                if (c_tid == ids->Tag_SelfClose) {
                    self_closing = true;
                    token = child;
                    break;
                }
                if (c_tid != TextParser_END && c_tid == ids->AttributeName) {
                    size_t child_pos = get_token_start(child);
                    validation_string_defer(attr_name);
                    attr_name = (char *)malloc(child->len + 1);
                    if (attr_name == NULL) {
                        fprintf(stderr, "HTML validation failed: memory allocation error\n");
                        exit(1);
                    }
                    memcpy(attr_name, text + child_pos, child->len);
                    attr_name[child->len] = '\0';
                    for (int i = 0; attr_name[i]; i++) {
                        if (attr_name[i] >= 'A' && attr_name[i] <= 'Z') {
                            attr_name[i] = attr_name[i] - 'A' + 'a';
                        }
                    }

                    if (strcmp(attr_name, "src") == 0) has_src = true;
                    else if (strcmp(attr_name, "alt") == 0) has_alt = true;
                    else if (strcmp(attr_name, "content") == 0) has_content = true;
                    else if (strcmp(attr_name, "charset") == 0) has_charset = true;
                    else if (strcmp(attr_name, "href") == 0) has_href = true;
                    else if (strcmp(attr_name, "target") == 0) has_target = true;
                    else if (strcmp(attr_name, "rel") == 0) has_rel = true;

                    if (known_tag || !is_custom_element(tag_name)) {
                        bool valid_attr = is_html_global_attribute(attr_name) ||
                                          is_framework_attribute_token(text, child, text_size) ||
                                          (tag_info != NULL && is_tag_specific_attribute(tag_info, attr_name));

                        if (!valid_attr) {
                            char *str = dynamic_printf("Unknown attribute [%s] for HTML tag <%s>", attr_name, tag_name);
                            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, child_pos, child->len);
                        }
                    }
                }
                child = child->next;
            }

            size_t tag_end = (token != NULL) ? (get_token_start(token) + token->len) : (tag_start + start_tok->len);
            size_t tag_len = tag_end - tag_start;

            if (known_tag || !is_custom_element(tag_name)) {
                check_mandatory_attributes(ret, tag_name, tag_start, tag_len,
                                           has_src, has_alt, has_charset, has_content, has_href, has_target, has_rel);
            }

            if (!void_tag && !self_closing) {
                push_html_stack(&stack, &stack_top, &stack_capacity, tag_name, tag_start, tag_len);
            }
        }
        else if (tid != TextParser_END && tid == ids->ClosingTag) {
            char tag_name[128];
            get_closing_tag_name(text, token, tag_name, sizeof(tag_name));
            size_t token_pos = get_token_start(token);
            handle_closing_tag(ret, stack, &stack_top, tag_name, token_pos, token->len);
        }

        token = token->next;
    }

    for (int i = stack_top - 1; i >= 0; i--) {
        char *str = dynamic_printf("HTML tag [%s] requires a closing tag </%s>", stack[i].name, stack[i].name);
        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, stack[i].position, stack[i].len);
        free(stack[i].name);
    }
    free(stack);
}

static void textparser_validate_html_legacy(const html_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle) {
    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    html_stack_item *stack = NULL;
    int stack_capacity = 0;
    int stack_top = 0;

    textparser_token_item *token = textparser_get_first_token(handle);
    while (token != NULL) {
        if (token->token_id != TextParser_END && token->token_id == ids->legacy_Tag) {
            char tag_name[128];
            get_tag_name(text, token, tag_name, sizeof(tag_name));
            size_t token_pos = get_token_start(token);

            bool known_tag = find_html_tag_info(tag_name) != NULL;
            bool void_tag = is_html_void_element(tag_name);

            if (!known_tag && !is_custom_element(tag_name)) {
                char *str = dynamic_printf("Unknown HTML tag: [%s]", tag_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token_pos, strlen(tag_name) + 1);
            }

            if (known_tag || !is_custom_element(tag_name)) {
                const html_tag_info *tag_info = find_html_tag_info(tag_name);
                textparser_token_item *child = token->child;
                bool has_src = false;
                bool has_alt = false;
                bool has_content = false;
                bool has_charset = false;
                bool has_href = false;
                bool has_target = false;
                bool has_rel = false;

                while (child != NULL) {
                    if (child->token_id != TextParser_END && child->token_id == ids->legacy_AttributeName) {
                        size_t child_pos = get_token_start(child);
                        validation_string_defer(attr_name);
                        attr_name = (char *)malloc(child->len + 1);
                        if (attr_name == NULL) {
                            fprintf(stderr, "HTML validation failed: memory allocation error\n");
                            exit(1);
                        }
                        memcpy(attr_name, text + child_pos, child->len);
                        attr_name[child->len] = '\0';
                        for (int i = 0; attr_name[i]; i++) {
                            if (attr_name[i] >= 'A' && attr_name[i] <= 'Z') {
                                attr_name[i] = attr_name[i] - 'A' + 'a';
                            }
                        }

                        if (strcmp(attr_name, "src") == 0) has_src = true;
                        else if (strcmp(attr_name, "alt") == 0) has_alt = true;
                        else if (strcmp(attr_name, "content") == 0) has_content = true;
                        else if (strcmp(attr_name, "charset") == 0) has_charset = true;
                        else if (strcmp(attr_name, "href") == 0) has_href = true;
                        else if (strcmp(attr_name, "target") == 0) has_target = true;
                        else if (strcmp(attr_name, "rel") == 0) has_rel = true;

                        bool valid_attr = is_html_global_attribute(attr_name) ||
                                          is_framework_attribute_token(text, child, text_size) ||
                                          (tag_info != NULL && is_tag_specific_attribute(tag_info, attr_name));

                        if (!valid_attr) {
                            char *str = dynamic_printf("Unknown attribute [%s] for HTML tag <%s>", attr_name, tag_name);
                            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, child_pos, child->len);
                        }
                    }
                    child = child->next;
                }

                check_mandatory_attributes(ret, tag_name, token_pos, token->len,
                                           has_src, has_alt, has_charset, has_content, has_href, has_target, has_rel);
            }

            bool self_closing = is_tag_self_closing(text, token);
            if (!void_tag && !self_closing) {
                push_html_stack(&stack, &stack_top, &stack_capacity, tag_name, token_pos, token->len);
            }
        }
        else if (token->token_id != TextParser_END && token->token_id == ids->legacy_ClosingTag) {
            char tag_name[128];
            get_closing_tag_name(text, token, tag_name, sizeof(tag_name));
            size_t token_pos = get_token_start(token);
            handle_closing_tag(ret, stack, &stack_top, tag_name, token_pos, token->len);
        }

        token = token->next;
    }

    for (int i = stack_top - 1; i >= 0; i--) {
        char *str = dynamic_printf("HTML tag [%s] requires a closing tag </%s>", stack[i].name, stack[i].name);
        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, stack[i].position, stack[i].len);
        free(stack[i].name);
    }
    free(stack);
}

textparser_validation *textparser_validate_html(textparser_t handle) {
    html_dynamic_token_ids ids;
    resolve_html_token_ids(handle, &ids);

    textparser_validation *ret = nullptr;

    if (ids.legacy_Tag != TextParser_END) {
        textparser_validate_html_legacy(&ids, &ret, handle);
    } else {
        textparser_validate_html_v2(&ids, &ret, handle);
    }

    return ret;
}


#include "html.h"
#include "html_common.h"
#include "html_tags.h"

#include <textparser.h>
#include <html_definition.json.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

#define MAX_STACK_DEPTH 1024

typedef struct {
    char name[128];
    size_t position;
    size_t len;
} html_stack_item;

typedef struct {
    int Comment;
    int Doctype;
    int ClosingTag;
    int Tag;
    int AttributeName;
    int DoubleString;
    int SingleString;
} html_dynamic_token_ids;

static char *dynamic_printf(const char *format, ...) {
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);

    int length = vsnprintf(NULL, 0, format, args1);
    va_end(args1);

    if (length < 0) {
        va_end(args2);
        return NULL;
    }

    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        va_end(args2);
        return NULL;
    }

    vsnprintf(buffer, length + 1, format, args2);
    va_end(args2);

    return buffer;
}

static void textparser_validation_item_add(enum textparser_validation_item_type type, textparser_validation **validation, char *text, size_t position, size_t length) {
    int current_len = *validation ? (*validation)->len : 0;
    textparser_validation *new_val = realloc(*validation, offsetof(textparser_validation, items) + sizeof(textparser_validation_item *) * (current_len + 1));
    if (new_val == NULL) {
        free(text);
        return;
    }
    *validation = new_val;
    if (current_len == 0) {
        (*validation)->len = 0;
    }

    textparser_validation_item *item = malloc(sizeof(textparser_validation_item));
    if (item == NULL) {
        free(text);
        return;
    }
    item->type = type;
    item->position = position;
    item->length = length;
    item->text = text;

    (*validation)->items[(*validation)->len] = item;
    (*validation)->len++;
}

void textparser_validation_clear(textparser_validation *validation) {
    if (validation != NULL) {
        for (int i = 0; i < validation->len; i++) {
            free(validation->items[i]->text);
            free(validation->items[i]);
        }
        free(validation);
    }
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
    ids->Comment = find_token_id_by_name(handle, "Comment");
    ids->Doctype = find_token_id_by_name(handle, "Doctype");
    ids->ClosingTag = find_token_id_by_name(handle, "ClosingTag");
    ids->Tag = find_token_id_by_name(handle, "Tag");
    ids->AttributeName = find_token_id_by_name(handle, "AttributeName");
    ids->DoubleString = find_token_id_by_name(handle, "DoubleString");
    ids->SingleString = find_token_id_by_name(handle, "SingleString");
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
    size_t p = token->position + 1; // skip '<'
    size_t end = token->position + token->len;
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
    size_t p = token->position + 2; // skip '</'
    size_t end = token->position + token->len;
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
    size_t end = token->position + token->len;
    if (token->len < 3) return false;
    
    int p = (int)end - 2;
    while (p >= (int)token->position) {
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
    char *name = alloca(child->len + 1);
    memcpy(name, text + child->position, child->len);
    name[child->len] = '\0';
    
    // Framework attributes like (click), [class], *ngIf, @click, :href
    // Check characters immediately surrounding the token in the source text
    if (child->position > 0) {
        char prev = text[child->position - 1];
        if (prev == '[' || prev == '(' || prev == '*' || prev == '@' || prev == ':') {
            return true;
        }
    }
    if (child->position + child->len < text_size) {
        char next = text[child->position + child->len];
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

textparser_validation *textparser_validate_html(textparser_t handle) {
    html_dynamic_token_ids ids;
    resolve_html_token_ids(handle, &ids);

    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    textparser_validation *ret = nullptr;

    html_stack_item stack[MAX_STACK_DEPTH];
    int stack_top = 0;

    textparser_token_item *token = textparser_get_first_token(handle);
    while (token != NULL) {
        if (token->token_id != TextParser_END && token->token_id == ids.Tag) {
            char tag_name[128];
            get_tag_name(text, token, tag_name, sizeof(tag_name));

            // Validate tag name
            bool known_tag = find_html_tag_info(tag_name) != NULL;
            bool void_tag = is_html_void_element(tag_name);

            if (!known_tag && !is_custom_element(tag_name)) {
                char *str = dynamic_printf("Unknown HTML tag: [%s]", tag_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, strlen(tag_name) + 1);
            }

            // Validate attributes inside this tag
            // Custom elements can have any attributes, so we only validate attributes on standard tags
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
                    if (child->token_id != TextParser_END && child->token_id == ids.AttributeName) {
                        char *attr_name = alloca(child->len + 1);
                        memcpy(attr_name, text + child->position, child->len);
                        attr_name[child->len] = '\0';
                        // Convert to lowercase
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
                            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, child->position, child->len);
                        }
                    }
                    child = child->next;
                }

                // Perform mandatory attribute checks
                if (strcmp(tag_name, "img") == 0) {
                    if (!has_src) {
                        char *str = dynamic_printf("HTML tag <img> is missing mandatory attribute [src]");
                        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, token->len);
                    }
                    if (!has_alt) {
                        char *str = dynamic_printf("HTML tag <img> is missing mandatory attribute [alt]");
                        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING, &ret, str, token->position, token->len);
                    }
                } else if (strcmp(tag_name, "meta") == 0) {
                    if (!has_charset && !has_content) {
                        char *str = dynamic_printf("HTML tag <meta> is missing mandatory attribute [content]");
                        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, token->len);
                    }
                } else if (strcmp(tag_name, "base") == 0) {
                    if (!has_href && !has_target) {
                        char *str = dynamic_printf("HTML tag <base> is missing mandatory attribute [href] or [target]");
                        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, token->len);
                    }
                } else if (strcmp(tag_name, "link") == 0) {
                    if (!has_rel) {
                        char *str = dynamic_printf("HTML tag <link> is missing mandatory attribute [rel]");
                        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, token->len);
                    }
                } else if (strcmp(tag_name, "area") == 0) {
                    if (has_href && !has_alt) {
                        char *str = dynamic_printf("HTML tag <area> is missing mandatory attribute [alt] when [href] is present");
                        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, token->len);
                    }
                }
            }

            // Handle tag nesting
            bool self_closing = is_tag_self_closing(text, token);
            if (!void_tag && !self_closing) {
                // Push to stack
                if (stack_top < MAX_STACK_DEPTH) {
                    strncpy(stack[stack_top].name, tag_name, sizeof(stack[stack_top].name) - 1);
                    stack[stack_top].name[sizeof(stack[stack_top].name) - 1] = '\0';
                    stack[stack_top].position = token->position;
                    stack[stack_top].len = token->len;
                    stack_top++;
                }
            }
        }
        else if (token->token_id != TextParser_END && token->token_id == ids.ClosingTag) {
            char tag_name[128];
            get_closing_tag_name(text, token, tag_name, sizeof(tag_name));

            bool void_tag = is_html_void_element(tag_name);
            if (void_tag) {
                char *str = dynamic_printf("Ending tag </%s> is forbidden for void element", tag_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, token->len);
            } else {
                // Look for matching tag in the stack
                int found_idx = -1;
                for (int i = stack_top - 1; i >= 0; i--) {
                    if (strcmp(stack[i].name, tag_name) == 0) {
                        found_idx = i;
                        break;
                    }
                }

                if (found_idx != -1) {
                    // Intermediate tags on the stack were never closed
                    for (int i = stack_top - 1; i > found_idx; i--) {
                        char *str = dynamic_printf("HTML tag [%s] requires a closing tag </%s>", stack[i].name, stack[i].name);
                        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, stack[i].position, stack[i].len);
                    }
                    // Pop all the way to found_idx
                    stack_top = found_idx;
                } else {
                    char *str = dynamic_printf("Ending tag </%s> has no matching start tag", tag_name);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, token->position, token->len);
                }
            }
        }

        token = token->next;
    }

    // Any remaining tags on the stack were never closed
    for (int i = stack_top - 1; i >= 0; i--) {
        char *str = dynamic_printf("HTML tag [%s] requires a closing tag </%s>", stack[i].name, stack[i].name);
        textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, &ret, str, stack[i].position, stack[i].len);
    }

    return ret;
}

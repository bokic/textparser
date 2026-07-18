#include "php.h"
#include "php_common.h"
#include "php_functions.h"

#include <textparser.h>
#include <php_definition.json.h>

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

#define MAX_USER_FUNCS 512

typedef struct {
    int Tag;
    int Keyword;
    int Parenthesis;
    int LineComment;
    int BlockComment;
    int SingleString;
    int DoubleString;
    int ArrayIndex;
    int CodeBlock;
} php_dynamic_token_ids;

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

static void resolve_php_token_ids(textparser_t handle, php_dynamic_token_ids *ids) {
    ids->Tag = find_token_id_by_name(handle, "Tag");
    ids->Keyword = find_token_id_by_name(handle, "Keyword");
    ids->Parenthesis = find_token_id_by_name(handle, "Parenthesis");
    ids->LineComment = find_token_id_by_name(handle, "LineComment");
    ids->BlockComment = find_token_id_by_name(handle, "BlockComment");
    ids->SingleString = find_token_id_by_name(handle, "SingleString");
    ids->DoubleString = find_token_id_by_name(handle, "DoubleString");
    ids->ArrayIndex = find_token_id_by_name(handle, "ArrayIndex");
    ids->CodeBlock = find_token_id_by_name(handle, "CodeBlock");
}

static void str_tolower(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] - 'A' + 'a';
        }
    }
}

static int compare_php_function_info(const void *key, const void *element) {
    const char *name = (const char *)key;
    const php_function_info *info = (const php_function_info *)element;
    return strcmp(name, info->name);
}

static const php_function_info *find_php_function_info(const char *name) {
    return (const php_function_info *)bsearch(
        name,
        php_functions,
        php_function_count,
        sizeof(php_function_info),
        compare_php_function_info
    );
}

static bool is_token_in_string(textparser_t handle, textparser_token_item *token) {
    textparser_token_item *p = token->parent;
    while (p != nullptr) {
        const char *type_str = textparser_get_token_type_str(textparser_get_language(handle), p);
        if (type_str != nullptr) {
            if (strcmp(type_str, "SingleString") == 0 || strcmp(type_str, "DoubleString") == 0) {
                return true;
            }
            if (strcmp(type_str, "CodeBlock") == 0) {
                return false;
            }
        }
        p = p->parent;
    }
    return false;
}

static bool get_function_name_before_paren(textparser_t handle, textparser_token_item *paren_token, const char **func_name_out, size_t *func_len_out, size_t *func_pos_out) {
    const char *text = textparser_get_text(handle);
    size_t pos = paren_token->position;
    
    int p = (int)pos - 1;
    textparser_token_item *prev_tok = paren_token->prev;
    
    while (p >= 0) {
        if (text[p] == ' ' || text[p] == '\t' || text[p] == '\r' || text[p] == '\n') {
            p--;
            continue;
        }
        
        if (prev_tok != NULL && (prev_tok->position + prev_tok->len) > (size_t)p) {
            const char *type_str = textparser_get_token_type_str(textparser_get_language(handle), prev_tok);
            if (type_str != NULL && (strcmp(type_str, "LineComment") == 0 || strcmp(type_str, "BlockComment") == 0)) {
                p = (int)prev_tok->position - 1;
                prev_tok = prev_tok->prev;
                continue;
            }
        }
        break;
    }
    
    if (p < 0) return false;
    
    int end = p;
    while (p >= 0 && ((text[p] >= 'a' && text[p] <= 'z') ||
                      (text[p] >= 'A' && text[p] <= 'Z') ||
                      (text[p] >= '0' && text[p] <= '9') ||
                      text[p] == '_')) {
        p--;
    }
    
    size_t start = p + 1;
    size_t len = end - p;
    
    if (len == 0) return false;
    if (text[start] >= '0' && text[start] <= '9') return false;
    
    char *ident = alloca(len + 1);
    memcpy(ident, text + start, len);
    ident[len] = '\0';
    
    const char *keywords[] = {
        "echo", "print", "function", "class", "public", "private", "protected",
        "static", "if", "else", "elseif", "foreach", "for", "while", "do",
        "switch", "case", "default", "return", "require", "require_once",
        "include", "include_once", "new", "use", "namespace", "try", "catch",
        "throw", "finally", "global", "as", "array", "list"
    };
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(ident, keywords[i]) == 0) {
            return false;
        }
    }
    
    int q = (int)start - 1;
    while (q >= 0 && (text[q] == ' ' || text[q] == '\t' || text[q] == '\r' || text[q] == '\n')) {
        q--;
    }
    if (q >= 1) {
        if ((text[q-1] == '-' && text[q] == '>') || (text[q-1] == ':' && text[q] == ':')) {
            return false;
        }
    }
    
    int w = q;
    while (w >= 0 && ((text[w] >= 'a' && text[w] <= 'z') ||
                      (text[w] >= 'A' && text[w] <= 'Z') ||
                      (text[w] >= '0' && text[w] <= '9') ||
                      text[w] == '_')) {
        w--;
    }
    size_t w_start = w + 1;
    size_t w_len = q - w;
    if (w_len == 3 && strncmp(text + w_start, "new", 3) == 0) {
        return false;
    }
    
    *func_name_out = text + start;
    *func_len_out = len;
    *func_pos_out = start;
    return true;
}

static void collect_user_defined_functions_tree(textparser_t handle, textparser_token_item *token, int keyword_id, char **user_funcs, int *user_funcs_count) {
    const char *text = textparser_get_text(handle);
    while (token != NULL) {
        if (token->token_id == keyword_id && token->len == 8 && strncmp(text + token->position, "function", 8) == 0) {
            size_t p = token->position + token->len;
            size_t text_size = textparser_get_text_size(handle);
            textparser_token_item *next_tok = token->next;
            
            while (p < text_size) {
                if (text[p] == ' ' || text[p] == '\t' || text[p] == '\r' || text[p] == '\n') {
                    p++;
                    continue;
                }
                
                if (next_tok != NULL && next_tok->position == p) {
                    const char *type_str = textparser_get_token_type_str(textparser_get_language(handle), next_tok);
                    if (type_str != NULL && (strcmp(type_str, "LineComment") == 0 || strcmp(type_str, "BlockComment") == 0)) {
                        p += next_tok->len;
                        next_tok = next_tok->next;
                        continue;
                    }
                }
                break;
            }
            
            if (p < text_size) {
                size_t start = p;
                while (p < text_size && ((text[p] >= 'a' && text[p] <= 'z') ||
                                         (text[p] >= 'A' && text[p] <= 'Z') ||
                                         (text[p] >= '0' && text[p] <= '9') ||
                                         text[p] == '_')) {
                    p++;
                }
                size_t len = p - start;
                if (len > 0 && !(text[start] >= '0' && text[start] <= '9') && *user_funcs_count < MAX_USER_FUNCS) {
                    char *name = malloc(len + 1);
                    memcpy(name, text + start, len);
                    name[len] = '\0';
                    str_tolower(name);
                    user_funcs[(*user_funcs_count)++] = name;
                }
            }
        }
        
        if (token->child != NULL) {
            collect_user_defined_functions_tree(handle, token->child, keyword_id, user_funcs, user_funcs_count);
        }
        token = token->next;
    }
}

static void textparser_validate_php_token(const php_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token, const char **user_funcs, int user_funcs_count) {
    if (token->token_id != TextParser_END && token->token_id == ids->Parenthesis) {
        // Skip if this token is inside a literal string context
        if (is_token_in_string(handle, token)) {
            return;
        }

        const char *func_name = nullptr;
        size_t func_len = 0;
        size_t func_pos = 0;

        if (get_function_name_before_paren(handle, token, &func_name, &func_len, &func_pos)) {
            // Check if user-defined or built-in function
            char *name_lower = alloca(func_len + 1);
            memcpy(name_lower, func_name, func_len);
            name_lower[func_len] = '\0';
            str_tolower(name_lower);

            bool is_user_defined = false;
            for (int i = 0; i < user_funcs_count; i++) {
                if (strcmp(name_lower, user_funcs[i]) == 0) {
                    is_user_defined = true;
                    break;
                }
            }

            if (is_user_defined) {
                return; // skip parameter checks for user-defined functions for simplicity
            }

            const php_function_info *fn_info = find_php_function_info(name_lower);
            if (fn_info == nullptr) {
                char *str = dynamic_printf("Unknown PHP function: [%s]", name_lower);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, func_pos, func_len);
            } else {
                // Count arguments passed inside this parenthesis
                const char *text = textparser_get_text(handle);
                size_t start_pos = token->position + 1;
                size_t end_pos = token->position + token->len - 1;
                size_t p = start_pos;
                textparser_token_item *curr_child = token->child;
                int comma_count = 0;
                bool has_arguments = false;

                while (p < end_pos) {
                    if (curr_child != NULL && curr_child->position == p) {
                        const char *type_str = textparser_get_token_type_str(textparser_get_language(handle), curr_child);
                        if (type_str != NULL && (
                            strcmp(type_str, "SingleString") == 0 ||
                            strcmp(type_str, "DoubleString") == 0 ||
                            strcmp(type_str, "LineComment") == 0 ||
                            strcmp(type_str, "BlockComment") == 0 ||
                            strcmp(type_str, "Parenthesis") == 0 ||
                            strcmp(type_str, "ArrayIndex") == 0 ||
                            strcmp(type_str, "CodeBlock") == 0
                        )) {
                            if (strcmp(type_str, "LineComment") != 0 && strcmp(type_str, "BlockComment") != 0) {
                                has_arguments = true;
                            }
                            p += curr_child->len;
                            curr_child = curr_child->next;
                            continue;
                        }
                        curr_child = curr_child->next;
                    }

                    char c = text[p];
                    if (c == ',') {
                        comma_count++;
                        has_arguments = true;
                    } else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                        has_arguments = true;
                    }
                    p++;
                }

                int arg_count = 0;
                if (has_arguments) {
                    arg_count = comma_count + 1;
                }

                // Check for trailing comma
                int last_comma_check = (int)end_pos - 1;
                bool trailing_comma = false;
                while (last_comma_check >= (int)start_pos) {
                    char c = text[last_comma_check];
                    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                        last_comma_check--;
                        continue;
                    }
                    
                    textparser_token_item *child = token->child;
                    bool in_comment = false;
                    while (child != NULL) {
                        if (last_comma_check >= (int)child->position && last_comma_check < (int)(child->position + child->len)) {
                            const char *type_str = textparser_get_token_type_str(textparser_get_language(handle), child);
                            if (type_str != NULL && (strcmp(type_str, "LineComment") == 0 || strcmp(type_str, "BlockComment") == 0)) {
                                in_comment = true;
                                last_comma_check = (int)child->position - 1;
                                break;
                            }
                        }
                        child = child->next;
                    }
                    if (in_comment) {
                        continue;
                    }
                    
                    if (c == ',') {
                        trailing_comma = true;
                    }
                    break;
                }

                if (trailing_comma && arg_count > 0) {
                    arg_count--;
                }

                // Calculate parameter expectations
                int min_params = 0;
                int max_params = 0;
                bool is_variadic = false;

                if (fn_info->parameters != nullptr) {
                    const php_function_parameter_info *param = fn_info->parameters;
                    while (param->name != nullptr) {
                        max_params++;
                        if (param->required) {
                            min_params++;
                        }
                        if (param->is_variadic) {
                            is_variadic = true;
                        }
                        param++;
                    }
                }

                if (arg_count < min_params) {
                    char *str = dynamic_printf("Function [%s] requires at least %d arguments, but %d were provided", name_lower, min_params, arg_count);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, func_pos, func_len);
                } else if (!is_variadic && arg_count > max_params) {
                    char *str = dynamic_printf("Function [%s] takes at most %d arguments, but %d were provided", name_lower, max_params, arg_count);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, func_pos, func_len);
                }
            }
        }
    }
}

static void textparser_validate_php_tree(const php_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token, const char **user_funcs, int user_funcs_count, int depth) {
    if (depth >= MAX_RECURSION_DEPTH) {
        return;
    }
    while (token != nullptr) {
        textparser_validate_php_token(ids, ret, handle, token, user_funcs, user_funcs_count);
        if (token->child != nullptr) {
            textparser_validate_php_tree(ids, ret, handle, token->child, user_funcs, user_funcs_count, depth + 1);
        }
        token = token->next;
    }
}

textparser_validation *textparser_validate_php(textparser_t handle) {
    php_dynamic_token_ids ids;
    resolve_php_token_ids(handle, &ids);

    char *user_funcs[MAX_USER_FUNCS];
    int user_funcs_count = 0;

    textparser_token_item *token = textparser_get_first_token(handle);
    collect_user_defined_functions_tree(handle, token, ids.Keyword, user_funcs, &user_funcs_count);

    textparser_validation *ret = nullptr;
    textparser_validate_php_tree(&ids, &ret, handle, token, (const char **)user_funcs, user_funcs_count, 0);

    for (int i = 0; i < user_funcs_count; i++) {
        free(user_funcs[i]);
    }

    return ret;
}

#include "cfml.h"
#include "cfml_common.h"
#include "cfml_functions.h"
#include "cfml_tags.h"

#include <textparser.h>
#include <cfml_definition.json.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif


static char *dynamic_printf(const char *format, ...) {
    va_list args1, args2;

    // 1. Initialize the variable argument lists
    va_start(args1, format);
    // We make a copy because a va_list can only be read through once safely
    va_copy(args2, args1);

    // 2. Ask vsnprintf how many characters the final string will need
    // We pass NULL and 0 so it only counts the length without writing anything
    int length = vsnprintf(NULL, 0, format, args1);
    va_end(args1);

    // If something went wrong with formatting, return NULL
    if (length < 0) {
        va_end(args2);
        return NULL;
    }

    // 3. Allocate memory (+1 is vital to leave room for the '\0' null terminator)
    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        va_end(args2);
        return NULL; // Return NULL if the system runs out of memory
    }

    // 4. Fill the allocated buffer with our formatted text
    vsnprintf(buffer, length + 1, format, args2);
    va_end(args2);

    // 5. Return the newly created string to the caller
    return buffer;
}

static void textparser_validation_item_add(enum textparser_validation_item_type type, textparser_validation **validation, char *text, size_t position, size_t length) {
    int current_len = *validation ? (*validation)->len : 0;
    textparser_validation *new_val = realloc(*validation, offsetof(textparser_validation, items) + sizeof(textparser_validation_item *) * (current_len + 1));
    if (new_val == nullptr) {
        free(text);
        return;
    }
    *validation = new_val;
    if (current_len == 0) {
        (*validation)->len = 0;
    }

    textparser_validation_item *item = malloc(sizeof(textparser_validation_item));
    if (item == nullptr) {
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

void textparser_validation_clear(textparser_validation *validation)
{
    if (validation != nullptr) {
        for (int i = 0; i < validation->len; i++) {
            free(validation->items[i]->text);
            free(validation->items[i]);
        }
        free(validation);
    }
}

typedef struct {
    int StartTag;
    int LoopStartTag;
    int QueryStartTag;
    int OutputStartTag;
    int ScriptStartTag;
    int EndTag;
    int LoopEndTag;
    int QueryEndTag;
    int OutputEndTag;
    int ScriptEndTag;
    int Function;
    int ScriptLineComment;
    int ScriptBlockComment;
    int Parenthesis;
    int Expression;
    int ScriptExpression;
    int Separator;
} cfml_dynamic_token_ids;

static int find_token_id_by_name(textparser_t handle, const char *name) {
    const textparser_language_definition *definition = textparser_get_language(handle);
    if (definition == nullptr || definition->tokens == nullptr) {
        return TextParser_END;
    }
    int token_idx = 0;
    while (definition->tokens[token_idx].name != nullptr) {
        if (strcmp(definition->tokens[token_idx].name, name) == 0) {
            return token_idx;
        }
        token_idx++;
    }
    return TextParser_END;
}

static void resolve_cfml_token_ids(textparser_t handle, cfml_dynamic_token_ids *ids) {
    ids->StartTag = find_token_id_by_name(handle, "StartTag");
    ids->LoopStartTag = find_token_id_by_name(handle, "LoopStartTag");
    ids->QueryStartTag = find_token_id_by_name(handle, "QueryStartTag");
    ids->OutputStartTag = find_token_id_by_name(handle, "OutputStartTag");
    ids->ScriptStartTag = find_token_id_by_name(handle, "ScriptStartTag");
    ids->EndTag = find_token_id_by_name(handle, "EndTag");
    ids->LoopEndTag = find_token_id_by_name(handle, "LoopEndTag");
    ids->QueryEndTag = find_token_id_by_name(handle, "QueryEndTag");
    ids->OutputEndTag = find_token_id_by_name(handle, "OutputEndTag");
    ids->ScriptEndTag = find_token_id_by_name(handle, "ScriptEndTag");
    ids->Function = find_token_id_by_name(handle, "Function");
    ids->ScriptLineComment = find_token_id_by_name(handle, "ScriptLineComment");
    ids->ScriptBlockComment = find_token_id_by_name(handle, "ScriptBlockComment");
    ids->Parenthesis = find_token_id_by_name(handle, "Parenthesis");
    ids->Expression = find_token_id_by_name(handle, "Expression");
    ids->ScriptExpression = find_token_id_by_name(handle, "ScriptExpression");
    ids->Separator = find_token_id_by_name(handle, "Separator");
}

static bool is_start_tag_token(const cfml_dynamic_token_ids *ids, int token_id) {
    return token_id != TextParser_END && (
           token_id == ids->StartTag ||
           token_id == ids->LoopStartTag ||
           token_id == ids->QueryStartTag ||
           token_id == ids->OutputStartTag ||
           token_id == ids->ScriptStartTag);
}

static bool is_end_tag_token(const cfml_dynamic_token_ids *ids, int token_id) {
    return token_id != TextParser_END && (
           token_id == ids->EndTag ||
           token_id == ids->LoopEndTag ||
           token_id == ids->QueryEndTag ||
           token_id == ids->OutputEndTag ||
           token_id == ids->ScriptEndTag);
}

static bool is_token_self_closing(const char *text, textparser_token_item *token) {
    if (token->len >= 2) {
        const char *end = text + token->position + token->len;
        while (end > text + token->position && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
            end--;
        }
        if (end - (text + token->position) >= 2 && end[-2] == '/' && end[-1] == '>') {
            return true;
        }
    }
    return false;
}

static const cfml_tag_info *find_tag_info(const char *tag_name, size_t tag_len) {
    for (int i = 0; i < cfml_tag_count; i++) {
        if (strncmp(tag_name, cfml_tags[i].name, tag_len) == 0 && cfml_tags[i].name[tag_len] == '\0') {
            return &cfml_tags[i];
        }
    }
    return nullptr;
}

static bool has_matching_end_tag(const cfml_dynamic_token_ids *ids, const char *text, textparser_token_item *start_token, const char *tag_name, size_t tag_len) {
    int nesting = 1;
    textparser_token_item *curr = start_token->next;
    while (curr != nullptr) {
        if (is_start_tag_token(ids, curr->token_id)) {
            const char *curr_name = text + curr->position + 1;
            size_t max_len = curr->len > 1 ? curr->len - 1 : 0;
            size_t curr_len = 0;
            while (curr_len < max_len && curr_name[curr_len] &&
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                if (!is_token_self_closing(text, curr)) {
                    nesting++;
                }
            }
        }
        else if (is_end_tag_token(ids, curr->token_id)) {
            const char *curr_name = text + curr->position + 2;
            size_t max_len = curr->len > 2 ? curr->len - 2 : 0;
            size_t curr_len = 0;
            while (curr_len < max_len && curr_name[curr_len] &&
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                nesting--;
                if (nesting == 0) {
                    return true;
                }
            }
        }
        curr = curr->next;
    }
    return false;
}

static bool has_matching_start_tag(const cfml_dynamic_token_ids *ids, const char *text, textparser_token_item *end_token, const char *tag_name, size_t tag_len) {
    int nesting = 1;
    textparser_token_item *curr = end_token->prev;
    while (curr != nullptr) {
        if (is_end_tag_token(ids, curr->token_id)) {
            const char *curr_name = text + curr->position + 2;
            size_t max_len = curr->len > 2 ? curr->len - 2 : 0;
            size_t curr_len = 0;
            while (curr_len < max_len && curr_name[curr_len] &&
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                nesting++;
            }
        }
        else if (is_start_tag_token(ids, curr->token_id)) {
            const char *curr_name = text + curr->position + 1;
            size_t max_len = curr->len > 1 ? curr->len - 1 : 0;
            size_t curr_len = 0;
            while (curr_len < max_len && curr_name[curr_len] &&
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                if (!is_token_self_closing(text, curr)) {
                    nesting--;
                    if (nesting == 0) {
                        return true;
                    }
                }
            }
        }
        curr = curr->prev;
    }
    return false;
}

static const cfml_function_info *find_function_info(const char *func_name, size_t func_len) {
    for (int i = 0; i < cfml_function_count; i++) {
        if (strncasecmp(func_name, cfml_functions[i].name, func_len) == 0 && cfml_functions[i].name[func_len] == '\0') {
            return &cfml_functions[i];
        }
    }
    return nullptr;
}

static void textparser_validate_cfml_token(const cfml_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token)
{
    const char *text = textparser_get_text(handle);

    if (is_start_tag_token(ids, token->token_id)) {
        // Get tag name and length
        const char *tag_name = text + token->position + 1;
        size_t max_len = token->len > 1 ? token->len - 1 : 0;
        size_t tag_len = 0;
        while (tag_len < max_len && tag_name[tag_len] &&
               ((tag_name[tag_len] >= 'a' && tag_name[tag_len] <= 'z') ||
                (tag_name[tag_len] >= 'A' && tag_name[tag_len] <= 'Z') ||
                (tag_name[tag_len] >= '0' && tag_name[tag_len] <= '9') ||
                tag_name[tag_len] == '_')) {
            tag_len++;
        }

        if (tag_len == 21 && strncasecmp(tag_name, "cfprocessingdirective", 21) == 0) {
            if (token->position > 4096 || (token->position + token->len) > 4096) {
                char *str = dynamic_printf("cfprocessingdirective should be located within first 4096 bytes of the file");
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
            }
        }

        // Check if tag exists in the cfml_tags list
        const cfml_tag_info *info = find_tag_info(tag_name, tag_len);
        if (info == nullptr) {
            char *token_name = alloca(tag_len + 1);
            strncpy(token_name, tag_name, tag_len);
            token_name[tag_len] = '\0';
            char *str = dynamic_printf("Unknown CFML tag: [%s]", token_name);

            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, tag_len);
            return;
        }

        // Check if needs closing tag
        if (info->end_tag_type == CFML_END_TAG_REQUIRED) {
            if (!is_token_self_closing(text, token)) {
                if (!has_matching_end_tag(ids, text, token, tag_name, tag_len)) {
                    char *token_name = alloca(tag_len + 1);
                    strncpy(token_name, tag_name, tag_len);
                    token_name[tag_len] = '\0';
                    char *str = dynamic_printf("CFML tag [%s] requires a closing tag </%s>", token_name, token_name);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
                }
            }
        }
    }
    else if (is_end_tag_token(ids, token->token_id)) {
        // Get tag name and length (skipping </)
        const char *tag_name = text + token->position + 2;
        size_t max_len = token->len > 2 ? token->len - 2 : 0;
        size_t tag_len = 0;
        while (tag_len < max_len && tag_name[tag_len] &&
               ((tag_name[tag_len] >= 'a' && tag_name[tag_len] <= 'z') ||
                (tag_name[tag_len] >= 'A' && tag_name[tag_len] <= 'Z') ||
                (tag_name[tag_len] >= '0' && tag_name[tag_len] <= '9') ||
                tag_name[tag_len] == '_')) {
            tag_len++;
        }

        if (tag_len == 21 && strncasecmp(tag_name, "cfprocessingdirective", 21) == 0) {
            if (token->position > 4096 || (token->position + token->len) > 4096) {
                char *str = dynamic_printf("cfprocessingdirective should be located within first 4096 bytes of the file");
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
            }
        }

        const cfml_tag_info *info = find_tag_info(tag_name, tag_len);
        if (info != nullptr) {
            // Check if ending tag is forbidden
            if (info->end_tag_type == CFML_END_TAG_FORBIDDEN) {
                char *token_name = alloca(tag_len + 1);
                strncpy(token_name, tag_name, tag_len);
                token_name[tag_len] = '\0';
                char *str = dynamic_printf("Ending tag </%s> is forbidden", token_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
                return;
            }
        }

        // Check if end tag has its own corresponding start tag
        if (!has_matching_start_tag(ids, text, token, tag_name, tag_len)) {
            char *token_name = alloca(tag_len + 1);
            strncpy(token_name, tag_name, tag_len);
            token_name[tag_len] = '\0';
            char *str = dynamic_printf("Ending tag </%s> has no matching start tag", token_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
        }
    }
    else if (token->token_id != TextParser_END && token->token_id == ids->Function) {
        // Validate function name
        const char *func_name = text + token->position;
        size_t func_len = 0;
        while (func_len < token->len &&
               ((func_name[func_len] >= 'a' && func_name[func_len] <= 'z') ||
                (func_name[func_len] >= 'A' && func_name[func_len] <= 'Z') ||
                (func_name[func_len] >= '0' && func_name[func_len] <= '9') ||
                func_name[func_len] == '_')) {
            func_len++;
        }

        const cfml_function_info *fn_info = find_function_info(func_name, func_len);
        if (fn_info == nullptr) {
            char *token_name = alloca(func_len + 1);
            strncpy(token_name, func_name, func_len);
            token_name[func_len] = '\0';
            char *str = dynamic_printf("Unknown CFML function: [%s]", token_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, func_len);
        } else {
            // Find the Parenthesis token following this function name
            textparser_token_item *paren_token = token->next;
            while (paren_token != nullptr && paren_token->token_id != TextParser_END &&
                   (paren_token->token_id == ids->ScriptLineComment ||
                    paren_token->token_id == ids->ScriptBlockComment)) {
                paren_token = paren_token->next;
            }

            if (paren_token != nullptr && paren_token->token_id != TextParser_END && paren_token->token_id == ids->Parenthesis) {
                int arg_count = 0;
                textparser_token_item *paren_child = paren_token->child;
                if (paren_child != nullptr) {
                    if (paren_child->token_id != TextParser_END && (paren_child->token_id == ids->Expression ||
                        paren_child->token_id == ids->ScriptExpression)) {
                        textparser_token_item *arg_item = paren_child->child;
                        bool has_non_comment_children = false;
                        int separator_count = 0;
                        while (arg_item != nullptr) {
                            if (arg_item->token_id != TextParser_END && arg_item->token_id == ids->Separator) {
                                separator_count++;
                            } else if (arg_item->token_id != TextParser_END && arg_item->token_id != ids->ScriptLineComment &&
                                       arg_item->token_id != ids->ScriptBlockComment) {
                                has_non_comment_children = true;
                            }
                            arg_item = arg_item->next;
                        }
                        if (has_non_comment_children) {
                            arg_count = separator_count + 1;
                        }
                    } else {
                        if (paren_child->token_id != TextParser_END && paren_child->token_id != ids->ScriptLineComment &&
                            paren_child->token_id != ids->ScriptBlockComment) {
                            arg_count = 1;
                        }
                    }
                }

                // Count total and required parameters
                int min_params = 0;
                int max_params = 0;
                if (fn_info->parameters != nullptr) {
                    const cfml_function_parameter_info *param = fn_info->parameters;
                    while (param->name != nullptr) {
                        max_params++;
                        if (param->required) {
                            min_params++;
                        }
                        param++;
                    }
                }

                if (arg_count < min_params) {
                    char *token_name = alloca(func_len + 1);
                    strncpy(token_name, func_name, func_len);
                    token_name[func_len] = '\0';
                    char *str = dynamic_printf("Function [%s] requires at least %d arguments, but %d were provided", token_name, min_params, arg_count);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, func_len);
                } else if (arg_count > max_params) {
                    char *token_name = alloca(func_len + 1);
                    strncpy(token_name, func_name, func_len);
                    token_name[func_len] = '\0';
                    char *str = dynamic_printf("Function [%s] takes at most %d arguments, but %d were provided", token_name, max_params, arg_count);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, func_len);
                }
            }
        }
    }
}

enum textparser_encoding textparser_get_encoding_cfml(textparser_t handle) {
    // https://helpx.adobe.com/coldfusion/cfml-reference/coldfusion-tags/tags-p-q/cfprocessingdirective.html
    //
    // Implementation Logic for Encoding Detection:
    // NOTE: BOM detection (for UTF-8, UTF-16, UTF-32) is already handled in
    // textparser_openfile when loading the file into memory.
    // 
    // ASCII-compatible Scan for <cfprocessingdirective pageEncoding="...">:
    // - If there is no BOM, the parser assumes the file is in an ASCII-compatible encoding
    //   (like UTF-8, Latin-1/ISO-8859-1, Big5, Shift-JIS, GBK).
    // - In all ASCII-compatible encodings, the bytes representing English letters and tags
    //   are identical to standard ASCII (values below 0x80).
    // - Thus, the parser scans the first 4KB of the raw file byte-by-byte looking for the
    //   case-insensitive string "cfprocessingdirective" and extracts the "pageEncoding" value.
    // - Once resolved, the parser resets and re-reads the file in that encoding.
    //
    // BOM Requirement for Non-ASCII Encodings:
    // If the file is in a non-ASCII-compatible encoding (like UTF-16 or UTF-32) but lacks a BOM,
    // it will fail to be decoded correctly before reaching this check because a UTF-16/32
    // tag like <cfprocessingdirective> would contain interleaved null bytes and fail to match
    // during the initial ASCII-compatible scan. A BOM is required for the file-loader
    // (textparser_openfile) to decode it before searching for the tag.

    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    if (!text || text_size == 0) {
        return TEXTPARSER_ENCODING_UTF_8;
    }

    size_t limit = text_size < 4096 ? text_size : 4096;
    const char *directive_tag = "cfprocessingdirective";
    size_t tag_len = strlen(directive_tag);

    for (size_t i = 0; i < limit; i++) {
        if (text[i] == '<') {
            size_t pos = i + 1;
            while (pos < limit && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\r' || text[pos] == '\n')) {
                pos++;
            }
            if (pos + tag_len <= limit && strncasecmp(&text[pos], directive_tag, tag_len) == 0) {
                pos += tag_len;
                // Ensure tag name boundary
                if (pos < limit && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\r' || text[pos] == '\n' || text[pos] == '>' || text[pos] == '/')) {
                    // Search within tag attributes until '>' or end of scan limit
                    while (pos < limit && text[pos] != '>') {
                        const char *attr_name = "pageencoding";
                        size_t attr_len = strlen(attr_name);
                        if (pos + attr_len <= limit && strncasecmp(&text[pos], attr_name, attr_len) == 0) {
                            size_t eq_pos = pos + attr_len;
                            while (eq_pos < limit && (text[eq_pos] == ' ' || text[eq_pos] == '\t' || text[eq_pos] == '\r' || text[eq_pos] == '\n')) {
                                eq_pos++;
                            }
                            if (eq_pos < limit && text[eq_pos] == '=') {
                                size_t val_pos = eq_pos + 1;
                                while (val_pos < limit && (text[val_pos] == ' ' || text[val_pos] == '\t' || text[val_pos] == '\r' || text[val_pos] == '\n')) {
                                    val_pos++;
                                }
                                if (val_pos < limit) {
                                    char quote = '\0';
                                    if (text[val_pos] == '"' || text[val_pos] == '\'') {
                                        quote = text[val_pos];
                                        val_pos++;
                                    }
                                    size_t val_start = val_pos;
                                    size_t val_end = val_start;
                                    if (quote != '\0') {
                                        while (val_end < limit && text[val_end] != quote && text[val_end] != '>' && text[val_end] != '\r' && text[val_end] != '\n') {
                                            val_end++;
                                        }
                                    } else {
                                        while (val_end < limit && text[val_end] != ' ' && text[val_end] != '\t' && text[val_end] != '\r' && text[val_end] != '\n' && text[val_end] != '>' && text[val_end] != '/') {
                                            val_end++;
                                        }
                                    }

                                    size_t enc_len = val_end - val_start;
                                    if (enc_len > 0) {
                                        char enc_buf[64];
                                        if (enc_len >= sizeof(enc_buf)) {
                                            enc_len = sizeof(enc_buf) - 1;
                                        }
                                        memcpy(enc_buf, &text[val_start], enc_len);
                                        enc_buf[enc_len] = '\0';

                                        if (strcasecmp(enc_buf, "utf-8") == 0 || strcasecmp(enc_buf, "utf8") == 0) {
                                            return TEXTPARSER_ENCODING_UTF_8;
                                        } else if (strcasecmp(enc_buf, "utf-16") == 0 || strcasecmp(enc_buf, "utf16") == 0 || strcasecmp(enc_buf, "utf-16le") == 0 || strcasecmp(enc_buf, "utf-16be") == 0) {
                                            return TEXTPARSER_ENCODING_UTF_16;
                                        } else if (strcasecmp(enc_buf, "utf-32") == 0 || strcasecmp(enc_buf, "utf32") == 0 || strcasecmp(enc_buf, "utf-32le") == 0 || strcasecmp(enc_buf, "utf-32be") == 0) {
                                            return TEXTPARSER_ENCODING_UTF_32;
                                        } else if (strcasecmp(enc_buf, "iso-8859-1") == 0 || strcasecmp(enc_buf, "latin1") == 0 || strcasecmp(enc_buf, "latin-1") == 0 || strcasecmp(enc_buf, "us-ascii") == 0 || strcasecmp(enc_buf, "ascii") == 0) {
                                            return TEXTPARSER_ENCODING_LATIN1;
                                        }
                                    }
                                }
                            }
                        }
                        pos++;
                    }
                }
            }
        }
    }

    return TEXTPARSER_ENCODING_UTF_8;
}

static void textparser_validate_cfml_tree(const cfml_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token, int depth) {
    if (depth >= MAX_RECURSION_DEPTH) {
        return;
    }
    while (token != nullptr) {
        textparser_validate_cfml_token(ids, ret, handle, token);
        if (token->child != nullptr) {
            textparser_validate_cfml_tree(ids, ret, handle, token->child, depth + 1);
        }
        token = token->next;
    }
}

textparser_validation *textparser_validate_cfml(textparser_t handle) {
    cfml_dynamic_token_ids ids;
    resolve_cfml_token_ids(handle, &ids);

    textparser_validation *ret = nullptr;
    textparser_token_item *token = textparser_get_first_token(handle);
    textparser_validate_cfml_tree(&ids, &ret, handle, token, 0);
    return ret;
}

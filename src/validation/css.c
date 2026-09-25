#include "css.h"
#include "css_tags.h"
#include "validation.h"

#include <textparser.h>
#include <css_definition.json.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


typedef struct {
    int AtRule;
    int PseudoClass;
    int Declaration;

    int AtKeyword;
    int Identifier;
    int CustomProperty;
    int Colon;
    int Semicolon;
    int LBrace;
    int RBrace;
    int LParen;
    int RParen;
    int LBracket;
    int RBracket;
} css_dynamic_token_ids;

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

static void resolve_css_token_ids(textparser_t handle, css_dynamic_token_ids *ids) {
    ids->AtRule = find_token_id_by_name(handle, "AtRule");
    ids->PseudoClass = find_token_id_by_name(handle, "PseudoClass");
    ids->Declaration = find_token_id_by_name(handle, "Declaration");

    ids->AtKeyword = find_token_id_by_name(handle, "AtKeyword");
    ids->Identifier = find_token_id_by_name(handle, "Identifier");
    ids->CustomProperty = find_token_id_by_name(handle, "CustomProperty");
    ids->Colon = find_token_id_by_name(handle, "Colon");
    ids->Semicolon = find_token_id_by_name(handle, "Semicolon");
    ids->LBrace = find_token_id_by_name(handle, "LBrace");
    ids->RBrace = find_token_id_by_name(handle, "RBrace");
    ids->LParen = find_token_id_by_name(handle, "LParen");
    ids->RParen = find_token_id_by_name(handle, "RParen");
    ids->LBracket = find_token_id_by_name(handle, "LBracket");
    ids->RBracket = find_token_id_by_name(handle, "RBracket");
}

static int compare_strings(const void *a, const void *b) {
    const char *str_a = *(const char *const *)a;
    const char *str_b = *(const char *const *)b;
    return strcmp(str_a, str_b);
}

static bool is_css_property(const char *name) {
    if (strncmp(name, "--", 2) == 0) return true;
    
    if (strncmp(name, "-webkit-", 8) == 0 ||
        strncmp(name, "-moz-", 5) == 0 ||
        strncmp(name, "-ms-", 4) == 0 ||
        strncmp(name, "-o-", 3) == 0) {
        return true;
    }
    
    return bsearch(&name, css_properties, css_property_count, sizeof(char *), compare_strings) != NULL;
}

static bool is_css_pseudo_class(const char *name) {
    if (strncmp(name, ":-webkit-", 9) == 0 ||
        strncmp(name, ":-moz-", 6) == 0 ||
        strncmp(name, ":-ms-", 5) == 0 ||
        strncmp(name, ":-o-", 4) == 0) {
        return true;
    }
    return bsearch(&name, css_pseudo_classes, css_pseudo_class_count, sizeof(char *), compare_strings) != NULL;
}

static bool is_css_pseudo_element(const char *name) {
    if (strncmp(name, "::-webkit-", 10) == 0 ||
        strncmp(name, "::-moz-", 7) == 0 ||
        strncmp(name, "::-ms-", 6) == 0 ||
        strncmp(name, "::-o-", 5) == 0) {
        return true;
    }
    return bsearch(&name, css_pseudo_elements, css_pseudo_element_count, sizeof(char *), compare_strings) != NULL;
}

static bool is_css_at_rule(const char *name) {
    if (strncmp(name, "@-webkit-", 9) == 0 ||
        strncmp(name, "@-moz-", 6) == 0 ||
        strncmp(name, "@-ms-", 5) == 0 ||
        strncmp(name, "@-o-", 4) == 0) {
        return true;
    }
    return bsearch(&name, css_at_rules, css_at_rule_count, sizeof(char *), compare_strings) != NULL;
}

static void get_property_name(const char *text, textparser_token_item *token, char *buf, size_t max_len) {
    size_t token_pos = textparser_get_token_position(token);
    size_t p = token_pos;
    size_t end = token_pos + token->len;
    size_t i = 0;
    while (p < end && i + 1 < max_len) {
        char c = text[p];
        if (c == ':' || c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            break;
        }
        buf[i++] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
        p++;
    }
    buf[i] = '\0';
}

static void get_token_name_lower(const char *text, textparser_token_item *token, char *buf, size_t max_len) {
    size_t token_pos = textparser_get_token_position(token);
    size_t len = token->len < max_len - 1 ? token->len : max_len - 1;
    for (size_t i = 0; i < len; i++) {
        char c = text[token_pos + i];
        buf[i] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    }
    buf[len] = '\0';
}

static void textparser_validate_css_token(const css_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token) {
    const char *text = textparser_get_text(handle);
    size_t token_pos = textparser_get_token_position(token);
    if (token->token_id != TextParser_END && token->token_id == ids->Declaration) {
        char prop_name[128];
        get_property_name(text, token, prop_name, sizeof(prop_name));

        if (!is_css_property(prop_name)) {
            char *str = dynamic_printf("Unknown CSS property: [%s]", prop_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token_pos, strlen(prop_name));
        }
    }
    else if (token->token_id != TextParser_END && token->token_id == ids->PseudoClass) {
        char pseudo_name[128];
        get_token_name_lower(text, token, pseudo_name, sizeof(pseudo_name));

        if (strncmp(pseudo_name, "::", 2) == 0) {
            if (!is_css_pseudo_element(pseudo_name)) {
                char *str = dynamic_printf("Unknown CSS pseudo-element: [%s]", pseudo_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token_pos, token->len);
            }
        } else {
            if (!is_css_pseudo_class(pseudo_name)) {
                char *str = dynamic_printf("Unknown CSS pseudo-class: [%s]", pseudo_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token_pos, token->len);
            }
        }
    }
    else if (token->token_id != TextParser_END && token->token_id == ids->AtRule) {
        char at_name[128];
        get_token_name_lower(text, token, at_name, sizeof(at_name));

        if (!is_css_at_rule(at_name)) {
            char *str = dynamic_printf("Unknown CSS At-Rule: [%s]", at_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token_pos, token->len);
        }
    }
}

static void textparser_validate_css_tree(const css_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle, textparser_token_item *token, int depth) {
    if (depth >= MAX_RECURSION_DEPTH) {
        return;
    }
    while (token != nullptr) {
        textparser_validate_css_token(ids, ret, handle, token);
        if (token->child != nullptr) {
            textparser_validate_css_tree(ids, ret, handle, token->child, depth + 1);
        }
        token = token->next;
    }
}

static size_t get_token_start(textparser_token_item *token) {
    if ((token->node_flags & TEXTPARSER_NODE_EXPLICIT_SPAN) != 0) {
        return token->source_start;
    }
    return textparser_get_token_position(token);
}

static bool is_trivia_token(textparser_token_item *tok) {
    if (tok == NULL) return false;
    if (tok->token_id < 0) return true;
    if ((tok->node_flags & TEXTPARSER_NODE_TRIVIA) != 0) return true;
    return false;
}

static textparser_token_item *next_non_trivia(textparser_token_item *tok) {
    if (tok == NULL) return NULL;
    tok = tok->next;
    while (tok != NULL && is_trivia_token(tok)) {
        tok = tok->next;
    }
    return tok;
}

static int find_statement_terminator(const css_dynamic_token_ids *ids, textparser_token_item *tok) {
    int parens = 0;
    int brackets = 0;
    while (tok != NULL) {
        if (tok->token_id == ids->LParen) parens++;
        else if (tok->token_id == ids->RParen) { if (parens > 0) parens--; }
        else if (tok->token_id == ids->LBracket) brackets++;
        else if (tok->token_id == ids->RBracket) { if (brackets > 0) brackets--; }
        else if (parens == 0 && brackets == 0) {
            if (tok->token_id == ids->Semicolon || tok->token_id == ids->RBrace) return 1;
            if (tok->token_id == ids->LBrace) return 2;
        }
        tok = tok->next;
    }
    return 0;
}

static void textparser_validate_css_v2(const css_dynamic_token_ids *ids, textparser_validation **ret, textparser_t handle) {
    const char *text = textparser_get_text(handle);
    if (text == NULL) return;

    int brace_depth = 0;
    int paren_depth = 0;
    textparser_token_item *tok = textparser_get_first_token(handle);
    textparser_token_item *prev_meaningful = NULL;

    while (tok != NULL) {
        if (is_trivia_token(tok)) {
            tok = tok->next;
            continue;
        }

        if (tok->token_id == ids->LParen) {
            paren_depth++;
        } else if (tok->token_id == ids->RParen) {
            if (paren_depth > 0) paren_depth--;
        } else if (tok->token_id == ids->LBrace) {
            brace_depth++;
        } else if (tok->token_id == ids->RBrace) {
            if (brace_depth > 0) brace_depth--;
        } else if (paren_depth == 0) {
            /* 1. At-Rules */
            if (ids->AtKeyword != TextParser_END && tok->token_id == ids->AtKeyword) {
                char at_name[128];
                size_t pos = get_token_start(tok);
                size_t len = tok->len < sizeof(at_name) - 1 ? tok->len : sizeof(at_name) - 1;
                for (size_t i = 0; i < len; i++) {
                    char c = text[pos + i];
                    at_name[i] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
                }
                at_name[len] = '\0';

                if (!is_css_at_rule(at_name)) {
                    char *str = dynamic_printf("Unknown CSS At-Rule: [%s]", at_name);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, pos, tok->len);
                }
            }
            /* 2. Property Declarations */
            else if (brace_depth > 0 && (tok->token_id == ids->Identifier || tok->token_id == ids->CustomProperty)) {
                textparser_token_item *after = next_non_trivia(tok);
                if (after != NULL && after->token_id == ids->Colon) {
                    int term = find_statement_terminator(ids, after->next);
                    if (term == 1) {
                        char prop_name[128];
                        size_t pos = get_token_start(tok);
                        size_t len = tok->len < sizeof(prop_name) - 1 ? tok->len : sizeof(prop_name) - 1;
                        for (size_t i = 0; i < len; i++) {
                            char c = text[pos + i];
                            prop_name[i] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
                        }
                        prop_name[len] = '\0';

                        if (!is_css_property(prop_name)) {
                            char *str = dynamic_printf("Unknown CSS property: [%s]", prop_name);
                            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, pos, tok->len);
                        }
                    }
                }
            }
            /* 3. Pseudo-Classes and Pseudo-Elements */
            else if (tok->token_id == ids->Colon) {
                if (prev_meaningful == NULL || prev_meaningful->token_id != ids->Colon) {
                    textparser_token_item *next1 = next_non_trivia(tok);
                    if (next1 != NULL) {
                        if (next1->token_id == ids->Colon) {
                            textparser_token_item *ident = next_non_trivia(next1);
                            if (ident != NULL && ident->token_id == ids->Identifier) {
                                size_t start_pos = get_token_start(tok);
                                size_t end_pos = get_token_start(ident) + ident->len;
                                size_t span_len = end_pos - start_pos;
                                char pseudo_name[128];
                                size_t copy_len = span_len < sizeof(pseudo_name) - 1 ? span_len : sizeof(pseudo_name) - 1;
                                for (size_t i = 0; i < copy_len; i++) {
                                    char c = text[start_pos + i];
                                    pseudo_name[i] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
                                }
                                pseudo_name[copy_len] = '\0';

                                if (!is_css_pseudo_element(pseudo_name)) {
                                    char *str = dynamic_printf("Unknown CSS pseudo-element: [%s]", pseudo_name);
                                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, start_pos, span_len);
                                }
                            }
                        } else if (next1->token_id == ids->Identifier) {
                            int term = find_statement_terminator(ids, next1->next);
                            if (term == 2) {
                                size_t start_pos = get_token_start(tok);
                                size_t end_pos = get_token_start(next1) + next1->len;
                                size_t span_len = end_pos - start_pos;
                                char pseudo_name[128];
                                size_t copy_len = span_len < sizeof(pseudo_name) - 1 ? span_len : sizeof(pseudo_name) - 1;
                                for (size_t i = 0; i < copy_len; i++) {
                                    char c = text[start_pos + i];
                                    pseudo_name[i] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
                                }
                                pseudo_name[copy_len] = '\0';

                                if (!is_css_pseudo_class(pseudo_name)) {
                                    char *str = dynamic_printf("Unknown CSS pseudo-class: [%s]", pseudo_name);
                                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, start_pos, span_len);
                                }
                            }
                        }
                    }
                }
            }
        }

        prev_meaningful = tok;
        tok = tok->next;
    }
}

textparser_validation *textparser_validate_css(textparser_t handle) {
    css_dynamic_token_ids ids;
    resolve_css_token_ids(handle, &ids);

    textparser_validation *ret = nullptr;
    textparser_token_item *token = textparser_get_first_token(handle);

    if (ids.Declaration != TextParser_END || ids.AtRule != TextParser_END) {
        textparser_validate_css_tree(&ids, &ret, handle, token, 0);
    } else {
        textparser_validate_css_v2(&ids, &ret, handle);
    }

    return ret;
}


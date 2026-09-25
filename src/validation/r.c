#include "r.h"
#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define R_MAX_PARAMS 256

typedef struct {
    int loop_depth;
} r_context;

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

static bool is_na_or_nan(textparser_t handle, const textparser_node *node) {
    if (!node) return false;
    const textparser_node *term = textparser_node_first_terminal(node);
    if (!term) return false;
    return is_node(handle, term, "KwNa") ||
           is_node(handle, term, "KwNan") ||
           is_node(handle, term, "KwNaInteger") ||
           is_node(handle, term, "KwNaReal") ||
           is_node(handle, term, "KwNaComplex") ||
           is_node(handle, term, "KwNaCharacter");
}

static bool is_literal(textparser_t handle, const textparser_node *node) {
    if (!node) return false;
    const textparser_node *term = textparser_node_first_terminal(node);
    if (!term) return false;
    // If the expression has following tokens like LParen, LBracket, etc., it's not a bare literal
    if (term->next != NULL) return false;
    return is_node(handle, term, "DecNumber") ||
           is_node(handle, term, "HexNumber") ||
           is_node(handle, term, "FloatNumber") ||
           is_node(handle, term, "ComplexNumber") ||
           is_node(handle, term, "DoubleString") ||
           is_node(handle, term, "SingleString") ||
           is_node(handle, term, "RawString") ||
           is_node(handle, term, "KwTrue") ||
           is_node(handle, term, "KwFalse") ||
           is_node(handle, term, "KwNull") ||
           is_node(handle, term, "KwNa") ||
           is_node(handle, term, "KwInf") ||
           is_node(handle, term, "KwNan");
}

static void collect_parameters(textparser_t handle, const textparser_node *node,
                               const textparser_node **params, size_t *count, size_t max_params) {
    if (!node || *count >= max_params) return;
    if (is_node(handle, node, "Parameter") || is_node(handle, node, "DotDotDot")) {
        params[(*count)++] = node;
        return;
    }
    for (const textparser_node *c = node->child; c; c = c->next) {
        collect_parameters(handle, c, params, count, max_params);
    }
}

static const textparser_node *find_first_child_of_type(textparser_t handle, const textparser_node *node, const char *type_name) {
    if (!node) return NULL;
    if (is_node(handle, node, type_name)) return node;
    for (const textparser_node *c = node->child; c; c = c->next) {
        const textparser_node *found = find_first_child_of_type(handle, c, type_name);
        if (found) return found;
    }
    return NULL;
}

static void validate_function_parameters(textparser_t handle, const textparser_node *func_node) {
    const textparser_node *params[R_MAX_PARAMS];
    size_t param_count = 0;
    collect_parameters(handle, func_node, params, &param_count, R_MAX_PARAMS);

    char *seen_names[R_MAX_PARAMS] = {0};
    size_t seen_count = 0;
    int dots_count = 0;

    for (size_t i = 0; i < param_count; i++) {
        const textparser_node *p = params[i];
        if (is_node(handle, p, "DotDotDot") || find_first_child_of_type(handle, p, "DotDotDot") != NULL) {
            dots_count++;
            if (dots_count > 1) {
                report(handle, p, "R2002", "Repeated formal argument '...'");
            }
            continue;
        }

        const textparser_node *ident_node = find_first_child_of_type(handle, p, "Identifier");
        if (!ident_node) {
            ident_node = find_first_child_of_type(handle, p, "BacktickIdentifier");
        }
        if (!ident_node) {
            ident_node = find_first_child_of_type(handle, p, "MemberIdentifier");
        }

        if (ident_node) {
            size_t name_len = 0;
            char *name = node_text(handle, ident_node, &name_len);
            if (name) {
                bool duplicate = false;
                for (size_t j = 0; j < seen_count; j++) {
                    if (seen_names[j] && strcmp(seen_names[j], name) == 0) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Repeated formal argument '%s'", name);
                        report(handle, ident_node, "R2001", msg);
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate && seen_count < R_MAX_PARAMS) {
                    seen_names[seen_count++] = name;
                } else {
                    textparser_free_token_text(name);
                }
            }
        }
    }

    for (size_t i = 0; i < seen_count; i++) {
        if (seen_names[i]) textparser_free_token_text(seen_names[i]);
    }
}

static void check_r_node(textparser_t handle, const textparser_node *node, r_context *ctx) {
    if (!node) return;

    // Check FunctionDefinition and LambdaFunction
    if (is_node(handle, node, "FunctionDefinition") || is_node(handle, node, "LambdaFunction")) {
        validate_function_parameters(handle, node);
        int saved_depth = ctx->loop_depth;
        ctx->loop_depth = 0; // loops outside function cannot be broken from inside
        for (const textparser_node *child = node->child; child; child = child->next) {
            check_r_node(handle, child, ctx);
        }
        ctx->loop_depth = saved_depth;
        return;
    }

    // Check Loops
    if (is_node(handle, node, "ForExpression") ||
        is_node(handle, node, "WhileExpression") ||
        is_node(handle, node, "RepeatExpression")) {
        ctx->loop_depth++;
        for (const textparser_node *child = node->child; child; child = child->next) {
            check_r_node(handle, child, ctx);
        }
        ctx->loop_depth--;
        return;
    }

    // Check Break and Next
    if (is_node(handle, node, "KwBreak") || is_node(handle, node, "KwNext")) {
        if (ctx->loop_depth == 0) {
            report(handle, node, "R2003", "No loop for break/next");
        }
    }

    // Check Pratt binary operators
    if (is_node(handle, node, "Equal") || is_node(handle, node, "NotEqual")) {
        const textparser_node *left = node->child;
        const textparser_node *right = left ? left->next : NULL;
        if (is_na_or_nan(handle, left) || is_na_or_nan(handle, right)) {
            report_warning(handle, node, "R2004",
                           "Use is.na() or is.nan() rather than comparing directly with NA or NaN");
        }
    }

    if (is_node(handle, node, "LeftAssign") ||
        is_node(handle, node, "LeftSuperAssign") ||
        is_node(handle, node, "Assign")) {
        const textparser_node *left = node->child;
        if (is_literal(handle, left)) {
            report(handle, left, "R2005", "Invalid left-hand side in assignment");
        }
    }

    if (is_node(handle, node, "RightAssign") || is_node(handle, node, "RightSuperAssign")) {
        const textparser_node *left = node->child;
        const textparser_node *right = left ? left->next : NULL;
        if (is_literal(handle, right)) {
            report(handle, right ? right : node, "R2006",
                   "Invalid right-hand side in right-assignment");
        }
    }

    for (const textparser_node *child = node->child; child; child = child->next) {
        check_r_node(handle, child, ctx);
    }
}

static void validate_r_ast(textparser_t handle, const textparser_node *root) {
    r_context ctx = {0};
    check_r_node(handle, root, &ctx);
}

static textparser_action r_source_complete(textparser_t handle,
                                           const textparser_event *event,
                                           void *user_data) {
    (void)user_data;
    if (event && event->node) {
        for (const textparser_node *item = event->node; item; item = item->next) {
            validate_r_ast(handle, item);
        }
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_R int textparser_r_register_validators(textparser_t handle) {
    if (!handle) return -1;
    return textparser_register_handler(handle, "r.legality", r_source_complete, NULL);
}

EXPORT_R textparser_validation *textparser_validate_r(textparser_t handle) {
    if (handle == NULL) return NULL;

    size_t diag_count = textparser_get_diagnostic_count(handle);
    if (diag_count == 0) {
        const textparser_node *root = textparser_get_first_token(handle);
        if (root) {
            validate_r_ast(handle, root);
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
                                       diag.code ? diag.code : "R",
                                       diag.message ? diag.message : "");
            textparser_validation_item_add(type, &validation, msg, diag.start_pos, diag.length);
        }
    }
    return validation;
}

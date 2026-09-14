#include "php.h"
#include "php_common.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Validators use the public C grammar API, including for interpolation.
 * No PHP-specific behavior is added to the parser runtime. */
#define PHP_VALIDATION_DEPTH 128

static bool kind(const textparser_node *node, const char *name) {
    return node != nullptr && node->cst_kind != nullptr &&
        strcmp(node->cst_kind, name) == 0;
}

static const textparser_node *child_at(const textparser_node *node, size_t index) {
    const textparser_node *child = node ? node->child : nullptr;
    while (child && index--) child = child->next;
    return child;
}

/* The text getters use legacy sibling offsets. Supply a detached view of the
 * explicit grammar span rather than the grammar node's scaffolding siblings. */
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

static bool writable(textparser_t handle, const textparser_node *node, bool pattern, unsigned depth);

static bool writable_base(textparser_t handle, const textparser_node *node, unsigned depth) {
    if (depth > PHP_VALIDATION_DEPTH || !node) return false;
    if (kind(node, "ParenthesizedExpression"))
        return writable_base(handle, child_at(node, 1), depth + 1);
    if (kind(node, "CallSuffix")) {
        /* Nullsafe receivers make the whole write chain invalid, but nullsafe
         * expressions in an argument or index do not. */
        const textparser_node *callee = node->child;
        while (callee && (kind(callee, "CallSuffix") || kind(callee, "IndexSuffix") ||
                           kind(callee, "MemberAccessSuffix"))) {
            if (kind(callee, "MemberAccessSuffix") &&
                kind(child_at(callee, 1), "NullsafeMemberAccess")) return false;
            callee = callee->child;
        }
        return true;
    }
    return writable(handle, node, false, depth + 1);
}

static bool writable_pattern(textparser_t handle, const textparser_node *node, unsigned depth) {
    if (depth > PHP_VALIDATION_DEPTH) return false;
    for (const textparser_node *item = node; item; item = item->next) {
        if (kind(item, "ArrayElement")) {
            const textparser_node *value = item->child;
            if (kind(child_at(item, 1), "DoubleArrow")) value = child_at(item, 2);
            if (kind(value, "Sequence")) value = value->child;
            if (kind(value, "BitwiseAndOperator"))
                value = value->child ? value->child : value->next;
            if (!writable(handle, value, true, depth + 1)) return false;
        } else if (item->child && !writable_pattern(handle, item->child, depth + 1)) {
            return false;
        }
    }
    return true;
}

static bool writable(textparser_t handle, const textparser_node *node, bool pattern, unsigned depth) {
    if (!node || depth > PHP_VALIDATION_DEPTH) return false;
    if (kind(node, "Variable")) {
        size_t length = 0;
        char *text = node_text(handle, node, &length);
        bool result = text && !(length == 5 && memcmp(text, "$this", 5) == 0);
        textparser_free_token_text(text);
        return result;
    }
    if (kind(node, "VariableOrCall")) return writable(handle, node->child, pattern, depth + 1);
    if (kind(node, "DynamicVariable")) return true;
    if (kind(node, "IndexSuffix")) return writable_base(handle, node->child, depth + 1);
    if (kind(node, "MemberAccessSuffix")) {
        const textparser_node *access = child_at(node, 1);
        if (kind(access, "NullsafeMemberAccess")) return false;
        if (kind(access, "ScopeResolution")) return kind(child_at(node, 2), "Variable");
        /* $this cannot be reassigned, but its properties can be written. */
        const textparser_node *base = node->child;
        if (kind(base, "VariableOrCall") && kind(base->child, "Variable")) return true;
        return writable_base(handle, base, depth + 1);
    }
    if (pattern && (kind(node, "ArrayLiteral") || kind(node, "ListDestructuring")))
        return writable_pattern(handle, node->child, depth + 1);
    return false;
}

static bool comparison(const textparser_node *node, bool relational) {
    return relational ? kind(node, "CompareOperator") :
        kind(node, "EqualityOperator") || kind(node, "StrictCompareOperator") ||
        kind(node, "SpaceshipOperator");
}

static bool ternary(const textparser_node *node) {
    return kind(node, "Question") || kind(node, "ElvisOperator");
}

/* True when the array starts with a hole or has a non-final empty slot. A single
 * trailing comma (the final empty Sequence) is allowed. */
static bool array_has_empty_element(const textparser_node *elements) {
    if (!elements) return false;
    if (!kind(elements->child, "ArrayElement")) return true;
    const textparser_node *last = nullptr;
    for (const textparser_node *item = elements->child; item; item = item->next)
        if (kind(item, "Repeat"))
            for (const textparser_node *sequence = item->child; sequence; sequence = sequence->next)
                if (kind(sequence, "Sequence")) last = sequence;
    for (const textparser_node *item = elements->child; item; item = item->next) {
        if (!kind(item, "Repeat")) continue;
        for (const textparser_node *sequence = item->child; sequence; sequence = sequence->next) {
            if (!kind(sequence, "Sequence")) continue;
            bool empty = true;
            for (const textparser_node *part = sequence->child; part; part = part->next)
                if (kind(part, "ArrayElement")) { empty = false; break; }
            if (empty && sequence != last) return true;
        }
    }
    return false;
}

/* An array with holes is only legal as a destructuring target. Climb the explicit
 * grammar scaffolding to the enclosing ArrayLiteral and check whether it is an
 * assignment left-hand side or a `foreach` target (after `as`). */
static bool destructuring_target(const textparser_node *node) {
    const textparser_node *parent = node ? node->parent : nullptr;
    if (!parent) return false;
    if (kind(parent, "AssignOperator")) return parent->child == node;
    if (kind(parent, "ForEachStatement")) {
        bool after_as = false;
        for (const textparser_node *item = parent->child; item; item = item->next) {
            if (item == node) return after_as;
            if (kind(item, "AsKeyword")) after_as = true;
        }
        return false;
    }
    if (kind(parent, "ArrayElement") || kind(parent, "Sequence") ||
        kind(parent, "Repeat") || kind(parent, "ArrayElements") ||
        kind(parent, "ArrayLiteral") || kind(parent, "LongArrayLiteral"))
        return destructuring_target(parent);
    return false;
}

static void check_array_literal(textparser_t handle, const textparser_node *literal) {
    const textparser_node *elements = nullptr;
    for (const textparser_node *part = literal->child; part; part = part->next)
        if (kind(part, "ArrayElements")) { elements = part; break; }
    if (array_has_empty_element(elements) && !destructuring_target(literal))
        report(handle, literal, "PHP2009", "Cannot use empty array elements in arrays.");
}

/* Names declared with `function name(...)` in the source, case-folded. Calls to
 * them must not be checked against the bundled built-in signatures. */
typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} php_name_list;

static void name_list_add(php_name_list *list, const char *text, size_t length) {
    if (list->count == list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 16;
        char **items = realloc(list->items, capacity * sizeof(char *));
        if (!items) return;
        list->items = items;
        list->capacity = capacity;
    }
    char *copy = malloc(length + 1);
    if (!copy) return;
    for (size_t i = 0; i < length; ++i) copy[i] = (char)tolower((unsigned char)text[i]);
    copy[length] = '\0';
    list->items[list->count++] = copy;
}

static bool name_list_contains(const php_name_list *list, const char *text, size_t length) {
    for (size_t i = 0; i < list->count; ++i)
        if (strlen(list->items[i]) == length && strncmp(list->items[i], text, length) == 0)
            return true;
    return false;
}

static void name_list_free(php_name_list *list) {
    for (size_t i = 0; i < list->count; ++i) free(list->items[i]);
    free(list->items);
}

static void collect_declared_functions(textparser_t handle, const textparser_node *node, php_name_list *list) {
    for (const textparser_node *item = node; item; item = item->next) {
        if (kind(item, "FunctionDeclaration")) {
            bool keyword = false;
            for (const textparser_node *part = item->child; part; part = part->next) {
                if (kind(part, "FunctionKeyword")) { keyword = true; continue; }
                if (keyword && kind(part, "Identifier")) {
                    size_t length = 0;
                    char *text = node_text(handle, part, &length);
                    if (text) { name_list_add(list, text, length); textparser_free_token_text(text); }
                    break;
                }
            }
        }
        collect_declared_functions(handle, item->child, list);
    }
}

/* A global built-in call is an unqualified `foo` or a fully qualified `\foo`.
 * Methods, dynamic calls, namespaced calls, and parenthesized callees resolve at
 * runtime and are not checked. */
static const textparser_node *global_callee(const textparser_node *call) {
    const textparser_node *callee = call ? call->child : nullptr;
    if (!kind(callee, "VariableOrCall")) return nullptr;
    const textparser_node *name = callee->child;
    if (!kind(name, "QualifiedName")) return nullptr;
    const textparser_node *first = name->child;
    if (kind(first, "Identifier") && !first->next) return first;
    if (kind(first, "NamespaceSeparator") && kind(first->next, "Identifier") && !first->next->next)
        return first->next;
    return nullptr;
}

/* Count Argument nodes through the Repeat/Sequence scaffolding only, so nested
 * calls inside an argument are not counted again. */
static int argument_count(const textparser_node *node) {
    int count = 0;
    for (const textparser_node *item = node; item; item = item->next) {
        if (kind(item, "Argument")) ++count;
        else if (kind(item, "Repeat") || kind(item, "Sequence"))
            count += argument_count(item->child);
    }
    return count;
}

static int call_argument_count(const textparser_node *call) {
    for (const textparser_node *item = call->child; item; item = item->next)
        if (kind(item, "ArgumentList")) return argument_count(item->child);
    return 0;
}

static void check_call(textparser_t handle, const textparser_node *call, const php_name_list *declared) {
    /* `foo(...)` is a first-class callable, not a call; its placeholder spread is
     * a direct child (optionally under a Sequence) of CallSuffix, while unpacked
     * arguments sit in ArgumentList. */
    bool placeholder = false;
    for (const textparser_node *item = call->child; item; item = item->next) {
        if (kind(item, "ArgumentList")) { placeholder = false; break; }
        if (kind(item, "SpreadOperator") ||
            (kind(item, "Sequence") && kind(item->child, "SpreadOperator") && !item->child->next))
            placeholder = true;
    }
    if (placeholder) return;
    const textparser_node *name = global_callee(call);
    if (!name) return;
    size_t length = 0;
    char *text = node_text(handle, name, &length);
    if (!text) return;
    char *lower = malloc(length + 1);
    if (!lower) { textparser_free_token_text(text); return; }
    for (size_t i = 0; i < length; ++i) lower[i] = (char)tolower((unsigned char)text[i]);
    lower[length] = '\0';
    textparser_free_token_text(text);
    if (name_list_contains(declared, lower, length)) { free(lower); return; }
    const php_function_info *info = php_find_function(lower);
    if (!info) { free(lower); return; }
    char *error = php_function_arity_error(info, lower, call_argument_count(call));
    if (error) {
        report(handle, name, "PHP2008", error);
        free(error);
    }
    free(lower);
}

static void check_nodes(textparser_t handle, const textparser_node *node, unsigned depth, php_name_list *declared);

static bool parse_interpolation(textparser_t outer, const char *text, size_t length,
                                bool variable, unsigned depth) {
    if (depth > PHP_VALIDATION_DEPTH || length == 0 || length > INT_MAX) return false;
    const textparser_language_definition *definition = textparser_get_language(outer);
    int production = -1;
    const char *name = variable ? "InterpolatedVariable" : "Expression";
    for (size_t i = 0; i < definition->grammar->production_count; ++i) {
        if (definition->grammar->productions[i].name != nullptr &&
            strcmp(definition->grammar->productions[i].name, name) == 0)
            production = definition->grammar->productions[i].id;
    }
    if (production < 0) return false;
    textparser_t inner = nullptr;
    if (textparser_openmem(text, (int)length, TEXTPARSER_ENCODING_LATIN1, &inner) != 0) return false;
    bool valid = textparser_parse(inner, definition) == 0 && textparser_push_mode(inner, "PHP") == 0;
    for (size_t i = 0; valid && i < definition->operator_definition_count; ++i)
        valid = textparser_register_operator(inner, &definition->operator_definitions[i]) == 0;
    textparser_match_result result = {0};
    if (valid) valid = textparser_execute_production(inner, definition->grammar->productions,
        definition->grammar->production_count, production, &result) == 0 &&
        result.status == TEXTPARSER_MATCH_OK;
    if (valid) {
        const textparser_lex_token *remaining = nullptr;
        valid = textparser_lexer_peek(inner, 0, nullptr, &remaining) == 1;
        check_nodes(inner, result.node, depth + 1, nullptr);
        valid = valid && textparser_get_diagnostic_count(inner) == 0;
    }
    textparser_close(inner);
    return valid;
}

static bool id_start(unsigned char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch >= 0x80;
}

static bool id_part(unsigned char ch) { return id_start(ch) || (ch >= '0' && ch <= '9'); }

/* Locate a balanced interpolation boundary without mistaking braces in quoted
 * strings or comments for its closing delimiter. */
static size_t interpolation_end(const char *text, size_t start, size_t end) {
    unsigned braces = 1;
    for (size_t i = start; i < end; ++i) {
        char c = text[i];
        if (c == '\'' || c == '"' || c == '`') {
            char quote = c;
            while (++i < end && text[i] != quote) if (text[i] == '\\' && i + 1 < end) ++i;
        } else if (c == '/' && i + 1 < end && text[i + 1] == '*') {
            i += 2;
            while (i + 1 < end && !(text[i] == '*' && text[i + 1] == '/')) ++i;
            ++i;
        } else if (c == '#' || (c == '/' && i + 1 < end && text[i + 1] == '/')) {
            while (i < end && text[i] != '\n' && text[i] != '\r') ++i;
        } else if (c == '{') {
            if (++braces > PHP_VALIDATION_DEPTH) return end;
        } else if (c == '}' && --braces == 0) return i;
    }
    return end;
}

static bool check_interpolation(textparser_t handle, const char *text, size_t start,
                                size_t end, unsigned depth) {
    for (size_t i = start; i < end; ++i) {
        if (text[i] == '\\') { if (i + 1 < end) ++i; continue; }
        bool variable = text[i] == '{' && i + 1 < end && text[i + 1] == '$';
        bool expression = text[i] == '$' && i + 1 < end && text[i + 1] == '{';
        if (variable || expression) {
            size_t begin = i + (variable ? 1 : 2);
            size_t close = interpolation_end(text, begin, end);
            if (close == end || !parse_interpolation(handle, text + begin, close - begin,
                                                     variable, depth + 1)) return false;
            i = close;
        } else if (text[i] == '$' && i + 1 < end && id_start((unsigned char)text[i + 1])) {
            i += 2;
            while (i < end && id_part((unsigned char)text[i])) ++i;
            if (i < end && text[i] == '[') {
                size_t begin = ++i;
                if (i < end && text[i] == '$') {
                    ++i;
                    if (i >= end || !id_start((unsigned char)text[i])) return false;
                    while (i < end && id_part((unsigned char)text[i])) ++i;
                } else if (i < end && id_start((unsigned char)text[i])) {
                    while (i < end && id_part((unsigned char)text[i])) ++i;
                } else {
                    size_t offset = 0, matched = 0;
                    const char *integer = "-?(?:0[xX][0-9a-fA-F](?:_?[0-9a-fA-F])*|"
                        "0[bB][01](?:_?[01])*|0[oO][0-7](?:_?[0-7])*|[0-9](?:_?[0-9])*)";
                    if (!textparser_regex_match_pattern(handle, integer, text + i,
                            end - i, true, &offset, &matched) || offset != 0 || matched == 0)
                        return false;
                    i += matched;
                }
                if (i == begin || i >= end || text[i] != ']') return false;
            } else if (i > 0) --i;
        }
    }
    return true;
}

static bool heredoc_body(const char *text, size_t length, size_t *start, size_t *end) {
    size_t header = 0;
    while (header < length && text[header] != '\n') ++header;
    if (header == length) return false;
    *start = header + 1;
    size_t close = length;
    while (close > *start && text[close - 1] != '\n') --close;
    *end = close;
    size_t indent = 0;
    while (close + indent < length && (text[close + indent] == ' ' || text[close + indent] == '\t')) ++indent;
    char whitespace = indent ? text[close] : ' ';
    for (size_t i = 0; i < indent; ++i) if (text[close + i] != whitespace) return false;
    if (indent == 0) return true;
    for (size_t line = *start; line < close;) {
        size_t i = line;
        while (i < close && (text[i] == ' ' || text[i] == '\t')) {
            if (text[i] != whitespace && i - line < indent) return false;
            ++i;
        }
        bool blank = i == close || text[i] == '\n' || text[i] == '\r';
        if (!blank && i - line < indent) return false;
        while (i < close && text[i] != '\n') ++i;
        line = i + 1;
    }
    return true;
}

static void check_string(textparser_t handle, const textparser_node *node, unsigned depth) {
    size_t length = 0;
    char *text = node_text(handle, node, &length);
    if (!text) { report(handle, node, "PHP2005", "Unable to validate string contents."); return; }
    bool doc = kind(node, "Heredoc") || kind(node, "Nowdoc");
    size_t start = 1, end = length > 0 ? length - 1 : 0;
    if (doc && !heredoc_body(text, length, &start, &end)) {
        report(handle, node, "PHP2006", "Invalid heredoc/nowdoc indentation.");
    } else if (!kind(node, "Nowdoc") &&
               !check_interpolation(handle, text, start, end, depth + 1)) {
        report(handle, node, "PHP2005", "Invalid string interpolation.");
    }
    textparser_free_token_text(text);
}

static void check_nodes(textparser_t handle, const textparser_node *node, unsigned depth, php_name_list *declared) {
    if (depth > PHP_VALIDATION_DEPTH) {
        if (node) report(handle, node, "PHP2007", "PHP validation nesting limit exceeded.");
        return;
    }
    for (const textparser_node *item = node; item; item = item->next) {
        bool assignment = kind(item, "AssignOperator") || kind(item, "CompoundAssignOperator") ||
            kind(item, "PowerAssignOperator");
        if (assignment && item->child &&
            !writable(handle, item->child, kind(item, "AssignOperator"), 0))
            report(handle, item->child, "PHP2001", "Invalid assignment target.");
        if (kind(item, "IncDecOperator") && item->child && !writable(handle, item->child, false, 0))
            report(handle, item->child, "PHP2002", "Invalid increment/decrement target.");
        if (comparison(item, true) || comparison(item, false)) {
            bool relational = kind(item, "CompareOperator");
            if (comparison(item->child, relational) || comparison(child_at(item, 1), relational))
                report(handle, item, "PHP2003", "Comparison operators cannot be chained without parentheses.");
        }
        if (ternary(item)) {
            const textparser_node *left = item->child;
            const textparser_node *right = child_at(item, kind(item, "Question") ? 2 : 1);
            if ((ternary(left) && !(kind(item, "ElvisOperator") && kind(left, "ElvisOperator"))) ||
                (ternary(right) && !(kind(item, "ElvisOperator") && kind(right, "ElvisOperator"))))
                report(handle, item, "PHP2004", "Ternary operators require explicit parentheses when chained.");
        }
        if (kind(item, "CallSuffix")) check_call(handle, item, declared);
        if (kind(item, "ArrayLiteral") || kind(item, "LongArrayLiteral"))
            check_array_literal(handle, item);
        if (kind(item, "DoubleString") || kind(item, "BacktickString") ||
            kind(item, "Heredoc") || kind(item, "Nowdoc")) check_string(handle, item, depth);
        check_nodes(handle, item->child, depth + 1, declared);
    }
}

static textparser_action php_source_complete(textparser_t handle, const textparser_event *event,
                                             void *user_data) {
    (void)user_data;
    if (event && event->node) {
        php_name_list declared = {0};
        collect_declared_functions(handle, event->node, &declared);
        check_nodes(handle, event->node, 0, &declared);
        name_list_free(&declared);
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_PHP int textparser_php_register_validators(textparser_t handle) {
    if (!handle) return -1;
    return textparser_register_handler(handle, "php.legality", php_source_complete, nullptr);
}
